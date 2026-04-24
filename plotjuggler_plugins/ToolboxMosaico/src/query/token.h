#pragma once

#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

enum class TokenType
{
  Key,         // metadata key identifier (e.g., robot, sensor_type)
  Operator,    // ==  ~=  <  >  <=  >=
  Value,       // string literal "..." or '...' or numeric literal
  And,         // and
  Or,          // or
  Not,         // not
  OpenParen,   // (
  CloseParen,  // )
};

struct Token
{
  TokenType type;
  std::string text;  // raw source text of this token
  int start = 0;     // byte offset in source (inclusive)
  int end = 0;       // byte offset in source (exclusive)
};

// Lexer: turns a query string into a sequence of typed, positioned tokens.
//
// Classification rules:
//   - "and", "or", "not" → And/Or/Not
//   - ==, ~=, <, >, <=, >= → Operator
//   - Quoted strings ("..." or '...') → Value
//   - Bare numbers (parseable by strtod) → Value
//   - ( → OpenParen, ) → CloseParen
//   - Everything else → Key
//
// Unrecognized characters are skipped (no crash, no infinite loop).
class Lexer
{
public:
  explicit Lexer(std::string_view source) : src_(source) {}

  [[nodiscard]] std::vector<Token> tokenize() const
  {
    std::vector<Token> tokens;
    int i = 0;
    int len = static_cast<int>(src_.size());

    while (i < len)
    {
      // Skip whitespace.
      if (is_space(i)) { ++i; continue; }

      // Two-char operators.
      if (i + 1 < len)
      {
        auto two = src_.substr(static_cast<std::size_t>(i), 2);
        if (two == "==" || two == "~=" || two == "<=" || two == ">=")
        {
          tokens.push_back({TokenType::Operator, std::string(two), i, i + 2});
          i += 2;
          continue;
        }
      }

      // Single-char operators.
      if (src_[i] == '<' || src_[i] == '>')
      {
        tokens.push_back({TokenType::Operator, std::string(1, src_[i]), i, i + 1});
        ++i;
        continue;
      }

      // Parens.
      if (src_[i] == '(')
      {
        tokens.push_back({TokenType::OpenParen, "(", i, i + 1});
        ++i;
        continue;
      }
      if (src_[i] == ')')
      {
        tokens.push_back({TokenType::CloseParen, ")", i, i + 1});
        ++i;
        continue;
      }

      // Quoted string.
      if (src_[i] == '"' || src_[i] == '\'')
      {
        int start = i;
        char quote = src_[i];
        ++i;
        while (i < len && src_[i] != quote)
        {
          if (src_[i] == '\\' && i + 1 < len) ++i; // skip escaped
          ++i;
        }
        if (i < len) ++i; // consume closing quote
        tokens.push_back({TokenType::Value, std::string(src_.substr(start, i - start)), start, i});
        continue;
      }

      // Lone = or ~ (not part of a two-char op).
      if (src_[i] == '=' || src_[i] == '~')
      {
        tokens.push_back({TokenType::Operator, std::string(1, src_[i]), i, i + 1});
        ++i;
        continue;
      }

      // Word: identifier, number, or keyword.
      int start = i;
      while (i < len && !is_space(i) && !is_punct(i))
      {
        ++i;
      }

      if (i > start)
      {
        auto word = std::string(src_.substr(start, i - start));
        tokens.push_back({classify_word(word), std::move(word), start, i});
        continue;
      }

      // Unrecognized character — skip to prevent infinite loop.
      ++i;
    }

    return tokens;
  }

private:
  [[nodiscard]] bool is_space(int i) const
  {
    return std::isspace(static_cast<unsigned char>(src_[i]));
  }

  [[nodiscard]] bool is_punct(int i) const
  {
    char c = src_[i];
    return c == '(' || c == ')' || c == '=' || c == '~'
        || c == '<' || c == '>' || c == '"' || c == '\'';
  }

  [[nodiscard]] static TokenType classify_word(const std::string& word)
  {
    if (word == "and") return TokenType::And;
    if (word == "or") return TokenType::Or;
    if (word == "not") return TokenType::Not;

    // Check if it's a number.
    char* end = nullptr;
    std::strtod(word.c_str(), &end);
    if (end == word.c_str() + word.size()) return TokenType::Value;

    return TokenType::Key;
  }

  std::string_view src_;
};
