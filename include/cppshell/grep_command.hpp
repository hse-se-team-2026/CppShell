#pragma once

#include "cppshell/command.hpp"

#include <string>
#include <vector>

namespace cppshell {

/** Builtin: grep. */
class GrepCommand final : public ICommand {
public:
  /** Constructs the command with its argv (excluding the command name). */
  explicit GrepCommand(std::vector<std::string> args);

  /** Executes grep logic. */
  [[nodiscard]] CommandResult Execute(CommandContext &context) override;

private:
  std::vector<std::string> args_;
};

} // namespace cppshell
