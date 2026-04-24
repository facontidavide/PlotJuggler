#pragma once

#include <algorithm>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "types.h"

// What kind of token the cursor is positioned to receive.
enum class Expect
{
  Key,         // a metadata key name
  Operator,    // ==, ~=, <, >, <=, >=
  Value,       // a value for the current key
  Connective,  // and, or, not
  Any,         // beginning of expression or after open paren
};

// Result from the completion engine.
struct Completions
{
  Expect expect = Expect::Any;
  std::string current_key;                // the key in context (for Value completions)
  std::vector<std::string> suggestions;   // valid items to insert
};

// Operators the user can choose between.
inline const std::vector<std::string>& operators()
{
  static const std::vector<std::string> ops = {"==", "~=", "<", ">", "<=", ">="};
  return ops;
}

// Connectives.
inline const std::vector<std::string>& connectives()
{
  static const std::vector<std::string> conns = {"and", "or", "not"};
  return conns;
}

// Simple tokenizer: splits query text into tokens for cursor-context analysis.
// Not a Lua parser — just enough to know what kind of thing goes next.
inline std::vector<std::string> tokenize(std::string_view text)
{
  std::vector<std::string> tokens;
  std::size_t i = 0;

  while (i < text.size())
  {
    // skip whitespace
    if (std::isspace(static_cast<unsigned char>(text[i])))
    {
      ++i;
      continue;
    }

    // two-char operators
    if (i + 1 < text.size())
    {
      auto two = text.substr(i, 2);
      if (two == "==" || two == "~=" || two == "<=" || two == ">=")
      {
        tokens.emplace_back(two);
        i += 2;
        continue;
      }
    }

    // single-char operators and parens
    if (text[i] == '<' || text[i] == '>' || text[i] == '(' || text[i] == ')')
    {
      tokens.emplace_back(1, text[i]);
      ++i;
      continue;
    }

    // quoted string
    if (text[i] == '"' || text[i] == '\'')
    {
      char quote = text[i];
      std::size_t start = i;
      ++i;
      while (i < text.size() && text[i] != quote)
      {
        if (text[i] == '\\') ++i;  // skip escaped char
        ++i;
      }
      if (i < text.size()) ++i;  // consume closing quote
      tokens.emplace_back(text.substr(start, i - start));
      continue;
    }

    // single-char fallback for operator chars that didn't form a two-char op
    if (text[i] == '=' || text[i] == '~')
    {
      tokens.emplace_back(1, text[i]);
      ++i;
      continue;
    }

    // word (identifier, number, connective)
    std::size_t start = i;
    while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i]))
           && text[i] != '(' && text[i] != ')' && text[i] != '=' && text[i] != '~'
           && text[i] != '<' && text[i] != '>' && text[i] != '"' && text[i] != '\'')
    {
      ++i;
    }
    if (i > start)
    {
      tokens.emplace_back(text.substr(start, i - start));
    }
    else
    {
      // Safety: skip any unrecognized character to prevent infinite loop.
      ++i;
    }
  }

  return tokens;
}

// Returns true if the token is an operator.
inline bool is_operator(std::string_view tok)
{
  for (const auto& op : operators())
  {
    if (tok == op) return true;
  }
  return false;
}

// Returns true if the token is a connective.
inline bool is_connective(std::string_view tok)
{
  return tok == "and" || tok == "or" || tok == "not";
}

// Returns true if the token looks like a value (quoted string or number).
inline bool is_value(std::string_view tok)
{
  if (tok.empty()) return false;
  if (tok.front() == '"' || tok.front() == '\'') return true;
  // Try number.
  char* end = nullptr;
  std::strtod(tok.data(), &end);
  return end == tok.data() + tok.size();
}

// Expand shorthand syntax:
//   key op value1 or value2  →  key op value1 or key op value2
//   key op value1 and value2 →  key op value1 and key op value2
//
// Works by tracking the last seen key+op. When a connective is followed
// by a bare value (no key+op), the last key+op is inserted before it.
inline std::string expand(std::string_view text)
{
  auto tokens = tokenize(text);
  if (tokens.size() < 4) return std::string(text);

  std::vector<std::string> out;
  std::string last_key;
  std::string last_op;

  std::size_t i = 0;
  while (i < tokens.size())
  {
    auto& tok = tokens[i];

    // Pattern: identifier operator value
    if (!is_connective(tok) && !is_operator(tok) && !is_value(tok)
        && tok != "(" && tok != ")" && tok != "not")
    {
      // This looks like a key.
      if (i + 1 < tokens.size() && is_operator(tokens[i + 1]))
      {
        last_key = tok;
        last_op = tokens[i + 1];
        out.push_back(tok);
        ++i;
        out.push_back(tokens[i]);  // operator
        ++i;
        continue;
      }
    }

    // Pattern: connective followed by a bare value → insert last_key last_op
    if (is_connective(tok) && tok != "not"
        && !last_key.empty() && !last_op.empty()
        && i + 1 < tokens.size() && is_value(tokens[i + 1]))
    {
      out.push_back(tok);  // "and" / "or"
      ++i;
      out.push_back(last_key);
      out.push_back(last_op);
      out.push_back(tokens[i]);  // the value
      ++i;
      continue;
    }

    out.push_back(tok);
    ++i;
  }

  // Reassemble with spaces.
  std::string result;
  for (std::size_t j = 0; j < out.size(); ++j)
  {
    if (j > 0) result += ' ';
    result += out[j];
  }
  return result;
}

// Compute completions for the given query text at the given cursor position.
//
// The schema provides known keys and their values. The cursor position is
// the character offset where the user's cursor sits (end of text for appending).
[[nodiscard]] inline Completions complete(std::string_view text, std::size_t cursor,
                                          const Schema& schema)
{
  // Only analyze text up to the cursor.
  auto prefix = text.substr(0, std::min(cursor, text.size()));
  auto tokens = tokenize(prefix);

  Completions result;

  // No tokens yet or after open paren — expect a key (or not, or open paren).
  if (tokens.empty())
  {
    result.expect = Expect::Any;
    for (const auto& [key, vals] : schema)
    {
      result.suggestions.push_back(key);
    }
    return result;
  }

  const auto& last = tokens.back();

  // After a connective or open paren — expect a key.
  if (is_connective(last) || last == "(")
  {
    result.expect = Expect::Key;
    for (const auto& [key, vals] : schema)
    {
      result.suggestions.push_back(key);
    }
    return result;
  }

  // After an operator — expect a value. Walk back to find the key.
  if (is_operator(last))
  {
    result.expect = Expect::Value;
    // The key should be the token before the operator.
    if (tokens.size() >= 2)
    {
      result.current_key = tokens[tokens.size() - 2];
      auto it = schema.find(result.current_key);
      if (it != schema.end())
      {
        result.suggestions = it->second;
      }
    }
    return result;
  }

  // After a value or closing paren — expect a connective.
  // Heuristic: if the token two positions back is a key and one position back
  // is an operator, then the last token is a value.
  if (tokens.size() >= 3 && is_operator(tokens[tokens.size() - 2]))
  {
    result.expect = Expect::Connective;
    result.suggestions = {"and", "or"};
    return result;
  }

  if (last == ")")
  {
    result.expect = Expect::Connective;
    result.suggestions = {"and", "or"};
    return result;
  }

  // After a key (identifier that's not a connective and not an operator) — expect operator.
  if (!is_connective(last) && !is_operator(last))
  {
    // Could be a partial key being typed, or a complete key waiting for operator.
    // Check if the last token is a known key.
    auto it = schema.find(last);
    if (it != schema.end())
    {
      result.expect = Expect::Operator;
      result.current_key = last;
      result.suggestions = {"==", "~=", "<", ">", "<=", ">="};
      return result;
    }

    // Partial key — filter keys by prefix.
    result.expect = Expect::Key;
    for (const auto& [key, vals] : schema)
    {
      if (key.compare(0, last.size(), last) == 0)
      {
        result.suggestions.push_back(key);
      }
    }
    return result;
  }

  result.expect = Expect::Any;
  return result;
}
