#include "cppshell/command.hpp"

#include "cppshell/builtins.hpp"
#include "cppshell/external_command.hpp"

namespace cppshell {

std::unique_ptr<ICommand> CommandFactory::Create(
    const std::string& name, const std::vector<std::string>& args,
    const Environment& envForCommand) const {
  if (name == "echo") {
    return std::make_unique<EchoCommand>(args);
  }
  if (name == "pwd") {
    return std::make_unique<PwdCommand>(args);
  }
  if (name == "cat") {
    return std::make_unique<CatCommand>(args);
  }
  if (name == "wc") {
    return std::make_unique<WcCommand>(args);
  }
  if (name == "exit") {
    return std::make_unique<ExitCommand>(args);
  }

  return std::make_unique<ExternalCommand>(name, args, envForCommand);
}

}  // namespace cppshell
