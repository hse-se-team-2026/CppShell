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

[[nodiscard]] bool IsValidEnvName(const std::string &s) {
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

} // namespace

ParseResult ParseLine(std::string_view input) {
  ParseResult result;

  const TokenizeResult tok = Tokenize(input);
  if (!tok.Ok()) {
    result.error = tok.error;
    return result;
  }

  if (tok.tokens.empty()) {
    result.pipeline = std::nullopt;
    return result;
  }

  Pipeline pipeline;
  size_t start = 0;

  while (start < tok.tokens.size()) {
    // Find next pipe or end
    size_t end = start;
    while (end < tok.tokens.size() && tok.tokens[end] != "|") {
      end++;
    }

    // Process segment [start, end)
    Command cmd;
    size_t i = start;

    // Leading assignments: NAME=value
    for (; i < end; ++i) {
      const std::string &t = tok.tokens[i];
      const size_t eq = t.find('=');
      if (eq == std::string::npos || eq == 0) {
        break;
      }

      const std::string name = t.substr(0, eq);
      const std::string value = t.substr(eq + 1);
      if (!IsValidEnvName(name)) {
        break;
      }

      cmd.assignments.emplace(name, value);
    }

    if (i < end) {
      cmd.command = tok.tokens[i];
      for (size_t j = i + 1; j < end; ++j) {
        cmd.args.push_back(tok.tokens[j]);
      }
    }

    pipeline.commands.push_back(std::move(cmd));

    // skip pipe
    start = end + 1;
  }

  result.pipeline = std::move(pipeline);
  return result;
}

} // namespace cppshell
