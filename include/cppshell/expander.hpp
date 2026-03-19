#pragma once

#include "cppshell/environment.hpp"
#include <string>

namespace cppshell {

/**
 * Expands variables and expressions in the input string.
 *
 * Supported expansions:
 * - $VAR
 * - ${VAR}
 * - $((expression)) (basic arithmetic)
 *
 * Handles quoting:
 * - Single quotes '...' prevent usage of special characters.
 * - Double quotes "..." allow substitution.
 */
/**
 * @brief Expands variables and arithmetic expressions in the input string.
 *
 * This function handles:
 * - Variable substitution: $VAR, ${VAR}
 * - Arithmetic expansion: $((expression)) (basic support)
 * - Quoting:
 *   - Single quotes ('...') disable all expansions.
 *   - Double quotes ("...") allow variable expansion but preserve most
 * literals.
 * - Escaping:
 *   - Backslash (\) escapes the next character (e.g., \$ -> $, \\ -> \).
 *
 * @param input The input string to expand.
 * @param env The environment containing variable values.
 * @return The string with all expansions performed.
 */
std::string Expand(const std::string &input, const Environment &env);

} // namespace cppshell
