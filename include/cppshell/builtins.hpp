#pragma once

#include "cppshell/command.hpp"

#include <string>
#include <vector>

namespace cppshell {

/** Builtin: echo. */
class EchoCommand final : public ICommand {
public:
  /** Constructs the command with its argv (excluding the command name). */
  explicit EchoCommand(std::vector<std::string> args);

  /** Prints args to stdout separated by spaces and ends with '\n'. */
  [[nodiscard]] CommandResult Execute(CommandContext &context) override;

private:
  std::vector<std::string> args_;
};

/** Builtin: pwd. */
class PwdCommand final : public ICommand {
public:
  /** Constructs the command with its argv (excluding the command name). */
  explicit PwdCommand(std::vector<std::string> args);

  /** Prints the current working directory. Errors on extra args. */
  [[nodiscard]] CommandResult Execute(CommandContext &context) override;

private:
  std::vector<std::string> args_;
};

/** Builtin: cat. */
class CatCommand final : public ICommand {
public:
  /** Constructs the command with its argv (excluding the command name). */
  explicit CatCommand(std::vector<std::string> args);

  /** Outputs file contents to stdout; if no args, copies stdin to stdout. */
  [[nodiscard]] CommandResult Execute(CommandContext &context) override;

private:
  std::vector<std::string> args_;
};

/** Builtin: wc. */
class WcCommand final : public ICommand {
public:
  /** Constructs the command with its argv (excluding the command name). */
  explicit WcCommand(std::vector<std::string> args);

  /** Prints `<lines> <words> <bytes>` for a file; if no args, reads stdin. */
  [[nodiscard]] CommandResult Execute(CommandContext &context) override;

private:
  std::vector<std::string> args_;
};

/** Builtin: exit. */
class ExitCommand final : public ICommand {
public:
  /** Constructs the command with its argv (excluding the command name). */
  explicit ExitCommand(std::vector<std::string> args);

  /** Requests shell termination with optional numeric exit code. */
  [[nodiscard]] CommandResult Execute(CommandContext &context) override;

private:
  std::vector<std::string> args_;
};

} // namespace cppshell
