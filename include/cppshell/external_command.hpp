#pragma once

#include "cppshell/command.hpp"

#include <string>
#include <vector>

namespace cppshell {

/** External command runner (unknown commands are executed as processes). */
class ExternalCommand final : public ICommand {
public:
  /** Constructs a runnable external command. */
  ExternalCommand(std::string program, std::vector<std::string> args,
                  Environment envForCommand);

  /** Spawns the external process and waits for completion. */
  [[nodiscard]] CommandResult Execute(CommandContext &context) override;

private:
  std::string program_;
  std::vector<std::string> args_;
  Environment env_;
};

} // namespace cppshell
