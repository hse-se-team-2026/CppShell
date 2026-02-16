#include "cppshell/tokenizer.hpp"

#include <cctype>

namespace cppshell {

TokenizeResult Tokenize(std::string_view line) {
  TokenizeResult result;
  std::string current;
  char quote = '\0';
  bool escaped = false; // Add state for escape

  auto Flush = [&]() {
    if (!current.empty()) {
      result.tokens.push_back(current);
      current.clear();
    }
  };

  for (size_t i = 0; i < line.size(); ++i) {
    const char ch = line[i];

    if (escaped) {
      // Append escaped character literally.
      current.push_back(ch);
      escaped = false;
      continue;
    }

    if (ch == '\\') {
      // Handle escape character. The next character will be treated literally.
      escaped = true;
      continue;
    }

    if (quote == '\0') {
      if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
        Flush();
        continue;
      }

      if (ch == '|') {
        Flush();
        result.tokens.emplace_back("|");
        continue;
      }

      if (ch == '\'' || ch == '"') {
        quote = ch;
        continue;
      }

      current.push_back(ch);
      continue;
    }

    // Inside quotes.
    if (ch == quote) {
      quote = '\0';
      continue;
    }

    current.push_back(ch);
  }

  if (quote != '\0') {
    result.error = "Unterminated quote";
    return result;
  }

  if (escaped) {
    // Trailing backslash is treated as an error (e.g., unexpected EOF).
    result.error = "Trailing backslash";
    return result;
  }

  Flush();
  return result;
}

} // namespace cppshell
