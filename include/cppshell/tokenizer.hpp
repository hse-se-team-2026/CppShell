#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace cppshell {

/** Result of tokenizing a command line. */
struct TokenizeResult {
  /** Token list, if tokenization succeeded. */
  std::vector<std::string> tokens;
  /** Human-readable error; empty if successful. */
  std::string error;

  /** Returns true if tokenization succeeded. */
  [[nodiscard]] bool Ok() const { return error.empty(); }
};

/**
 * Splits a command line into tokens.
 *
 * Rules (for this homework stage):
 * - Whitespace separates tokens.
 * - Single and double quotes both group text into a single token.
 * - Quotes are removed from the resulting token.
 * - No variable substitution and no pipes are handled here.
 */
[[nodiscard]] TokenizeResult Tokenize(std::string_view line);

} // namespace cppshell
