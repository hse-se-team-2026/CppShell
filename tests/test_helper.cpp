#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

[[nodiscard]] int ParseInt(const std::string &s, int fallback) {
  try {
    return std::stoi(s);
  } catch (...) {
    return fallback;
  }
}

} // namespace

int main(int argc, char **argv) {
  std::vector<std::string> args;
  args.reserve(static_cast<size_t>(argc));
  for (int i = 0; i < argc; ++i) {
    args.emplace_back(argv[i] == nullptr ? "" : argv[i]);
  }

  if (args.size() < 2) {
    std::cerr << "usage: helper <command> [args...]\n";
    return 2;
  }

  const std::string &cmd = args[1];

  if (cmd == "printenv") {
    if (args.size() != 3) {
      std::cerr << "printenv: expected NAME\n";
      return 2;
    }

    const char *v = std::getenv(args[2].c_str());
    if (v == nullptr) {
      return 1;
    }

    std::cout << v << "\n";
    return 0;
  }

  if (cmd == "exit") {
    if (args.size() != 3) {
      std::cerr << "exit: expected CODE\n";
      return 2;
    }
    return ParseInt(args[2], 3);
  }

  if (cmd == "stderr") {
    std::cerr << "hello from stderr\n";
    return 0;
  }

  if (cmd == "catstdin") {
    std::cout << std::cin.rdbuf();
    return 0;
  }

  std::cerr << "unknown command\n";
  return 2;
}
