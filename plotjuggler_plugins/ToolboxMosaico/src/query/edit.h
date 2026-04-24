#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "complete.h"
#include "token.h"
#include "types.h"

// What a dropdown action would do at this cursor position.
enum class Action
{
  Disabled,  // dropdown should be greyed out
  Insert,    // append new text at cursor (no existing token to replace)
  Replace,   // overwrite the token the cursor is on
};

// Full analysis of what the cursor position means for the UI.
// Computed once per cursor move, used by all three dropdowns.
struct CursorContext
{
  int cursor = 0;

  // The token the cursor is on or touching.
  // -1 if cursor is in whitespace between tokens.
  int token_index = -1;
  Token active_token;  // valid only when token_index >= 0

  // From complete() on the prefix up to cursor.
  Expect expect = Expect::Any;
  std::string context_key;
  int suggestion_count = 0;

  // What each dropdown would do.
  Action key_action = Action::Disabled;
  Action op_action = Action::Disabled;
  Action val_action = Action::Disabled;

  // Convenience: is the dropdown usable at all?
  [[nodiscard]] bool can_pick_key() const { return key_action != Action::Disabled; }
  [[nodiscard]] bool can_pick_op() const { return op_action != Action::Disabled; }
  [[nodiscard]] bool can_pick_value() const { return val_action != Action::Disabled; }
};

// Find the token whose span touches the cursor.
// Cursor at position P is "on" a token if P is in [start, end].
// This means the cursor right after a token (P == end) is still on it —
// the common case when the user just finished typing a word.
// Returns token index, or -1 if cursor is in whitespace.
[[nodiscard]] inline int find_active_token(const std::vector<Token>& tokens, int cursor)
{
  for (int i = 0; i < static_cast<int>(tokens.size()); ++i)
  {
    if (cursor >= tokens[i].start && cursor <= tokens[i].end)
      return i;
  }
  return -1;
}

// Compute the full cursor context for a position in the query text.
[[nodiscard]] inline CursorContext analyze(std::string_view text, int cursor,
                                           const Schema& schema)
{
  CursorContext ctx;
  ctx.cursor = cursor;

  // Tokenize the FULL text to find what token the cursor is on.
  Lexer lex(text);
  auto tokens = lex.tokenize();

  ctx.token_index = find_active_token(tokens, cursor);
  if (ctx.token_index >= 0)
  {
    ctx.active_token = tokens[ctx.token_index];
  }

  // Run complete() on the prefix for baseline analysis.
  auto comp = complete(text, static_cast<std::size_t>(cursor), schema);
  ctx.expect = comp.expect;
  ctx.context_key = comp.current_key;
  ctx.suggestion_count = static_cast<int>(comp.suggestions.size());

  // Override expect when cursor is on a known token.
  // complete() only sees the prefix ("rob" → partial key → Expect::Key),
  // but the Lexer sees the full token ("robot"). When the cursor is on a
  // token, expect should reflect what comes AFTER that token type.
  if (ctx.token_index >= 0)
  {
    switch (ctx.active_token.type)
    {
      case TokenType::Key:
        ctx.expect = Expect::Operator;
        ctx.context_key = ctx.active_token.text;
        break;
      case TokenType::Operator:
        ctx.expect = Expect::Value;
        if (ctx.token_index > 0
            && tokens[ctx.token_index - 1].type == TokenType::Key)
          ctx.context_key = tokens[ctx.token_index - 1].text;
        break;
      case TokenType::Value:
      case TokenType::CloseParen:
        ctx.expect = Expect::Connective;
        if (ctx.token_index >= 2
            && tokens[ctx.token_index - 1].type == TokenType::Operator
            && tokens[ctx.token_index - 2].type == TokenType::Key)
          ctx.context_key = tokens[ctx.token_index - 2].text;
        break;
      case TokenType::And:
      case TokenType::Or:
      case TokenType::Not:
      case TokenType::OpenParen:
        ctx.expect = Expect::Key;
        ctx.context_key.clear();
        break;
    }
  }

  // --- Derive actions ---
  // Replace: cursor is on an existing token of the matching type.
  // Insert:  expect says this token type goes next.
  // Both can be true (cursor on key → key=Replace AND op=Insert).

  bool on_key   = ctx.token_index >= 0 && ctx.active_token.type == TokenType::Key;
  bool on_op    = ctx.token_index >= 0 && ctx.active_token.type == TokenType::Operator;
  bool on_value = ctx.token_index >= 0 && ctx.active_token.type == TokenType::Value;

  // Key dropdown:
  //   Replace: cursor is on a Key token
  //   Insert:  expect is Key, Any, or Connective (auto-chain)
  if (on_key)
    ctx.key_action = Action::Replace;
  else if (ctx.expect == Expect::Key || ctx.expect == Expect::Any
           || ctx.expect == Expect::Connective)
    ctx.key_action = Action::Insert;

  // Op dropdown:
  //   Replace: cursor is on an Operator token
  //   Insert:  expect is Operator (i.e., cursor on a key → op next)
  if (on_op)
    ctx.op_action = Action::Replace;
  else if (ctx.expect == Expect::Operator)
    ctx.op_action = Action::Insert;

  // Value dropdown:
  //   Replace: cursor is on a Value token
  //   Insert:  expect is Value (i.e., cursor on an operator → value next)
  if (on_value)
    ctx.val_action = Action::Replace;
  else if (ctx.expect == Expect::Value)
    ctx.val_action = Action::Insert;

  return ctx;
}

// --- String formatting for debug display ---

[[nodiscard]] inline const char* action_str(Action a)
{
  switch (a)
  {
    case Action::Disabled: return "\xe2\x80\x94";  // em-dash
    case Action::Insert:   return "INSERT";
    case Action::Replace:  return "REPLACE";
  }
  return "?";
}

[[nodiscard]] inline const char* expect_str(Expect e)
{
  switch (e)
  {
    case Expect::Key:        return "Key";
    case Expect::Operator:   return "Operator";
    case Expect::Value:      return "Value";
    case Expect::Connective: return "Connective";
    case Expect::Any:        return "Any";
  }
  return "?";
}

[[nodiscard]] inline const char* token_type_str(TokenType t)
{
  switch (t)
  {
    case TokenType::Key:        return "Key";
    case TokenType::Operator:   return "Op";
    case TokenType::Value:      return "Val";
    case TokenType::And:        return "And";
    case TokenType::Or:         return "Or";
    case TokenType::Not:        return "Not";
    case TokenType::OpenParen:  return "(";
    case TokenType::CloseParen: return ")";
  }
  return "?";
}

// Format the cursor context as a two-line debug string.
[[nodiscard]] inline std::string format_debug(const CursorContext& ctx)
{
  std::string line1 = "pos:" + std::to_string(ctx.cursor);

  if (ctx.token_index >= 0)
  {
    line1 += "  active:\"" + ctx.active_token.text + "\" "
             + token_type_str(ctx.active_token.type)
             + "[" + std::to_string(ctx.active_token.start)
             + "," + std::to_string(ctx.active_token.end) + ")";
  }
  else
  {
    line1 += "  active:(none)";
  }

  line1 += "  expect:" + std::string(expect_str(ctx.expect));

  if (!ctx.context_key.empty())
    line1 += "  ctx_key:" + ctx.context_key;

  line1 += "  sugg:" + std::to_string(ctx.suggestion_count);

  std::string line2 = "key:" + std::string(action_str(ctx.key_action));
  if (ctx.key_action == Action::Replace)
    line2 += "[" + std::to_string(ctx.active_token.start)
             + "," + std::to_string(ctx.active_token.end) + ")";

  line2 += "  op:" + std::string(action_str(ctx.op_action));
  if (ctx.op_action == Action::Replace)
    line2 += "[" + std::to_string(ctx.active_token.start)
             + "," + std::to_string(ctx.active_token.end) + ")";

  line2 += "  val:" + std::string(action_str(ctx.val_action));
  if (ctx.val_action == Action::Replace)
    line2 += "[" + std::to_string(ctx.active_token.start)
             + "," + std::to_string(ctx.active_token.end) + ")";

  return line1 + "\n" + line2;
}
