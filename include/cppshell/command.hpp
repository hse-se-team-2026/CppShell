#pragma once

#include "cppshell/environment.hpp"

#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace cppshell {

/** Execution streams for a command invocation. */
struct CommandStreams {
  /** Command standard input stream. */
  std::istream &in;
  /** Command standard output stream. */
  std::ostream &out;
  /** Command standard error stream. */
  std::ostream &err;
};

/** Context passed to commands during execution. */
struct CommandContext {
  /** Streams for this invocation. */
  CommandStreams streams;
  /** Effective environment for this invocation. */
  const Environment &env;
};

/** Result of executing a command. */
struct CommandResult {
  /** Process/command exit code. */
  int exitCode = 0;
  /** If true, the shell should terminate after this command. */
  bool shouldExit = false;
  /** Shell exit code to return from main() when shouldExit is true. */
  int shellExitCode = 0;
};

/**
 * Command interface.
 *
 * Builtins and external commands share this interface so the shell can execute
 * them uniformly.
 */
class ICommand {
public:
  /** Virtual destructor for interface type. */
  virtual ~ICommand() = default;

  /** Executes the command using the provided context. */
  [[nodiscard]] virtual CommandResult Execute(CommandContext &context) = 0;
};

/** Factory for builtins and external commands. */
class CommandFactory {
public:
  /** Creates an appropriate command implementation for the given name. */
  [[nodiscard]] std::unique_ptr<ICommand>
  Create(const std::string &name, const std::vector<std::string> &args,
         const Environment &envForCommand) const;
};

} // namespace cppshell
