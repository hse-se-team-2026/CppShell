#include "cppshell/expander.hpp"

#include <cctype>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>

namespace cppshell {

namespace {

bool IsMsgChar(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

long ApplyOp(long a, long b, char op) {
  switch (op) {
  case '+':
    return a + b;
  case '-':
    return a - b;
  case '*':
    return a * b;
  case '/':
    return b ? a / b : 0; // Avoid division by zero
  }
  return 0;
}

int Precedence(char op) {
  if (op == '+' || op == '-')
    return 1;
  if (op == '*' || op == '/')
    return 2;
  return 0;
}

std::string EvaluateArithmetic(const std::string &expr) {
  std::stack<long> values;
  std::stack<char> ops;

  for (size_t i = 0; i < expr.length(); i++) {
    if (std::isspace(expr[i]))
      continue;

    if (std::isdigit(expr[i])) {
      long val = 0;
      while (i < expr.length() && std::isdigit(expr[i])) {
        val = (val * 10) + (expr[i] - '0');
        i++;
      }
      values.push(val);
      i--;
    } else if (expr[i] == '(') {
      ops.push('(');
    } else if (expr[i] == ')') {
      while (!ops.empty() && ops.top() != '(') {
        if (values.size() < 2)
          return "0";
        long val2 = values.top();
        values.pop();
        long val1 = values.top();
        values.pop();
        char op = ops.top();
        ops.pop();
        values.push(ApplyOp(val1, val2, op));
      }
      if (!ops.empty())
        ops.pop();
    } else if (expr[i] == '+' || expr[i] == '-' || expr[i] == '*' ||
               expr[i] == '/') {
      while (!ops.empty() && Precedence(ops.top()) >= Precedence(expr[i])) {
        if (values.size() < 2)
          break;
        long val2 = values.top();
        values.pop();
        long val1 = values.top();
        values.pop();
        char op = ops.top();
        ops.pop();
        values.push(ApplyOp(val1, val2, op));
      }
      ops.push(expr[i]);
    }
  }

  while (!ops.empty()) {
    if (values.size() < 2)
      break;
    long val2 = values.top();
    values.pop();
    long val1 = values.top();
    values.pop();
    char op = ops.top();
    ops.pop();
    values.push(ApplyOp(val1, val2, op));
  }

  if (values.empty())
    return "0";
  return std::to_string(values.top());
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
          if (insideDouble && next == '"') {
            result.push_back(next);
            i++;
            continue;
          }
          // If outside double quotes, backslash escapes everything
          if (!insideDouble) {
            result.push_back(next);
            i++;
            continue;
          }

          // Inside double quotes, other backslashes are literal
          result.push_back(c);
          // And next char will be processed normally
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
        // Recursively expand variables inside expression
        std::string expandedExpr = Expand(expr, env);
        result += EvaluateArithmetic(expandedExpr);
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
