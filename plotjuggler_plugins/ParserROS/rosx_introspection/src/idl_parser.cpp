#include "rosx_introspection/idl_parser.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>

#include "rosx_introspection/contrib/peglib.h"
#include "rosx_introspection/idl_grammar.hpp"

namespace RosMsgParser {

namespace {

// Intermediate parsing result before type resolution
struct IDLParsingResult {
  std::vector<ROSMessage::Ptr> messages;
  std::vector<EnumDefinition> enums;
  std::vector<TypedefAlias> typedefs;
  std::vector<DiscriminatedUnion> unions;
  std::map<std::string, int64_t> constants;
};

// Normalize IDL scoped name (A::B::C) to ROSType format (A::B/C)
// Uses the last :: as the separator between module and type name
std::string normalizeTypeName(const std::string& scoped_name) {
  auto pos = scoped_name.rfind("::");
  if (pos == std::string::npos) {
    return scoped_name;
  }
  // Replace last "::" with "/"
  std::string result = scoped_name;
  result.replace(pos, 2, "/");
  // Replace remaining "::" with "::" (keep as-is for module path)
  return result;
}

// Build a full scoped name from current module path + identifier
std::string makeFullName(const std::string& module_path, const std::string& name) {
  if (module_path.empty()) {
    return name;
  }
  return module_path + "::" + name;
}

// Evaluate a constant expression AST node to an int64_t value
int64_t evaluateConstExpr(const std::shared_ptr<peg::Ast>& ast, const std::map<std::string, int64_t>& constants) {
  if (ast->name == "DEC_LITERAL") {
    return std::stoll(ast->token_to_string());
  }
  if (ast->name == "HEX_LITERAL") {
    return std::stoll(ast->token_to_string(), nullptr, 16);
  }
  if (ast->name == "FLOAT_LITERAL") {
    return static_cast<int64_t>(std::stod(ast->token_to_string()));
  }
  if (ast->name == "STRING_LITERAL") {
    return 0;  // strings can't be used as integer constants
  }
  if (ast->name == "SCOPED_NAME" || ast->name == "IDENTIFIER") {
    auto name = ast->token_to_string();
    auto it = constants.find(name);
    if (it != constants.end()) {
      return it->second;
    }
    throw std::runtime_error("Unknown constant: " + name);
  }
  if (ast->name == "PRIMARY_EXPR") {
    // Could be (CONST_EXPR), HEX_LITERAL, DEC_LITERAL, or SCOPED_NAME
    if (ast->nodes.size() == 1) {
      return evaluateConstExpr(ast->nodes[0], constants);
    }
    // Parenthesized expression: nodes[0] is the inner CONST_EXPR
    return evaluateConstExpr(ast->nodes[0], constants);
  }
  if (ast->name == "UNARY_EXPR") {
    if (ast->nodes.size() == 2) {
      // NEG_OP + PRIMARY_EXPR
      return -evaluateConstExpr(ast->nodes[1], constants);
    }
    return evaluateConstExpr(ast->nodes[0], constants);
  }
  if (ast->name == "MULT_EXPR") {
    int64_t result = evaluateConstExpr(ast->nodes[0], constants);
    for (size_t i = 1; i < ast->nodes.size(); i += 2) {
      auto op = ast->nodes[i]->token_to_string();
      auto right = evaluateConstExpr(ast->nodes[i + 1], constants);
      if (op == "*") {
        result *= right;
      } else if (op == "/") {
        if (right == 0) {
          throw std::runtime_error("Division by zero in constant expression");
        }
        result /= right;
      } else if (op == "%") {
        if (right == 0) {
          throw std::runtime_error("Modulo by zero in constant expression");
        }
        result %= right;
      }
    }
    return result;
  }
  if (ast->name == "ADD_EXPR") {
    int64_t result = evaluateConstExpr(ast->nodes[0], constants);
    for (size_t i = 1; i < ast->nodes.size(); i += 2) {
      auto op = ast->nodes[i]->token_to_string();
      auto right = evaluateConstExpr(ast->nodes[i + 1], constants);
      if (op == "+") {
        result += right;
      } else if (op == "-") {
        result -= right;
      }
    }
    return result;
  }
  if (ast->name == "CONST_EXPR") {
    if (ast->nodes.size() == 1) {
      return evaluateConstExpr(ast->nodes[0], constants);
    }
    // CONST_EXPR is ADD_EXPR which handles +/-
    return evaluateConstExpr(ast->nodes[0], constants);
  }
  throw std::runtime_error("Cannot evaluate expression node: " + ast->name);
}

// After optimize_ast(), collapsed nodes swap names:
//   name = inner (most specific) rule name
//   original_name = outer (parent) rule name
// Use `roleName()` to determine a node's semantic role in its parent rule.
// Use `ast->name` to determine the node's own type.
std::string roleName(const std::shared_ptr<peg::Ast>& ast) {
  return ast->original_name.empty() ? ast->name : ast->original_name;
}

// Extract type name string from a TYPE_SPEC AST node.
// Uses ast->name (the node's own type after optimization).
std::string extractTypeName(const std::shared_ptr<peg::Ast>& ast) {
  const auto& n = ast->name;

  if (n == "BASE_TYPE" || n == "SCOPED_NAME" || n == "IDENTIFIER") {
    return ast->token_to_string();
  }
  if (n == "STRING_TYPE") {
    return "string";
  }
  if (n == "SEQUENCE_TYPE") {
    return extractTypeName(ast->nodes[0]);
  }
  if (n == "TYPE_SPEC") {
    if (ast->is_token) {
      return ast->token_to_string();
    }
    if (!ast->nodes.empty()) {
      return extractTypeName(ast->nodes[0]);
    }
  }
  if (ast->is_token) {
    return ast->token_to_string();
  }
  throw std::runtime_error("Cannot extract type name from: " + n + " (original: " + ast->original_name + ")");
}

// Check if a TYPE_SPEC node represents a sequence
bool isSequenceType(const std::shared_ptr<peg::Ast>& ast) {
  if (ast->name == "SEQUENCE_TYPE") {
    return true;
  }
  if ((ast->name == "TYPE_SPEC" || ast->original_name == "TYPE_SPEC") && !ast->nodes.empty()) {
    return isSequenceType(ast->nodes[0]);
  }
  return false;
}

// Check if a TYPE_SPEC node represents a string type
bool isStringType(const std::shared_ptr<peg::Ast>& ast) {
  if (ast->name == "STRING_TYPE") {
    return true;
  }
  if ((ast->name == "TYPE_SPEC" || ast->original_name == "TYPE_SPEC") && !ast->nodes.empty()) {
    return isStringType(ast->nodes[0]);
  }
  return false;
}

// Get the inner type of a sequence
std::string getSequenceInnerType(const std::shared_ptr<peg::Ast>& ast) {
  if (ast->name == "SEQUENCE_TYPE") {
    return extractTypeName(ast->nodes[0]);
  }
  if ((ast->name == "TYPE_SPEC" || ast->original_name == "TYPE_SPEC") && !ast->nodes.empty()) {
    return getSequenceInnerType(ast->nodes[0]);
  }
  throw std::runtime_error("Not a sequence type");
}

// Parse annotations from a list of AST nodes, returning flags
struct AnnotationFlags {
  bool is_key = false;
  bool is_optional = false;
};

// Extract the name from an ANNOTATION node (handles both token and structured forms)
std::string getAnnotationName(const std::shared_ptr<peg::Ast>& node) {
  if (node->is_token) {
    return node->token_to_string();
  }
  for (const auto& child : node->nodes) {
    if (child->name == "ANNOTATION_NAME") {
      return child->token_to_string();
    }
  }
  return {};
}

// Extract the first parameter value from an ANNOTATION node (e.g., @value(42) → "42")
std::string getAnnotationParam(const std::shared_ptr<peg::Ast>& node) {
  for (const auto& child : node->nodes) {
    if (child->name == "ANNOTATION_ARGS") {
      for (const auto& param : child->nodes) {
        if (param->name == "ANNOTATION_PARAM") {
          return param->token_to_string();
        }
      }
    }
  }
  return {};
}

// Main AST walker class
class IDLAstWalker {
 public:
  IDLParsingResult& result;

  IDLAstWalker(IDLParsingResult& r) : result(r) {}

  void walk(const std::shared_ptr<peg::Ast>& ast, const std::string& module_path = "") {
    // After optimization: name=inner_rule (most specific), original_name=outer_rule.
    // Use ast->name for dispatch (it's the actual rule that matched).
    const auto& n = ast->name;

    if (n == "MODULE_DCL") {
      handleModule(ast, module_path);
    } else if (n == "STRUCT_DCL") {
      handleStruct(ast, module_path);
    } else if (n == "ENUM_DCL") {
      handleEnum(ast, module_path);
    } else if (n == "UNION_DCL") {
      handleUnion(ast, module_path);
    } else if (n == "TYPEDEF_DCL") {
      handleTypedef(ast, module_path);
    } else if (n == "CONST_DCL") {
      handleConst(ast, module_path);
    } else if (n == "DOCUMENT" || n.substr(0, 10) == "DEFINITION") {
      // Container nodes — recurse into children
      for (const auto& child : ast->nodes) {
        walk(child, module_path);
      }
    }
    // PREPROCESSOR, ANNOTATION_DCL, etc. — skip silently
  }

 private:
  void handleModule(const std::shared_ptr<peg::Ast>& ast, const std::string& module_path) {
    // Children: [ANNOTATION*] IDENTIFIER DEFINITION+
    // After optimization, DEFINITION may be collapsed to its child
    std::string module_name;
    for (const auto& child : ast->nodes) {
      auto role = roleName(child);
      if (role == "IDENTIFIER" && module_name.empty()) {
        module_name = child->token_to_string();
      } else if (!module_name.empty()) {
        walk(child, makeFullName(module_path, module_name));
      }
    }
  }

  void handleStruct(const std::shared_ptr<peg::Ast>& ast, const std::string& module_path) {
    std::string struct_name;
    std::string base_type;

    auto msg = std::make_shared<ROSMessage>(std::string{});

    for (const auto& child : ast->nodes) {
      auto role = roleName(child);
      if (role == "IDENTIFIER" && struct_name.empty()) {
        struct_name = child->token_to_string();
      } else if (role == "INHERITANCE") {
        if (child->is_token) {
          base_type = child->token_to_string();
        } else if (!child->nodes.empty()) {
          base_type = child->nodes[0]->token_to_string();
        }
      } else if (role == "MEMBER") {
        auto fields = parseMember(child, module_path);
        for (auto& f : fields) {
          msg->fields().push_back(std::move(f));
        }
      }
    }

    auto full_name = makeFullName(module_path, struct_name);
    msg->setType(ROSType(full_name));

    // Store base type info for later resolution
    if (!base_type.empty()) {
      // We'll resolve inheritance after all structs are parsed
      _inheritance_map[full_name] = base_type;
    }

    result.messages.push_back(std::move(msg));
  }

  struct DeclaratorInfo {
    std::string field_name;
    SmallVector<int, 2> array_dims;  // each dimension separately: {3, 4} for [3][4]
  };

  DeclaratorInfo parseDeclarator(const std::shared_ptr<peg::Ast>& child) {
    DeclaratorInfo info;
    if (child->is_token) {
      info.field_name = child->token_to_string();
    } else {
      for (const auto& decl_child : child->nodes) {
        if (decl_child->name == "IDENTIFIER" || decl_child->original_name == "IDENTIFIER") {
          info.field_name = decl_child->token_to_string();
        } else if (decl_child->name == "FIXED_ARRAY_SIZE" || decl_child->original_name == "FIXED_ARRAY_SIZE") {
          int dim;
          if (!decl_child->nodes.empty()) {
            dim = static_cast<int>(evaluateConstExpr(decl_child->nodes[0], result.constants));
          } else {
            dim = static_cast<int>(evaluateConstExpr(decl_child, result.constants));
          }
          info.array_dims.push_back(dim);
        }
      }
    }
    return info;
  }

  std::vector<ROSField> parseMember(const std::shared_ptr<peg::Ast>& ast, const std::string& module_path) {
    // MEMBER children: [ANNOTATION*] TYPE_SPEC DECLARATORS
    // DECLARATORS: DECLARATOR (COMMA DECLARATOR)*
    AnnotationFlags flags;
    std::string type_name;
    bool is_sequence = false;
    std::vector<DeclaratorInfo> declarators;

    for (const auto& child : ast->nodes) {
      auto role = child->original_name.empty() ? child->name : child->original_name;

      if (role == "ANNOTATION" || child->name == "ANNOTATION") {
        auto ann_name = getAnnotationName(child);
        if (ann_name == "key") {
          flags.is_key = true;
        } else if (ann_name == "optional") {
          flags.is_optional = true;
        }
      } else if (role == "TYPE_SPEC") {
        if (isSequenceType(child)) {
          is_sequence = true;
          type_name = getSequenceInnerType(child);
        } else if (isStringType(child)) {
          type_name = "string";
        } else {
          type_name = extractTypeName(child);
        }
      } else if (role == "DECLARATORS" || role == "DECLARATOR") {
        // DECLARATORS contains one or more DECLARATOR children
        if (child->name == "DECLARATORS") {
          for (const auto& decl : child->nodes) {
            if (decl->name == "DECLARATOR" || decl->original_name == "DECLARATOR") {
              declarators.push_back(parseDeclarator(decl));
            }
          }
        } else {
          // Single DECLARATOR (may happen after optimization)
          declarators.push_back(parseDeclarator(child));
        }
      }
    }

    // If no declarators found, try treating the whole node's last child as a declarator
    if (declarators.empty()) {
      declarators.push_back({"unknown", {}});
    }

    auto resolved_type_name = resolveTypeName(type_name, module_path);

    std::vector<ROSField> fields;
    for (const auto& decl : declarators) {
      // Compute total array size from dimensions
      int total_size = 1;
      for (int d : decl.array_dims) {
        total_size *= d;
      }
      int arr_size = is_sequence ? -1 : (decl.array_dims.empty() ? 1 : total_size);

      ROSField field(ROSType(resolved_type_name), decl.field_name);
      field.setArray(arr_size != 1, arr_size);
      if (decl.array_dims.size() > 1) {
        field.setArrayDimensions(decl.array_dims);
      }
      field.setIsKey(flags.is_key);
      field.setOptional(flags.is_optional);
      fields.push_back(std::move(field));
    }
    return fields;
  }

  void handleEnum(const std::shared_ptr<peg::Ast>& ast, const std::string& module_path) {
    std::string enum_name;
    EnumDefinition def;

    int32_t next_value = 0;
    int32_t next_dds_compat_value = 0;
    for (const auto& child : ast->nodes) {
      auto role = roleName(child);
      if (role == "IDENTIFIER" && enum_name.empty()) {
        enum_name = child->token_to_string();
      } else if (role == "ENUMERATOR") {
        std::string name;
        std::optional<int32_t> explicit_value;
        bool explicit_value_from_annotation = false;

        // After optimization, ENUMERATOR may be a leaf (ENUMERATOR[IDENTIFIER])
        if (child->is_token) {
          name = child->token_to_string();
        } else {
          for (const auto& ec : child->nodes) {
            auto ecrole = roleName(ec);
            if (ecrole == "IDENTIFIER") {
              name = ec->token_to_string();
            } else if (ec->name == "ANNOTATION") {
              // Handle @value(N) annotation — extract N as explicit value
              auto ann_name = getAnnotationName(ec);
              if (ann_name == "value") {
                auto param = getAnnotationParam(ec);
                if (!param.empty()) {
                  try {
                    // Trim whitespace
                    while (!param.empty() && std::isspace(param.front())) {
                      param.erase(param.begin());
                    }
                    explicit_value = static_cast<int32_t>(std::stoll(param, nullptr, 0));
                    explicit_value_from_annotation = true;
                  } catch (...) {}
                }
              }
            } else {
              // Anything else is a constant expression (e.g., = value)
              explicit_value = static_cast<int32_t>(evaluateConstExpr(ec, result.constants));
              explicit_value_from_annotation = false;
            }
          }
        }
        const int32_t enum_value = explicit_value.value_or(next_value);
        const int32_t dds_compat_value =
            explicit_value.has_value() && !explicit_value_from_annotation ? *explicit_value : next_dds_compat_value;
        def.values.push_back({name, enum_value, dds_compat_value});
        auto full_enum_name = makeFullName(module_path, name);
        result.constants[full_enum_name] = enum_value;
        result.constants[name] = enum_value;
        next_value = enum_value + 1;
        next_dds_compat_value = dds_compat_value + 1;
      }
    }

    auto full_name = makeFullName(module_path, enum_name);
    def.id = ROSType(full_name);
    result.enums.push_back(std::move(def));
  }

  void handleUnion(const std::shared_ptr<peg::Ast>& ast, const std::string& module_path) {
    std::string union_name;
    DiscriminatedUnion def;

    for (const auto& child : ast->nodes) {
      auto role = roleName(child);
      if (role == "IDENTIFIER" && union_name.empty()) {
        union_name = child->token_to_string();
      } else if (role == "SWITCH_TYPE") {
        if (child->is_token) {
          def.discriminant_type = child->token_to_string();
        } else if (!child->nodes.empty()) {
          def.discriminant_type = extractTypeName(child->nodes[0]);
        } else {
          def.discriminant_type = extractTypeName(child);
        }
      } else if (role == "CASE") {
        handleCase(child, def, module_path);
      }
    }

    auto full_name = makeFullName(module_path, union_name);
    def.id = ROSType(full_name);
    result.unions.push_back(std::move(def));
  }

  void handleCase(const std::shared_ptr<peg::Ast>& ast, DiscriminatedUnion& union_def, const std::string& module_path) {
    // CASE children: CASE_LABEL+ [ANNOTATION*] TYPE_SPEC DECLARATOR
    std::vector<std::string> labels;
    bool is_default = false;
    std::string type_name;
    std::string field_name;
    int array_size = 1;

    for (const auto& child : ast->nodes) {
      auto role = roleName(child);

      if (role == "CASE_LABEL") {
        // CASE_LABEL/0 = 'case' CONST_EXPR ':' (has children for the expression)
        // CASE_LABEL/1 = 'default' ':'       (no children, KW_DEFAULT is suppressed)
        if (child->nodes.empty() && !child->is_token) {
          // No children = default label (KW_DEFAULT was suppressed by grammar)
          is_default = true;
        } else if (child->is_token) {
          auto tok = child->token_to_string();
          if (tok == "default") {
            is_default = true;
          } else {
            try {
              auto val = evaluateConstExpr(child, result.constants);
              labels.push_back(std::to_string(val));
            } catch (...) {
              labels.push_back(tok);
            }
          }
        } else {
          for (const auto& lc : child->nodes) {
            try {
              auto val = evaluateConstExpr(lc, result.constants);
              labels.push_back(std::to_string(val));
            } catch (...) {
              labels.push_back(lc->token_to_string());
            }
          }
        }
      } else if (role == "TYPE_SPEC") {
        if (isSequenceType(child)) {
          type_name = getSequenceInnerType(child);
          array_size = -1;
        } else {
          type_name = extractTypeName(child);
        }
      } else if (role == "DECLARATOR") {
        if (child->is_token) {
          field_name = child->token_to_string();
        } else {
          for (const auto& dc : child->nodes) {
            auto dcrole = roleName(dc);
            if (dcrole == "IDENTIFIER") {
              field_name = dc->token_to_string();
            } else if (dcrole == "FIXED_ARRAY_SIZE") {
              if (!dc->nodes.empty()) {
                array_size = static_cast<int>(evaluateConstExpr(dc->nodes[0], result.constants));
              }
            }
          }
        }
      }
    }

    auto resolved_type_name = resolveTypeName(type_name, module_path);
    UnionCaseField case_field;
    case_field.type = ROSType(resolved_type_name);
    case_field.field_name = field_name;
    case_field.is_array = (array_size != 1);
    case_field.array_size = array_size;

    if (is_default) {
      union_def.default_case = std::move(case_field);
    } else {
      for (const auto& label : labels) {
        union_def.cases.insert({label, case_field});
      }
    }
  }

  void handleTypedef(const std::shared_ptr<peg::Ast>& ast, const std::string& module_path) {
    // TYPEDEF_DCL children: TYPE_SPEC DECLARATOR
    std::string base_type;
    std::string alias_name;

    for (const auto& child : ast->nodes) {
      auto role = roleName(child);
      if (role == "TYPE_SPEC") {
        if (isStringType(child)) {
          base_type = "string";
        } else {
          base_type = extractTypeName(child);
        }
      } else if (role == "DECLARATORS" || role == "DECLARATOR") {
        // For typedefs, extract the alias name from the first declarator
        if (child->name == "DECLARATORS") {
          for (const auto& decl : child->nodes) {
            if (decl->name == "DECLARATOR" || decl->original_name == "DECLARATOR") {
              auto info = parseDeclarator(decl);
              alias_name = info.field_name;
              break;  // only use first declarator for typedef
            }
          }
        } else if (child->is_token) {
          alias_name = child->token_to_string();
        } else {
          auto info = parseDeclarator(child);
          alias_name = info.field_name;
        }
      }
    }

    auto full_name = makeFullName(module_path, alias_name);
    TypedefAlias alias;
    alias.id = ROSType(full_name);
    alias.base_type = base_type;
    alias.resolved_type = toBuiltinType(base_type);
    result.typedefs.push_back(std::move(alias));
  }

  void handleConst(const std::shared_ptr<peg::Ast>& ast, const std::string& module_path) {
    // CONST_DCL children: TYPE_SPEC IDENTIFIER CONST_EXPR
    std::string const_name;
    int64_t const_value = 0;
    bool found_name = false;

    for (const auto& child : ast->nodes) {
      auto role = roleName(child);
      if (role == "TYPE_SPEC") {
        // Skip the type — we only need name and value
        continue;
      }
      if (role == "IDENTIFIER" && !found_name) {
        // Skip first IDENTIFIER if it's actually the type name for non-base types
        // The grammar has: CONST_DCL <- 'const' TYPE_SPEC IDENTIFIER '=' CONST_EXPR
        // TYPE_SPEC comes first, then the const name
        const_name = child->token_to_string();
        found_name = true;
      } else {
        // Everything else is part of the constant expression
        const_value = evaluateConstExpr(child, result.constants);
      }
    }

    auto full_name = makeFullName(module_path, const_name);
    result.constants[full_name] = const_value;
    result.constants[const_name] = const_value;
  }

  // Try to resolve a type name using the current module path
  std::string resolveTypeName(const std::string& type_name, const std::string& module_path) {
    // If it's a builtin type, return as-is
    if (toBuiltinType(type_name) != OTHER) {
      return type_name;
    }
    // If it already contains "::", it's fully qualified
    if (type_name.find("::") != std::string::npos) {
      return type_name;
    }
    // Try to qualify with current module
    if (!module_path.empty()) {
      return makeFullName(module_path, type_name);
    }
    return type_name;
  }

 public:
  // Map of struct full_name -> base type name (for inheritance resolution)
  std::map<std::string, std::string> _inheritance_map;
};

}  // anonymous namespace

MessageSchema::Ptr ParseIDL(const std::string& topic_name, const ROSType& root_type, const std::string& idl_schema) {
  // Set up the PEG parser
  peg::parser parser(idl_grammar());

  parser.enable_ast();

  std::string parse_error;
  parser.set_logger([&](size_t line, size_t col, const std::string& msg, const std::string& /*rule*/) {
    parse_error += "IDL parse error at line " + std::to_string(line) + ":" + std::to_string(col) + ": " + msg + "\n";
  });

  // Parse the IDL string
  std::shared_ptr<peg::Ast> ast;
  if (!parser.parse(idl_schema, ast)) {
    throw std::runtime_error("Failed to parse IDL schema:\n" + parse_error);
  }

  // NOTE: We intentionally do NOT call optimize_ast() here.
  // Optimization collapses single-child nodes (e.g., SEQUENCE_TYPE → BASE_TYPE),
  // which loses structural information we need (like whether a type is a sequence).

  // Walk the AST to build the intermediate result
  IDLParsingResult parsing_result;
  IDLAstWalker walker(parsing_result);
  walker.walk(ast);

  // --- Type Resolution Pass ---
  // (ported from dds_parser.cpp:19-139)

  // Build lookup maps
  std::unordered_map<ROSType, ROSMessage::Ptr> msg_map;
  for (auto& msg : parsing_result.messages) {
    msg_map[msg->type()] = msg;
  }

  std::unordered_map<ROSType, EnumDefinition*> enum_map;
  for (auto& def : parsing_result.enums) {
    enum_map[def.id] = &def;
  }

  std::unordered_map<ROSType, TypedefAlias*> typedef_map;
  for (auto& def : parsing_result.typedefs) {
    typedef_map[def.id] = &def;
  }

  std::unordered_map<ROSType, DiscriminatedUnion*> union_map;
  for (auto& def : parsing_result.unions) {
    union_map[def.id] = &def;
  }

  // Resolve struct inheritance: prepend base struct fields
  for (auto& [derived_name, base_name] : walker._inheritance_map) {
    auto derived_type = ROSType(derived_name);

    // Try to find base type — first as-is, then qualified with derived's module
    auto base_type = ROSType(base_name);
    auto derived_it = msg_map.find(derived_type);
    auto base_it = msg_map.find(base_type);
    if (base_it == msg_map.end()) {
      // Qualify base_name with derived's module path
      auto pos = derived_name.rfind("::");
      if (pos != std::string::npos) {
        auto module = derived_name.substr(0, pos);
        base_type = ROSType(module + "::" + base_name);
        base_it = msg_map.find(base_type);
      }
    }

    if (derived_it != msg_map.end() && base_it != msg_map.end()) {
      auto& derived_fields = derived_it->second->fields();
      auto& base_fields = base_it->second->fields();
      derived_fields.insert(derived_fields.begin(), base_fields.begin(), base_fields.end());
    }
  }

  // Collect all fields for typedef substitution and pointer resolution
  std::vector<ROSField*> all_fields;
  for (auto& msg : parsing_result.messages) {
    for (auto& field : msg->fields()) {
      all_fields.push_back(&field);
    }
  }
  // Collect union case fields separately (they use UnionCaseField, not ROSField)
  std::vector<UnionCaseField*> all_union_fields;
  for (auto& udef : parsing_result.unions) {
    for (auto& [key, field] : udef.cases) {
      all_union_fields.push_back(&field);
    }
    if (udef.default_case) {
      all_union_fields.push_back(&udef.default_case.value());
    }
  }

  // Substitute typedefs on struct fields
  for (auto* field : all_fields) {
    if (field->type().isBuiltin()) {
      continue;
    }
    auto td_it = typedef_map.find(field->type());
    if (td_it != typedef_map.end()) {
      auto resolved = toBuiltinType(td_it->second->base_type);
      if (resolved != OTHER) {
        field->changeType(ROSType(td_it->second->base_type));
      }
    }
  }

  // Substitute typedefs on union case fields
  for (auto* ufield : all_union_fields) {
    if (ufield->type.isBuiltin()) {
      continue;
    }
    auto td_it = typedef_map.find(ufield->type);
    if (td_it != typedef_map.end()) {
      auto resolved = toBuiltinType(td_it->second->base_type);
      if (resolved != OTHER) {
        ufield->type = ROSType(td_it->second->base_type);
      }
    }
  }

  // --- Build MessageSchema ---
  auto schema = std::make_shared<MessageSchema>();
  schema->topic_name = topic_name;

  // Find root message.
  // IDL type names are canonically stored with "::" (e.g. "ims_msgs::RoboticsInputs").
  // Callers may still hand us a ROS-style "pkg/Type" name; translate the last '/'
  // to '::' and retry before falling back to a msg-name only match.
  auto root_it = msg_map.find(root_type);
  if (root_it == msg_map.end()) {
    const std::string& base = root_type.baseName();
    auto slash = base.rfind('/');
    if (slash != std::string::npos) {
      std::string idl_style = base;
      idl_style.replace(slash, 1, "::");
      root_it = msg_map.find(ROSType(idl_style));
    }
  }
  if (root_it == msg_map.end()) {
    for (auto& [type, msg] : msg_map) {
      if (type.msgName() == root_type.msgName()) {
        root_it = msg_map.find(type);
        break;
      }
    }
    if (root_it == msg_map.end()) {
      throw std::runtime_error("Root message type not found: " + root_type.baseName());
    }
  }
  schema->root_msg = root_it->second;
  schema->msg_library = std::move(msg_map);

  // Move IDL types into schema FIRST so pointers remain valid
  for (auto& def : parsing_result.enums) {
    auto id = def.id;
    schema->enum_library[id] = std::move(def);
  }
  for (auto& def : parsing_result.unions) {
    auto id = def.id;
    schema->union_library[id] = std::move(def);
  }
  for (auto& def : parsing_result.typedefs) {
    schema->typedef_library[def.id] = std::move(def);
  }

  // NOW set enum/union/struct pointers on struct fields (pointing into schema's maps)
  for (auto* field : all_fields) {
    if (field->type().isBuiltin()) {
      continue;
    }

    auto enum_it = schema->enum_library.find(field->type());
    if (enum_it != schema->enum_library.end()) {
      field->setEnumPtr(&enum_it->second);
      continue;
    }

    auto union_it = schema->union_library.find(field->type());
    if (union_it != schema->union_library.end()) {
      field->setUnionPtr(&union_it->second);
      continue;
    }
  }

  // Build field tree (reusing the existing BuildMessageSchema pattern)
  // Create synthetic root field. Use the type that matched the library (canonical
  // "::" form) so downstream lookups by baseName() hit.
  schema->root_field = std::make_unique<ROSField>(schema->root_msg->type(), topic_name);
  schema->field_tree.root()->setValue(schema->root_field.get());

  std::function<void(const ROSMessage&, details::TreeNode<const ROSField*>*)> recursiveTreeCreator;

  recursiveTreeCreator = [&](const ROSMessage& msg, details::TreeNode<const ROSField*>* node) {
    // Must reserve capacity before adding children (tree uses assert on capacity)
    node->children().reserve(msg.fields().size());

    for (const auto& field : msg.fields()) {
      if (field.isConstant()) {
        continue;
      }

      auto* new_node = node->addChild(&field);

      if (!field.type().isBuiltin()) {
        // Check if it's a struct (recurse into it)
        if (field.getEnum() == nullptr && field.getUnion() == nullptr) {
          auto child_it = schema->msg_library.find(field.type());
          if (child_it != schema->msg_library.end()) {
            recursiveTreeCreator(*child_it->second, new_node);
          }
        }
        // For unions, we don't recurse into cases in the field tree.
        // Union case resolution happens at deserialization time.
        // The field tree just has the union field as a leaf-like node.
      }
    }
  };

  recursiveTreeCreator(*schema->root_msg, schema->field_tree.root());

  CacheFieldTreePaths(schema->field_tree);

  return schema;
}

}  // namespace RosMsgParser
