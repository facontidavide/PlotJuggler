#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "ast.h"
#include "token.h"
#include "types.h"

// Query: the single entry point for parsing and analyzing a query string.
//
// Usage:
//   Query q("robot == \"humanoid\" or \"drone\"", schema);
//   q.valid();           // true — all leaves are complete CompareExprs
//   q.lua();             // "robot == \"humanoid\" or robot == \"drone\"" (expanded)
//   q.tokens();          // vector of positioned tokens
//   q.ast();             // root of the expression tree
//   q.token_at(14);      // token at character offset 14
//
// The Query is immutable after construction. To analyze a new string, create a new Query.
class Query
{
public:
  Query() = default;

  Query(std::string_view source, const Schema& schema)
      : source_(source)
  {
    Lexer lexer(source);
    tokens_ = lexer.tokenize();

    Parser parser(tokens_);
    auto result = parser.parse();
    ast_ = std::move(result.ast);
    complete_ = result.complete;
    parse_error_ = std::move(result.error);

    // If the parse is complete (all leaves are full comparisons), use the
    // expanded serialization. Otherwise fall back to the raw source — the
    // query may be valid Lua that our subset parser doesn't understand
    // (e.g., string.find(), nil comparisons, function calls).
    if (complete_)
    {
      lua_str_ = serialize(ast_.get());
    }
    else
    {
      lua_str_ = source_;
    }
  }

  // The original source string.
  [[nodiscard]] const std::string& source() const { return source_; }

  // The token stream from lexing.
  [[nodiscard]] const std::vector<Token>& tokens() const { return tokens_; }

  // The parsed AST. Null if the input was empty.
  [[nodiscard]] const Expr* ast() const { return ast_.get(); }

  // True if every leaf is a full CompareExpr (no bare keys or partials).
  [[nodiscard]] bool complete() const { return complete_; }

  // Non-empty if parsing failed structurally.
  [[nodiscard]] const std::string& error() const { return parse_error_; }

  // Is there anything to evaluate?
  [[nodiscard]] bool empty() const { return tokens_.empty(); }

  // The Lua-ready expanded string (shorthand expanded, suitable for eval).
  [[nodiscard]] const std::string& lua() const { return lua_str_; }

  // Find the token at a given character offset in the source.
  // Returns nullptr if no token covers that position.
  [[nodiscard]] const Token* token_at(int pos) const
  {
    for (const auto& tok : tokens_)
    {
      if (pos >= tok.start && pos < tok.end) return &tok;
    }
    return nullptr;
  }

  // Find the token index at a given character offset.
  // Returns -1 if no token covers that position.
  [[nodiscard]] int token_index_at(int pos) const
  {
    for (int i = 0; i < static_cast<int>(tokens_.size()); ++i)
    {
      if (pos >= tokens_[i].start && pos < tokens_[i].end) return i;
    }
    return -1;
  }

  // Get the most recent key in context before a given token index.
  // Walks backward through tokens to find the nearest Key token.
  [[nodiscard]] std::string key_before(int token_index) const
  {
    for (int i = token_index - 1; i >= 0; --i)
    {
      if (tokens_[i].type == TokenType::Key) return tokens_[i].text;
    }
    return {};
  }

  // Determine what token type is expected at the given character offset.
  [[nodiscard]] TokenType expected_at(int pos) const
  {
    // Find which token we're at or after.
    int idx = -1;
    for (int i = 0; i < static_cast<int>(tokens_.size()); ++i)
    {
      if (tokens_[i].start <= pos) idx = i;
      else break;
    }

    if (idx < 0) return TokenType::Key;  // empty or before first token

    auto last_type = tokens_[idx].type;

    // If cursor is inside a token, we're editing that token type.
    if (pos < tokens_[idx].end) return last_type;

    // Cursor is after the token — what comes next?
    switch (last_type)
    {
      case TokenType::Key:       return TokenType::Operator;
      case TokenType::Operator:  return TokenType::Value;
      case TokenType::Value:     return TokenType::And;  // connective
      case TokenType::And:
      case TokenType::Or:
      case TokenType::Not:
      case TokenType::OpenParen: return TokenType::Key;
      case TokenType::CloseParen: return TokenType::And;  // connective
    }

    return TokenType::Key;
  }

private:
  std::string source_;
  std::vector<Token> tokens_;
  ExprPtr ast_;
  bool complete_ = false;
  std::string parse_error_;
  std::string lua_str_;
};
