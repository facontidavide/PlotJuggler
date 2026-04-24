/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "token.h"

// --- AST node types ---

enum class NodeType
{
  Compare,  // key op value       (e.g., robot == "humanoid")
  Binary,   // left and/or right  (e.g., A and B)
  Not,      // not expr           (e.g., not (X))
  Group,    // ( expr )
  Key,      // bare key, incomplete clause (e.g., robot)
  Partial,  // key op, missing value (e.g., robot ==)
};

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

// Base expression node. All AST nodes carry their source range.
struct Expr
{
  NodeType type;
  int start = 0;  // source offset of first token
  int end = 0;    // source offset past last token

  explicit Expr(NodeType t) : type(t)
  {
  }
  virtual ~Expr() = default;

  Expr(const Expr&) = delete;
  Expr& operator=(const Expr&) = delete;
  Expr(Expr&&) = default;
  Expr& operator=(Expr&&) = default;
};

// key op value
struct CompareExpr : Expr
{
  Token key;
  Token op;
  Token value;

  CompareExpr(Token k, Token o, Token v)
    : Expr(NodeType::Compare), key(std::move(k)), op(std::move(o)), value(std::move(v))
  {
    start = key.start;
    end = value.end;
  }
};

// left (and|or) right
struct BinaryExpr : Expr
{
  ExprPtr left;
  Token connective;  // the "and" / "or" token
  ExprPtr right;

  BinaryExpr(ExprPtr l, Token conn, ExprPtr r)
    : Expr(NodeType::Binary), left(std::move(l)), connective(std::move(conn)), right(std::move(r))
  {
    start = left->start;
    end = right->end;
  }
};

// not expr
struct NotExpr : Expr
{
  Token not_token;
  ExprPtr operand;

  NotExpr(Token nt, ExprPtr op)
    : Expr(NodeType::Not), not_token(std::move(nt)), operand(std::move(op))
  {
    start = not_token.start;
    end = operand->end;
  }
};

// ( expr )
struct GroupExpr : Expr
{
  Token open;
  ExprPtr inner;
  Token close;

  GroupExpr(Token o, ExprPtr inner_expr, Token c)
    : Expr(NodeType::Group), open(std::move(o)), inner(std::move(inner_expr)), close(std::move(c))
  {
    start = open.start;
    end = close.end;
  }
};

// Bare key — incomplete clause (user is still typing).
struct KeyExpr : Expr
{
  Token key;

  explicit KeyExpr(Token k) : Expr(NodeType::Key), key(std::move(k))
  {
    start = key.start;
    end = key.end;
  }
};

// key op — missing value (user is still typing).
struct PartialExpr : Expr
{
  Token key;
  Token op;

  PartialExpr(Token k, Token o) : Expr(NodeType::Partial), key(std::move(k)), op(std::move(o))
  {
    start = key.start;
    end = op.end;
  }
};

// --- Parser ---
//
// Parses a token stream into an AST. Handles:
//   - Full clauses: key op value
//   - Partial clauses: bare key, key op (missing value)
//   - Connectives: and, or (left-to-right, or has lower precedence than and)
//   - Not: not expr
//   - Groups: ( expr )
//   - Shorthand: key op val1 or val2 → key op val1 or key op val2
//     (expanded during parsing by tracking last key+op)
//
// The parser never throws. Malformed input produces partial/key nodes.

struct ParseResult
{
  ExprPtr ast;
  bool complete = false;  // true if every leaf is a full CompareExpr
  std::string error;      // non-empty if structurally broken
};

class Parser
{
public:
  explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens))
  {
  }

  [[nodiscard]] ParseResult parse()
  {
    if (tokens_.empty())
    {
      return { nullptr, false, "empty" };
    }

    pos_ = 0;
    auto expr = parse_or();

    // If there are leftover tokens, the parse is incomplete.
    bool leftover = pos_ < static_cast<int>(tokens_.size());

    bool all_complete = expr && check_complete(expr.get());

    return { std::move(expr), all_complete && !leftover, leftover ? "unexpected tokens" : "" };
  }

private:
  // or-level: lowest precedence
  ExprPtr parse_or()
  {
    auto left = parse_and();
    if (!left)
    {
      return nullptr;
    }

    while (pos_ < size() && tokens_[pos_].type == TokenType::Or)
    {
      auto conn = tokens_[pos_++];
      auto right = parse_and();
      if (!right)
      {
        // "A or" with nothing after — still return what we have.
        break;
      }
      left = std::make_unique<BinaryExpr>(std::move(left), std::move(conn), std::move(right));
    }

    return left;
  }

  // and-level: higher precedence than or
  ExprPtr parse_and()
  {
    auto left = parse_unary();
    if (!left)
    {
      return nullptr;
    }

    while (pos_ < size() && tokens_[pos_].type == TokenType::And)
    {
      auto conn = tokens_[pos_++];
      auto right = parse_unary();
      if (!right)
      {
        break;
      }
      left = std::make_unique<BinaryExpr>(std::move(left), std::move(conn), std::move(right));
    }

    return left;
  }

  // not-level
  ExprPtr parse_unary()
  {
    if (pos_ < size() && tokens_[pos_].type == TokenType::Not)
    {
      auto not_tok = tokens_[pos_++];
      auto operand = parse_unary();
      if (!operand)
      {
        return nullptr;
      }
      return std::make_unique<NotExpr>(std::move(not_tok), std::move(operand));
    }

    return parse_primary();
  }

  // Primary: group, comparison, shorthand value, or bare key
  ExprPtr parse_primary()
  {
    if (pos_ >= size())
    {
      return nullptr;
    }

    // Grouped expression: ( expr )
    if (tokens_[pos_].type == TokenType::OpenParen)
    {
      auto open = tokens_[pos_++];
      auto inner = parse_or();
      if (pos_ < size() && tokens_[pos_].type == TokenType::CloseParen)
      {
        auto close = tokens_[pos_++];
        if (!inner)
        {
          // Empty parens — create a group with no inner.
          return std::make_unique<GroupExpr>(std::move(open), nullptr, std::move(close));
        }
        return std::make_unique<GroupExpr>(std::move(open), std::move(inner), std::move(close));
      }
      // Unclosed paren — return inner or null.
      return inner;
    }

    // Shorthand value: a bare value after connective inherits last_key + last_op.
    // e.g., robot == "a" or "b" → the "b" is parsed here as a shorthand compare.
    if (tokens_[pos_].type == TokenType::Value && !last_key_.text.empty())
    {
      auto val = tokens_[pos_++];
      return std::make_unique<CompareExpr>(last_key_, last_op_, std::move(val));
    }

    // Key — start of a comparison or bare key.
    if (tokens_[pos_].type == TokenType::Key)
    {
      auto key = tokens_[pos_++];

      // Key + operator?
      if (pos_ < size() && tokens_[pos_].type == TokenType::Operator)
      {
        auto op = tokens_[pos_++];

        // Key + operator + value?
        if (pos_ < size() && tokens_[pos_].type == TokenType::Value)
        {
          auto val = tokens_[pos_++];

          // Remember for shorthand expansion.
          last_key_ = key;
          last_op_ = op;

          return std::make_unique<CompareExpr>(std::move(key), std::move(op), std::move(val));
        }

        // Key + operator, no value yet (partial).
        last_key_ = key;
        last_op_ = op;
        return std::make_unique<PartialExpr>(std::move(key), std::move(op));
      }

      // Bare key.
      return std::make_unique<KeyExpr>(std::move(key));
    }

    // Unexpected token — skip it to avoid infinite loop.
    ++pos_;
    return nullptr;
  }

  [[nodiscard]] static bool check_complete(const Expr* expr)
  {
    if (!expr)
    {
      return false;
    }
    switch (expr->type)
    {
      case NodeType::Compare:
        return true;
      case NodeType::Binary: {
        auto* b = static_cast<const BinaryExpr*>(expr);
        return check_complete(b->left.get()) && check_complete(b->right.get());
      }
      case NodeType::Not:
        return check_complete(static_cast<const NotExpr*>(expr)->operand.get());
      case NodeType::Group:
        return check_complete(static_cast<const GroupExpr*>(expr)->inner.get());
      case NodeType::Key:
      case NodeType::Partial:
        return false;
    }
    return false;
  }

  [[nodiscard]] int size() const
  {
    return static_cast<int>(tokens_.size());
  }

  std::vector<Token> tokens_;
  int pos_ = 0;

  // Last seen key+op for shorthand expansion.
  Token last_key_;
  Token last_op_;
};

// --- Helpers ---

// Serialize an AST back to a Lua-compatible string (with shorthand expanded).
inline std::string serialize(const Expr* expr)
{
  if (!expr)
  {
    return {};
  }

  switch (expr->type)
  {
    case NodeType::Compare: {
      auto* c = static_cast<const CompareExpr*>(expr);
      return c->key.text + " " + c->op.text + " " + c->value.text;
    }
    case NodeType::Binary: {
      auto* b = static_cast<const BinaryExpr*>(expr);
      return serialize(b->left.get()) + " " + b->connective.text + " " + serialize(b->right.get());
    }
    case NodeType::Not: {
      auto* n = static_cast<const NotExpr*>(expr);
      return "not " + serialize(n->operand.get());
    }
    case NodeType::Group: {
      auto* g = static_cast<const GroupExpr*>(expr);
      return "(" + serialize(g->inner.get()) + ")";
    }
    case NodeType::Key:
      return static_cast<const KeyExpr*>(expr)->key.text;
    case NodeType::Partial: {
      auto* p = static_cast<const PartialExpr*>(expr);
      return p->key.text + " " + p->op.text;
    }
  }
  return {};
}
