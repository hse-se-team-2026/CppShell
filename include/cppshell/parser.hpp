#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cppshell {

struct Command {
  /** Environment assignments like NAME=value, appearing before the command. */
  std::unordered_map<std::string, std::string> assignments;
  /** Command name, or empty if the command contained only assignments. */
  std::string command;
  /** Command arguments, excluding the command name. */
  std::vector<std::string> args;
};

/** Parsed representation of a command pipeline. */
struct Pipeline {
  std::vector<Command> commands;
};

/** Result of parsing a line into assignments + command + args. */
struct ParseResult {
  /** Parsed pipeline, if parsing succeeded and the input wasn't empty. */
  std::optional<Pipeline> pipeline;
  /** Human-readable error; empty if successful. */
  std::string error;

  /** Returns true if parsing succeeded. */
  [[nodiscard]] bool Ok() const { return error.empty(); }
};

/**
 * Parses an input line.
 *
 * This stage supports:
 * - tokenization with quotes;
 * - leading NAME=value assignments;
 * - a single command invocation.
 */
[[nodiscard]] ParseResult ParseLine(std::string_view input);

} // namespace cppshell
