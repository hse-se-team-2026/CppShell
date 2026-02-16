#include "cppshell/grep_command.hpp"
#include "CLI/CLI.hpp"

#include <iostream>
#include <iterator>
#include <regex>
#include <sstream>

namespace cppshell {

GrepCommand::GrepCommand(std::vector<std::string> args)
    : args_(std::move(args)) {}

CommandResult GrepCommand::Execute(CommandContext &context) {
  // CLI11 expects a C-style `argv` array where `argv[0]` is the program name.
  // Our `args_` vector contains only arguments, not the command name itself.
  // We prepend a dummy "grep" string to satisfy CLI11's requirement.
  
  std::vector<std::string> argv;
  argv.reserve(args_.size() + 1);
  argv.push_back("grep");
  argv.insert(argv.end(), args_.begin(), args_.end());

  // Create C-style argv array for CLI11 parsing.
  std::vector<char *> c_argv;
  c_argv.reserve(argv.size());
  for (auto &s : argv) {
    c_argv.push_back(s.data());
  }

  CLI::App app{"grep utility"};

  std::string pattern;
  std::vector<std::string> files;
  bool ignoreCase = false;
  bool wordRegexp = false;
  int afterContext = 0;

  // Define options
  app.add_option("pattern", pattern, "Pattern to search for")->required();
  app.add_option("files", files, "Files to search in");
  app.add_flag("-i,--ignore-case", ignoreCase, "Ignore case distinctions");
  app.add_flag("-w,--word-regexp", wordRegexp, "Select only those lines containing matches that form whole words");
  app.add_option("-A,--after-context", afterContext, "Print NUM lines of trailing context");

  try {
    app.parse(static_cast<int>(c_argv.size()), c_argv.data());
  } catch (const CLI::ParseError &e) {
    // If help flag is present, print to stdout, otherwise error
    if (e.get_name() == "CallForHelp") {
        std::stringstream ss;
        app.exit(e, ss, ss);
        context.streams.out << ss.str();
        return {0};
    }
    // CLI11 formats the error message for us
    std::stringstream ss;
    int exitCode = app.exit(e, ss, ss);
    context.streams.err << ss.str();
    return {exitCode}; // Grep usually returns >0 on error
  }

  // Configure regex
  std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
  if (ignoreCase) {
    flags |= std::regex_constants::icase;
  }

  std::regex re;
  try {
    // Implement -w (word regexp) logic.
    // Ideally, we wrap the pattern with \b boundaries.
    // This works well for standard word characters.
    std::string finalPattern = pattern;
    if (wordRegexp) {
        finalPattern = "\\b" + pattern + "\\b";
    }
    re.assign(finalPattern, flags);
  } catch (const std::regex_error &e) {
    context.streams.err << "grep: invalid regex: " << e.what() << "\n";
    return {2};
  }

  // If no files provided, read from stdin (represented by empty string in our logic below)
  if (files.empty()) {
    files.push_back(""); 
  }

  int returnCode = 1; // 1 means "no match found" (standard grep behavior)
  
  // Logic for context printing (-A)
  // We track how many lines to print remaining after a match using 'linesToPrint'.
  // Standard grep merges overlapping contexts, which is naturally handled by resetting 
  // the counter on a new match.

  for (const auto &file : files) {
    std::unique_ptr<std::istream> fileStream;
    std::istream *in = nullptr;

    if (file.empty() || file == "-") {
      in = &context.streams.in; // Use shared context stdin
    } else {
        auto fs = std::make_unique<std::ifstream>(file);
        if (!*fs) {
            context.streams.err << "grep: " << file << ": No such file or directory\n";
            returnCode = 2; // Error occurred
            continue;
        }
        fileStream = std::move(fs);
        in = fileStream.get();
    }

    std::string line;
    int linesToPrint = 0; // Remaining lines to print for context
    
    // Read lines one by one. Handles Windows CRLF by stripping \r.
    while (std::getline(*in, line)) {
      if (!line.empty() && line.back() == '\r') {
          line.pop_back();
      }

      bool match = false;
      if (std::regex_search(line, re)) {
        match = true;
        returnCode = 0; // Found at least one match
        linesToPrint = afterContext; // Reset counter/extend context on new match
      }

      if (match || linesToPrint > 0) {
        context.streams.out << line << '\n';
        if (!match) {
            linesToPrint--;
        }
      }
    }
  }

  return {returnCode};
}

} // namespace cppshell
