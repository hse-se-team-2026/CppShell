#include "cppshell/shell.hpp"

#include "cppshell/expander.hpp"
#include "cppshell/parser.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include "cppshell/pipe.hpp"
#include <thread>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace cppshell {

Shell::Shell() : baseEnv_(), factory_() {}

int Shell::Run(std::istream &in, std::ostream &out, std::ostream &err,
               bool interactive) {
  int lastExitCode = 0;
  std::string line;

  while (true) {
    if (interactive) {
      out << "cppshell> " << std::flush;
    }

    if (!std::getline(in, line)) {
      // EOF
      return lastExitCode;
    }

    // Perform variable substitution and arithmetic expansion.
    const std::string expandedLine = Expand(line, baseEnv_);

    const ParseResult parsed = ParseLine(expandedLine);
    if (!parsed.Ok()) {
      err << "parse error: " << parsed.error << '\n';
      lastExitCode = 2; // Syntax error code
      continue;
    }

    if (!parsed.pipeline.has_value()) {
      continue;
    }

    const Pipeline &pipeline = *parsed.pipeline;
    if (pipeline.commands.empty()) {
      continue;
    }

    // Single command optimization (and required for builtins changing shell
    // state like cd/exit)
    if (pipeline.commands.size() == 1) {
      const auto &cmdData = pipeline.commands[0];
      if (cmdData.command.empty()) {
        for (const auto &[name, value] : cmdData.assignments) {
          baseEnv_.Set(name, value);
        }
        lastExitCode = 0; // Assignment-only commands usually succeed
        continue;
      }
      Environment envForCommand = baseEnv_.WithOverrides(cmdData.assignments);
      CommandStreams streams{in, out, err};
      CommandContext ctx{streams, envForCommand};
      std::unique_ptr<ICommand> cmd =
          factory_.Create(cmdData.command, cmdData.args, envForCommand);
      const CommandResult r = cmd->Execute(ctx);
      lastExitCode = r.exitCode;
      if (r.shouldExit) {
        return r.shellExitCode;
      }
      continue;
    }

#ifdef _WIN32
    // Windows Implementation using std::thread and in-memory Pipe
    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<Pipe>> pipes;
    std::vector<int> exitCodes(pipeline.commands.size(), 0);

    // Create pipes
    for (size_t i = 0; i < pipeline.commands.size() - 1; ++i) {
      pipes.push_back(std::make_unique<Pipe>());
    }

    for (size_t i = 0; i < pipeline.commands.size(); ++i) {
      threads.emplace_back([&, i]() {
        const auto &cmdData = pipeline.commands[i];
        Environment envForCommand = baseEnv_.WithOverrides(cmdData.assignments);

        // Input Setup
        std::unique_ptr<PipeReadBuffer> readBuf;
        std::unique_ptr<std::istream> pipeIn;
        std::istream *currentIn = &in;

        if (i > 0) {
          readBuf = std::make_unique<PipeReadBuffer>(*pipes[i - 1]);
          pipeIn = std::make_unique<std::istream>(readBuf.get());
          currentIn = pipeIn.get();
        }

        // Output Setup
        std::unique_ptr<PipeWriteBuffer> writeBuf;
        std::unique_ptr<std::ostream> pipeOut;
        std::ostream *currentOut = &out;

        if (i < pipeline.commands.size() - 1) {
          writeBuf = std::make_unique<PipeWriteBuffer>(*pipes[i]);
          pipeOut = std::make_unique<std::ostream>(writeBuf.get());
          currentOut = pipeOut.get();
        }

        CommandStreams streams{*currentIn, *currentOut, err};
        CommandContext ctx{streams, envForCommand};

        std::unique_ptr<ICommand> cmd =
            factory_.Create(cmdData.command, cmdData.args, envForCommand);
        const CommandResult r = cmd->Execute(ctx);
        exitCodes[i] = r.exitCode;

        // Close output pipe to signal EOF to the next command
        if (i < pipeline.commands.size() - 1) {
          pipes[i]->Close();
        }
      });
    }

    // Wait for all threads
    for (auto &t : threads) {
      if (t.joinable())
        t.join();
    }

    if (!exitCodes.empty()) {
      lastExitCode = exitCodes.back();
    }

#else
    // Linux Implementation (fork/exec/pipe)
    int prevPipeRead = -1;
    std::vector<pid_t> pids;

    // Helper to close FDs safely
    auto safe_close = [](int &fd) {
      if (fd != -1) {
        close(fd);
        fd = -1;
      }
    };

    for (size_t i = 0; i < pipeline.commands.size(); ++i) {
      int pipefds[2] = {-1, -1};
      bool hasNext = (i + 1 < pipeline.commands.size());

      if (hasNext) {
        if (pipe(pipefds) == -1) {
          perror("pipe");
          break;
        }
      }

      pid_t pid = fork();
      if (pid == -1) {
        perror("fork");
        safe_close(pipefds[0]);
        safe_close(pipefds[1]);
        break;
      }

      if (pid == 0) {
        // Child process
        if (prevPipeRead != -1) {
          dup2(prevPipeRead, STDIN_FILENO);
          safe_close(prevPipeRead);
        }
        if (hasNext) {
          dup2(pipefds[1], STDOUT_FILENO);
          safe_close(pipefds[1]);
          safe_close(pipefds[0]); // Child does not read from next pipe
        }

        const auto &cmdData = pipeline.commands[i];
        Environment envForCommand = baseEnv_.WithOverrides(cmdData.assignments);
        // In child, we use std::cin/cout/cerr which are mapped to FDs 0/1/2
        // Since we dup2'd FDs, std::cout writes to pipe.
        CommandStreams streams{std::cin, std::cout, std::cerr};
        CommandContext ctx{streams, envForCommand};

        std::unique_ptr<ICommand> cmd =
            factory_.Create(cmdData.command, cmdData.args, envForCommand);
        const CommandResult r = cmd->Execute(ctx);
        std::exit(r.exitCode);
      } else {
        // Parent process
        pids.push_back(pid);
        if (prevPipeRead != -1) {
          safe_close(prevPipeRead);
        }
        if (hasNext) {
          safe_close(pipefds[1]);    // Parent writes nothing
          prevPipeRead = pipefds[0]; // Parent holds read end for next child
        }
      }
    }

    // Close last read end
    safe_close(prevPipeRead);

    // Wait for all children
    for (pid_t pid : pids) {
      int status;
      waitpid(pid, &status, 0);
      if (WIFEXITED(status)) {
        lastExitCode = WEXITSTATUS(status);
      } else if (WIFSIGNALED(status)) {
        lastExitCode = 128 + WTERMSIG(status);
      }
    }
#endif
    // Return code is from last command? Usually yes.
    // We don't return here but continue loop.
  }
}

} // namespace cppshell
