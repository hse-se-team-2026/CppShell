#include "cppshell/expander.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>

namespace cppshell {

namespace {

bool IsMsgChar(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

std::string EvaluateArithmetic(const std::string &expr) {
  try {
    std::string cleanExpr;
    for (char c : expr) {
      if (!std::isspace(static_cast<unsigned char>(c))) {
        cleanExpr += c;
      }
    }
    if (cleanExpr.empty())
      return "0";
    return cleanExpr;
  } catch (...) {
    return "0";
  }
}

} // namespace

std::string Expand(const std::string &input, const Environment &env) {
  std::string result;
  bool insideSingle = false;
  bool insideDouble = false;
  bool escaped = false;

  for (size_t i = 0; i < input.size(); ++i) {
    char c = input[i];

    if (escaped) {
      result.push_back(c);
      escaped = false;
      continue;
    }

    if (c == '\\') {
      if (insideSingle) {
        result.push_back(c);
      } else {
        if (i + 1 < input.size()) {
          char next = input[i + 1];
          if (next == '$') {
            result.push_back(next);
            i++;
            continue;
          }
          if (next == '\\') {
            result.push_back(next);
            i++;
            continue;
          }
          result.push_back(c);
          escaped = true;
        } else {
          result.push_back(c);
        }
      }
      continue;
    }

    if (c == '\'') {
      if (!insideDouble) {
        insideSingle = !insideSingle;
      }
      result.push_back(c);
      continue;
    }

    if (c == '"') {
      if (!insideSingle) {
        insideDouble = !insideDouble;
      }
      result.push_back(c);
      continue;
    }

    if (c == '$' && !insideSingle) {
      if (i + 1 >= input.size()) {
        result.push_back(c);
        continue;
      }

      if (input[i + 1] == '(' && i + 2 < input.size() && input[i + 2] == '(') {
        size_t startExpr = i + 3;
        size_t endExpr = input.find("))", startExpr);
        if (endExpr == std::string::npos) {
          result.push_back(c);
          continue;
        }
        std::string expr = input.substr(startExpr, endExpr - startExpr);
        result += EvaluateArithmetic(expr);
        i = endExpr + 1;
        continue;
      }

      if (input[i + 1] == '{') {
        size_t endBrace = input.find('}', i + 2);
        if (endBrace != std::string::npos) {
          std::string varName = input.substr(i + 2, endBrace - (i + 2));
          result += env.Get(varName);
          i = endBrace;
          continue;
        }
      }

      size_t j = i + 1;
      if (j < input.size() && (std::isalpha(input[j]) || input[j] == '_')) {
        std::string varName;
        while (j < input.size() && IsMsgChar(input[j])) {
          varName.push_back(input[j]);
          j++;
        }
        result += env.Get(varName);
        i = j - 1;
        continue;
      }

      result.push_back(c);
      continue;
    }

    result.push_back(c);
  }
  return result;
}

} // namespace cppshell
