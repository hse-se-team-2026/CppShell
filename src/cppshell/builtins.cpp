#include "cppshell/builtins.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <map>
#include <sstream>

namespace cppshell {

namespace {

struct WcStats {
  size_t lines = 0;
  size_t words = 0;
  size_t bytes = 0;
};

[[nodiscard]] WcStats CountStream(std::istream &in) {
  WcStats s;
  bool inWord = false;

  std::istreambuf_iterator<char> it(in);
  const std::istreambuf_iterator<char> end;
  for (; it != end; ++it) {
    unsigned char uch = static_cast<unsigned char>(*it);
    ++s.bytes;
    if (uch == '\n') {
      ++s.lines;
    }

    const bool ws = (std::isspace(uch) != 0);
    if (ws) {
      inWord = false;
    } else {
      if (!inWord) {
        ++s.words;
        inWord = true;
      }
    }
  }

  return s;
}

} // namespace

EchoCommand::EchoCommand(std::vector<std::string> args)
    : args_(std::move(args)) {}

CommandResult EchoCommand::Execute(CommandContext &context) {
  for (size_t i = 0; i < args_.size(); ++i) {
    if (i != 0) {
      context.streams.out << ' ';
    }
    context.streams.out << args_[i];
  }
  context.streams.out << '\n';
  CommandResult r;
  r.exitCode = 0;
  return r;
}

PwdCommand::PwdCommand(std::vector<std::string> args)
    : args_(std::move(args)) {}

CommandResult PwdCommand::Execute(CommandContext &context) {
  if (!args_.empty()) {
    context.streams.err << "pwd: too many arguments\n";
    CommandResult r;
    r.exitCode = 2;
    return r;
  }

  std::error_code ec;
  const std::filesystem::path cwd = std::filesystem::current_path(ec);
  if (ec) {
    context.streams.err << "pwd: failed to get current directory\n";
    CommandResult r;
    r.exitCode = 1;
    return r;
  }

  context.streams.out << cwd.string() << '\n';
  CommandResult r;
  r.exitCode = 0;
  return r;
}

CatCommand::CatCommand(std::vector<std::string> args)
    : args_(std::move(args)) {}

CommandResult CatCommand::Execute(CommandContext &context) {
  int exitCode = 0;

  if (args_.empty()) {
    context.streams.out << context.streams.in.rdbuf();
    CommandResult r;
    r.exitCode = 0;
    return r;
  }

  for (const auto &file : args_) {
    std::ifstream in(file, std::ios::binary);
    if (!in) {
      context.streams.err << "cat: cannot open file: " << file << "\n";
      exitCode = 1;
      continue;
    }
    context.streams.out << in.rdbuf();
  }

  CommandResult r;
  r.exitCode = exitCode;
  return r;
}

WcCommand::WcCommand(std::vector<std::string> args) : args_(std::move(args)) {}

CommandResult WcCommand::Execute(CommandContext &context) {
  if (args_.empty()) {
    const WcStats s = CountStream(context.streams.in);
    context.streams.out << s.lines << ' ' << s.words << ' ' << s.bytes << '\n';
    CommandResult r;
    r.exitCode = 0;
    return r;
  }

  if (args_.size() != 1) {
    context.streams.err << "wc: expected exactly one file argument\n";
    CommandResult r;
    r.exitCode = 2;
    return r;
  }

  const std::string &file = args_.front();
  std::ifstream in(file, std::ios::binary);
  if (!in) {
    context.streams.err << "wc: cannot open file: " << file << "\n";
    CommandResult r;
    r.exitCode = 1;
    return r;
  }

  const WcStats s = CountStream(in);
  context.streams.out << s.lines << ' ' << s.words << ' ' << s.bytes << '\n';
  CommandResult r;
  r.exitCode = 0;
  return r;
}

ExitCommand::ExitCommand(std::vector<std::string> args)
    : args_(std::move(args)) {}

CommandResult ExitCommand::Execute(CommandContext &context) {
  if (args_.empty()) {
    CommandResult r;
    r.exitCode = 0;
    r.shouldExit = true;
    r.shellExitCode = 0;
    return r;
  }

  if (args_.size() > 1) {
    context.streams.err << "exit: too many arguments\n";
    CommandResult r;
    r.exitCode = 1;
    r.shouldExit = false;
    return r;
  }

  try {
    const int code = std::stoi(args_[0]);
    CommandResult r;
    r.exitCode = 0;
    r.shouldExit = true;
    r.shellExitCode = code;
    return r;
  } catch (...) {
    context.streams.err << "exit: expected numeric exit code\n";
    CommandResult r;
    r.exitCode = 2;
    return r;
  }
}

namespace {

struct BuiltinInfo {
  std::string summary;
  std::string detail;
};

const std::map<std::string, BuiltinInfo> &GetBuiltinsInfo() {
  static const std::map<std::string, BuiltinInfo> builtins = {
      {"echo",
       {"echo [arg ...]",
        "Output the args, separated by spaces, terminated with a newline.\n"
        "Exit Status: Returns 0 unless a write error occurs."}},
      {"pwd",
       {"pwd",
        "Print the absolute pathname of the current working directory.\n"
        "Exit Status: Returns 0 unless an error occurs while reading the "
        "current directory."}},
      {"cat",
       {"cat [FILE]...",
        "Concatenate FILE(s) to standard output.\n"
        "With no FILE, or when FILE is -, read standard input."}},
      {"wc",
       {"wc [FILE]", "Print newline, word, and byte counts for each FILE.\n"
                     "With no FILE, or when FILE is -, read standard input."}},
      {"exit",
       {"exit [n]",
        "Exit the shell with a status of N.  If N is omitted, the exit "
        "status is that of the last command executed."}},
      {"grep",
       {"grep [OPTIONS] PATTERN [FILE]...",
        "Search for PATTERN in each FILE or standard input.\n"
        "Options:\n"
        "  -i, --ignore-case    ignore case distinctions\n"
        "  -w, --word-regexp    force PATTERN to match only whole words\n"
        "  -A, --after-context=NUM\n"
        "                       print NUM lines of trailing context\n"
        "Examples of PATTERN (ECMAScript syntax):\n"
        "  ^Error               lines starting with 'Error'\n"
        "  [0-9]+               lines containing one or more digits\n"
        "  (cpp|hpp)$           lines ending in .cpp or .hpp\n"
        "  \\bword\\b             lines containing 'word' as a whole word"}},
      {"help",
       {"help [pattern ...]",
        "Display information about builtin commands.\n"
        "Displays brief summaries of builtin commands.  If PATTERN is\n"
        "specified, gives detailed help on all commands matching PATTERN."}}};
  return builtins;
}

} // namespace

HelpCommand::HelpCommand(std::vector<std::string> args)
    : args_(std::move(args)) {}

CommandResult HelpCommand::Execute(CommandContext &context) {
  const auto &builtins = GetBuiltinsInfo();

  if (args_.empty()) {
    context.streams.out << "CppShell, version 0.1.0-release\n"
                           "These shell commands are defined internally.  Type "
                           "`help' to see this list.\n"
                           "Type `help name' to find out more about the "
                           "function `name'.\n\n";

    size_t i = 0;
    for (const auto &[name, info] : builtins) {
      context.streams.out << std::left << std::setw(20) << info.summary;
      if (++i % 2 == 0) {
        context.streams.out << '\n';
      }
    }
    if (i % 2 != 0) {
      context.streams.out << '\n';
    }
    return {0};
  }

  int exitCode = 0;
  for (const auto &pattern : args_) {
    auto it = builtins.find(pattern);
    if (it != builtins.end()) {
      context.streams.out << it->first << ": " << it->second.summary << "\n"
                          << "    " << it->second.detail << "\n";
    } else {
      context.streams.err << "help: no help topics match `" << pattern
                          << "'.  Try `help help'.\n";
      exitCode = 1;
    }
  }

  return {exitCode};
}

} // namespace cppshell
