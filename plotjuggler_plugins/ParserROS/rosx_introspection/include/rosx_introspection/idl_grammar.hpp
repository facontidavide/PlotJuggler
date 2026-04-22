#pragma once

namespace RosMsgParser {

/// PEG grammar for the DDS IDL subset used by this library.
/// Covers: modules, structs (with inheritance), enums, unions, typedefs,
/// constants, sequences, bounded strings, arrays, annotations, and
/// constant expressions with arithmetic.
inline const char* idl_grammar() {
  static const char* grammar = R"(

DOCUMENT        <- WS? DEFINITION* WS?
DEFINITION      <- WS? ((MODULE_DCL / CONST_DCL / STRUCT_DCL / ENUM_DCL / UNION_DCL / TYPEDEF_DCL
                 / FORWARD_DCL) SEMICOLON / PREPROCESSOR)

# Forward declarations (parsed and ignored)
FORWARD_DCL     <- ANNOTATION* (KW_STRUCT / KW_UNION) IDENTIFIER

# Modules
MODULE_DCL      <- ANNOTATION* KW_MODULE IDENTIFIER OPEN_BRACE DEFINITION+ CLOSE_BRACE

# Structs
STRUCT_DCL      <- ANNOTATION* KW_STRUCT IDENTIFIER INHERITANCE? OPEN_BRACE MEMBER* CLOSE_BRACE
INHERITANCE     <- COLON SCOPED_NAME
MEMBER          <- ANNOTATION* TYPE_SPEC DECLARATORS SEMICOLON

# Multiple declarators: int32 a, b, c;
DECLARATORS     <- DECLARATOR (COMMA DECLARATOR)*

# Declarators (field name, possibly with array dimensions)
DECLARATOR      <- IDENTIFIER FIXED_ARRAY_SIZE*
FIXED_ARRAY_SIZE <- OPEN_BRACKET CONST_EXPR CLOSE_BRACKET

# Enums
ENUM_DCL        <- KW_ENUM IDENTIFIER OPEN_BRACE ENUMERATOR (COMMA ENUMERATOR)* CLOSE_BRACE
ENUMERATOR      <- ANNOTATION* IDENTIFIER (EQUAL_OP CONST_EXPR)?

# Unions
UNION_DCL       <- KW_UNION IDENTIFIER WS? KW_SWITCH OPEN_PAREN SWITCH_TYPE CLOSE_PAREN
                   OPEN_BRACE CASE+ CLOSE_BRACE
SWITCH_TYPE     <- TYPE_SPEC
CASE            <- CASE_LABEL+ ANNOTATION* TYPE_SPEC DECLARATOR SEMICOLON
CASE_LABEL      <- KW_CASE CONST_EXPR COLON / KW_DEFAULT COLON

# Typedefs
TYPEDEF_DCL     <- KW_TYPEDEF TYPE_SPEC DECLARATORS

# Constants
CONST_DCL       <- KW_CONST TYPE_SPEC IDENTIFIER EQUAL_OP CONST_EXPR

# Type specifications
TYPE_SPEC       <- WS? (SEQUENCE_TYPE / STRING_TYPE / BASE_TYPE / SCOPED_NAME) WS?
SEQUENCE_TYPE   <- KW_SEQUENCE OPEN_ANG TYPE_SPEC (COMMA CONST_EXPR)? CLOSE_ANG
STRING_TYPE     <- KW_STRING (OPEN_ANG CONST_EXPR CLOSE_ANG)?
BASE_TYPE       <- < 'unsigned' WS 'long' WS 'long'
                 / 'unsigned' WS 'long'
                 / 'unsigned' WS 'short'
                 / 'long' WS 'long'
                 / 'float64' / 'float32'
                 / 'uint64' / 'uint32' / 'uint16' / 'uint8'
                 / 'int64' / 'int32' / 'int16' / 'int8'
                 / 'boolean' / 'octet' / 'float' / 'double'
                 / 'long' / 'short' / 'char' / 'bool' / 'byte' > END_KW

# Constant expressions with arithmetic
CONST_EXPR      <- ADD_EXPR
ADD_EXPR        <- MULT_EXPR (ADD_OP MULT_EXPR)*
MULT_EXPR       <- UNARY_EXPR (MULT_OP UNARY_EXPR)*
UNARY_EXPR      <- NEG_OP PRIMARY_EXPR / PRIMARY_EXPR
PRIMARY_EXPR    <- OPEN_PAREN CONST_EXPR CLOSE_PAREN / FLOAT_LITERAL / HEX_LITERAL / DEC_LITERAL
                 / STRING_LITERAL / SCOPED_NAME

# Annotations
ANNOTATION      <- WS? '@' ANNOTATION_NAME ANNOTATION_ARGS? WS?
ANNOTATION_NAME <- < [a-zA-Z_][a-zA-Z0-9_]* >
ANNOTATION_ARGS <- OPEN_PAREN ANNOTATION_PARAM (COMMA ANNOTATION_PARAM)* CLOSE_PAREN
ANNOTATION_PARAM <- < (!(')' / ',') .)+ >

# Identifiers and scoped names
SCOPED_NAME     <- < IDENTIFIER (DOUBLE_COLON IDENTIFIER)* >
IDENTIFIER      <- < [a-zA-Z_][a-zA-Z0-9_]* >

# Literals
FLOAT_LITERAL   <- < [0-9]+ '.' [0-9]* ([eE] [+-]? [0-9]+)? / '.' [0-9]+ ([eE] [+-]? [0-9]+)? / [0-9]+ [eE] [+-]? [0-9]+ >
HEX_LITERAL     <- < '0' [xX] [0-9a-fA-F]+ >
DEC_LITERAL     <- < [0-9]+ >
STRING_LITERAL  <- < '"' (!'"' .)* '"' >

# Preprocessor directives (stripped)
PREPROCESSOR    <- WS? '#' (!EOL .)* EOL?
EOL             <- '\r\n' / '\n' / '\r'

# Keywords (with word-boundary checking)
~END_KW         <- ![a-zA-Z0-9_]
~KW_MODULE      <- 'module' END_KW WS?
~KW_STRUCT      <- 'struct' END_KW WS?
~KW_ENUM        <- 'enum' END_KW WS?
~KW_UNION       <- 'union' END_KW WS?
~KW_SWITCH      <- 'switch' END_KW WS?
~KW_CASE        <- 'case' END_KW WS?
~KW_DEFAULT     <- 'default' END_KW WS?
~KW_TYPEDEF     <- 'typedef' END_KW WS?
~KW_CONST       <- 'const' END_KW WS?
~KW_SEQUENCE    <- 'sequence' END_KW WS?
~KW_STRING      <- 'string' END_KW WS?

# Base type keywords
~KW_BOOLEAN     <- 'boolean' END_KW
~KW_OCTET       <- 'octet' END_KW
~KW_CHAR        <- 'char' END_KW
~KW_FLOAT       <- 'float' END_KW
~KW_DOUBLE      <- 'double' END_KW
~KW_SHORT       <- 'short' END_KW
~KW_LONG        <- 'long' END_KW
~KW_LONG_LONG   <- 'long' WS 'long' END_KW
~KW_UNSIGNED_SHORT     <- 'unsigned' WS 'short' END_KW
~KW_UNSIGNED_LONG      <- 'unsigned' WS 'long' END_KW
~KW_UNSIGNED_LONG_LONG <- 'unsigned' WS 'long' WS 'long' END_KW
~KW_UINT8       <- 'uint8' END_KW
~KW_UINT16      <- 'uint16' END_KW
~KW_UINT32      <- 'uint32' END_KW
~KW_UINT64      <- 'uint64' END_KW
~KW_INT8        <- 'int8' END_KW
~KW_INT16       <- 'int16' END_KW
~KW_INT32       <- 'int32' END_KW
~KW_INT64       <- 'int64' END_KW
~KW_FLOAT32     <- 'float32' END_KW
~KW_FLOAT64     <- 'float64' END_KW
~KW_BOOL        <- 'bool' END_KW
~KW_BYTE        <- 'byte' END_KW

# Symbols
~SEMICOLON      <- WS? ';' WS?
~COLON          <- WS? ':' WS?
~DOUBLE_COLON   <- '::'
~COMMA          <- WS? ',' WS?
~EQUAL_OP       <- WS? '=' WS?
~OPEN_PAREN     <- WS? '(' WS?
~CLOSE_PAREN    <- WS? ')' WS?
~OPEN_BRACKET   <- WS? '[' WS?
~CLOSE_BRACKET  <- WS? ']' WS?
~OPEN_ANG       <- WS? '<' WS?
~CLOSE_ANG      <- WS? '>' WS?
~OPEN_BRACE     <- WS? '{' WS?
~CLOSE_BRACE    <- WS? '}' WS?
ADD_OP          <- WS? < [+-] > WS?
MULT_OP         <- WS? < [*/%] > WS?
NEG_OP          <- WS? < '-' > WS?

# Whitespace and comments
~WS             <- ([ \t\r\n]+ / '//' (!EOL .)* EOL / '/*' (!'*/' .)* '*/')+

)";
  return grammar;
}

}  // namespace RosMsgParser
