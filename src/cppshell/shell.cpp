#include "cppshell/shell.hpp"

#include "cppshell/parser.hpp"

#include <string>

namespace cppshell {

Shell::Shell() : baseEnv_(), factory_() {}

int Shell::Run(std::istream &in, std::ostream &out, std::ostream &err,
               bool interactive) {
  std::string line;

  while (true) {
    if (interactive) {
      out << "cppshell> " << std::flush;
    }

    if (!std::getline(in, line)) {
      // EOF
      return 0;
    }

    const ParseResult parsed = ParseLine(line);
    if (!parsed.Ok()) {
      err << "parse error: " << parsed.error << '\n';
      continue;
    }

    if (!parsed.line.has_value()) {
      continue;
    }

    const ParsedLine &pl = *parsed.line;

    if (pl.command.empty()) {
      // Only assignments: persist into base environment.
      for (const auto &[name, value] : pl.assignments) {
        baseEnv_.Set(name, value);
      }
      continue;
    }

    const Environment envForCommand = baseEnv_.WithOverrides(pl.assignments);

    CommandStreams streams{in, out, err};
    CommandContext ctx{streams, envForCommand};

    std::unique_ptr<ICommand> cmd =
        factory_.Create(pl.command, pl.args, envForCommand);
    const CommandResult r = cmd->Execute(ctx);

    if (r.shouldExit) {
      return r.shellExitCode;
    }
  }
}

} // namespace cppshell
