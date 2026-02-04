#pragma once

#include "cppshell/command.hpp"
#include "cppshell/environment.hpp"

#include <istream>
#include <ostream>

namespace cppshell {

/**
 * The interactive Read-Execute-Print Loop.
 *
 * At this stage it supports:
 * - tokenization with quotes;
 * - builtins: cat/echo/wc/pwd/exit;
 * - launching unknown commands as external processes;
 * - environment variables and per-command assignments NAME=value.
 */
class Shell {
 public:
  /** Constructs a shell with the current process environment. */
  Shell();

  /** Runs the loop and returns the shell exit code. */
  [[nodiscard]] int Run(std::istream& in, std::ostream& out, std::ostream& err,
                        bool interactive);

 private:
  Environment baseEnv_;
  CommandFactory factory_;
};

}  // namespace cppshell
