#include "cppshell/shell.hpp"

#include <iostream>

/**
 * Program entry point.
 *
 * Runs the interactive Read-Execute-Print Loop.
 */
int main() {
  cppshell::Shell shell;
  const int code = shell.Run(std::cin, std::cout, std::cerr, true);
  return code;
}