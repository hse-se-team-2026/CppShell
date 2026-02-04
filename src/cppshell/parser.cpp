#include "cppshell/parser.hpp"

#include "cppshell/tokenizer.hpp"

#include <cctype>

namespace cppshell {

namespace {

[[nodiscard]] bool IsNameStart(char c) {
  return (std::isalpha(static_cast<unsigned char>(c)) != 0) || c == '_';
}

[[nodiscard]] bool IsNameChar(char c) {
  return (std::isalnum(static_cast<unsigned char>(c)) != 0) || c == '_';
}

[[nodiscard]] bool IsValidEnvName(const std::string& s) {
  if (s.empty() || !IsNameStart(s[0])) {
    return false;
  }
  for (size_t i = 1; i < s.size(); ++i) {
    if (!IsNameChar(s[i])) {
      return false;
    }
  }
  return true;
}

}  // namespace

ParseResult ParseLine(std::string_view input) {
  ParseResult result;

  const TokenizeResult tok = Tokenize(input);
  if (!tok.Ok()) {
    result.error = tok.error;
    return result;
  }

  if (tok.tokens.empty()) {
    result.line = std::nullopt;
    return result;
  }

  ParsedLine parsed;
  size_t i = 0;

  // Leading assignments: NAME=value
  for (; i < tok.tokens.size(); ++i) {
    const std::string& t = tok.tokens[i];
    const size_t eq = t.find('=');
    if (eq == std::string::npos || eq == 0) {
      break;
    }

    const std::string name = t.substr(0, eq);
    const std::string value = t.substr(eq + 1);
    if (!IsValidEnvName(name)) {
      break;
    }

    parsed.assignments.emplace(name, value);
  }

  if (i >= tok.tokens.size()) {
    // Only assignments.
    parsed.command.clear();
    parsed.args.clear();
    result.line = std::move(parsed);
    return result;
  }

  parsed.command = tok.tokens[i];
  for (size_t j = i + 1; j < tok.tokens.size(); ++j) {
    parsed.args.push_back(tok.tokens[j]);
  }

  result.line = std::move(parsed);
  return result;
}

}  // namespace cppshell
