#include "cppshell/external_command.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <limits>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <vector>
#else
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
#endif
#include <algorithm>

namespace cppshell {

namespace {

constexpr size_t kBufferSize = 4096;

#ifdef _WIN32

struct PipePair {
  HANDLE read = nullptr;
  HANDLE write = nullptr;
};

[[nodiscard]] bool CreateInheritablePipe(PipePair &pipe, bool parentReads) {
  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;

  HANDLE r = nullptr;
  HANDLE w = nullptr;
  if (!CreatePipe(&r, &w, &sa, 0)) {
    return false;
  }

  // Disable inheritance for the end used by parent.
  if (parentReads) {
    SetHandleInformation(r, HANDLE_FLAG_INHERIT, 0);
  } else {
    SetHandleInformation(w, HANDLE_FLAG_INHERIT, 0);
  }

  pipe.read = r;
  pipe.write = w;
  return true;
}

void CloseHandleIfValid(HANDLE h) {
  if (h != nullptr) {
    CloseHandle(h);
  }
}

void PumpHandleToStream(HANDLE hRead, std::ostream &out) {
  char buffer[kBufferSize];
  DWORD read = 0;

  while (ReadFile(hRead, buffer, static_cast<DWORD>(sizeof(buffer)), &read,
                  nullptr) &&
         read != 0) {
    out.write(buffer, static_cast<std::streamsize>(read));
    out.flush();
  }
}

void PumpStreamToHandle(std::istream &in, HANDLE hWrite) {
  char buffer[kBufferSize];

  while (in) {
    in.read(buffer, static_cast<std::streamsize>(sizeof(buffer)));
    const std::streamsize got = in.gcount();
    if (got <= 0) {
      break;
    }

    const char *p = buffer;
    size_t remaining = static_cast<size_t>(got);
    while (remaining != 0) {
      DWORD written = 0;
      const DWORD toWrite = static_cast<DWORD>(
          remaining > static_cast<size_t>(std::numeric_limits<DWORD>::max())
              ? std::numeric_limits<DWORD>::max()
              : remaining);
      if (!WriteFile(hWrite, p, toWrite, &written, nullptr)) {
        return;
      }
      p += written;
      remaining -= written;
    }
  }
}

#else

struct PipePair {
  int read = -1;
  int write = -1;
};

void CloseFdIfValid(int fd) {
  if (fd >= 0) {
    close(fd);
  }
}

[[nodiscard]] bool CreatePipe(PipePair &pipe) {
  int fds[2] = {-1, -1};
  if (::pipe(fds) != 0) {
    return false;
  }

  // Best-effort close-on-exec.
  for (int i = 0; i < 2; ++i) {
    const int flags = fcntl(fds[i], F_GETFD);
    if (flags >= 0) {
      (void)fcntl(fds[i], F_SETFD, flags | FD_CLOEXEC);
    }
  }

  pipe.read = fds[0];
  pipe.write = fds[1];
  return true;
}

void PumpFdToStream(int fdRead, std::ostream &out) {
  char buffer[kBufferSize];
  while (true) {
    const ssize_t n = ::read(fdRead, buffer, sizeof(buffer));
    if (n <= 0) {
      break;
    }
    out.write(buffer, static_cast<std::streamsize>(n));
    out.flush();
  }
}

void PumpStreamToFd(std::istream &in, int fdWrite) {
  char buffer[kBufferSize];
  while (in) {
    in.read(buffer, static_cast<std::streamsize>(sizeof(buffer)));
    const std::streamsize got = in.gcount();
    if (got <= 0) {
      break;
    }

    const char *p = buffer;
    size_t remaining = static_cast<size_t>(got);
    while (remaining != 0) {
      const ssize_t wrote = ::write(fdWrite, p, remaining);
      if (wrote <= 0) {
        return;
      }
      p += wrote;
      remaining -= static_cast<size_t>(wrote);
    }
  }
}

#endif

#ifdef _WIN32
std::wstring Utf8ToWide(const std::string &value) {
  if (value.empty()) {
    return {};
  }
  const int size =
      MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
  if (size <= 0) {
    return {};
  }
  std::wstring out(static_cast<size_t>(size - 1), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, out.data(), size);
  return out;
}

std::wstring QuoteWindowsArg(const std::wstring &arg) {
  if (arg.empty()) {
    return L"\"\"";
  }

  const bool needQuotes = std::any_of(arg.begin(), arg.end(), [](wchar_t c) {
    return c == L' ' || c == L'\t' || c == L'\n' || c == L'\v' || c == L'\"';
  });
  if (!needQuotes) {
    return arg;
  }

  std::wstring out;
  out.push_back(L'\"');

  size_t backslashes = 0;
  for (wchar_t c : arg) {
    if (c == L'\\') {
      ++backslashes;
      continue;
    }

    if (c == L'\"') {
      out.append(backslashes * 2 + 1, L'\\');
      out.push_back(L'\"');
      backslashes = 0;
      continue;
    }

    if (backslashes != 0) {
      out.append(backslashes, L'\\');
      backslashes = 0;
    }
    out.push_back(c);
  }

  if (backslashes != 0) {
    out.append(backslashes * 2, L'\\');
  }

  out.push_back(L'\"');
  return out;
}

std::wstring BuildWindowsCommandLine(const std::vector<std::string> &argvUtf8) {
  std::wstring cmd;
  for (size_t i = 0; i < argvUtf8.size(); ++i) {
    if (i != 0) {
      cmd.push_back(L' ');
    }
    cmd.append(QuoteWindowsArg(Utf8ToWide(argvUtf8[i])));
  }
  return cmd;
}
#endif

} // namespace

ExternalCommand::ExternalCommand(std::string program,
                                 std::vector<std::string> args,
                                 Environment envForCommand)
    : program_(std::move(program)), args_(std::move(args)),
      env_(std::move(envForCommand)) {}

CommandResult ExternalCommand::Execute(CommandContext &context) {
  const bool inheritIn = (&context.streams.in == &std::cin);
  const bool inheritOut = (&context.streams.out == &std::cout);
  const bool inheritErr = (&context.streams.err == &std::cerr);

#ifdef _WIN32
  std::vector<std::string> argv;
  argv.reserve(1 + args_.size());
  argv.push_back(program_);
  argv.insert(argv.end(), args_.begin(), args_.end());

  std::wstring cmdLine = BuildWindowsCommandLine(argv);
  std::wstring envBlock = env_.ToWindowsEnvironmentBlock();

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};

  PipePair stdinPipe{};
  PipePair stdoutPipe{};
  PipePair stderrPipe{};

  const bool needRedirectIn = !inheritIn;
  const bool needRedirectOut = !inheritOut;
  const bool needRedirectErr = !inheritErr;

  if (needRedirectIn && !CreateInheritablePipe(stdinPipe, false)) {
    return CommandResult{.exitCode = 127};
  }
  if (needRedirectOut && !CreateInheritablePipe(stdoutPipe, true)) {
    CloseHandleIfValid(stdinPipe.read);
    CloseHandleIfValid(stdinPipe.write);
    return CommandResult{.exitCode = 127};
  }
  if (needRedirectErr && !CreateInheritablePipe(stderrPipe, true)) {
    CloseHandleIfValid(stdinPipe.read);
    CloseHandleIfValid(stdinPipe.write);
    CloseHandleIfValid(stdoutPipe.read);
    CloseHandleIfValid(stdoutPipe.write);
    return CommandResult{.exitCode = 127};
  }

  if (needRedirectIn || needRedirectOut || needRedirectErr) {
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput =
        needRedirectIn ? stdinPipe.read : GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput =
        needRedirectOut ? stdoutPipe.write : GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError =
        needRedirectErr ? stderrPipe.write : GetStdHandle(STD_ERROR_HANDLE);
  }

  const DWORD flags = CREATE_UNICODE_ENVIRONMENT;
  const BOOL ok = CreateProcessW(
      nullptr, cmdLine.data(), nullptr, nullptr,
      (needRedirectIn || needRedirectOut || needRedirectErr) ? TRUE : FALSE,
      flags, envBlock.empty() ? nullptr : envBlock.data(), nullptr, &si, &pi);

  if (!ok) {
    CloseHandleIfValid(stdinPipe.read);
    CloseHandleIfValid(stdinPipe.write);
    CloseHandleIfValid(stdoutPipe.read);
    CloseHandleIfValid(stdoutPipe.write);
    CloseHandleIfValid(stderrPipe.read);
    CloseHandleIfValid(stderrPipe.write);
    return CommandResult{.exitCode = 127};
  }

  // Parent doesn't need the child ends.
  if (needRedirectIn) {
    CloseHandleIfValid(stdinPipe.read);
  }
  if (needRedirectOut) {
    CloseHandleIfValid(stdoutPipe.write);
  }
  if (needRedirectErr) {
    CloseHandleIfValid(stderrPipe.write);
  }

  std::thread outThread;
  std::thread errThread;
  std::thread inThread;

  if (needRedirectOut) {
    outThread = std::thread([&]() {
      PumpHandleToStream(stdoutPipe.read, context.streams.out);
      CloseHandleIfValid(stdoutPipe.read);
    });
  }
  if (needRedirectErr) {
    errThread = std::thread([&]() {
      PumpHandleToStream(stderrPipe.read, context.streams.err);
      CloseHandleIfValid(stderrPipe.read);
    });
  }
  if (needRedirectIn) {
    inThread = std::thread([&]() {
      PumpStreamToHandle(context.streams.in, stdinPipe.write);
      CloseHandleIfValid(stdinPipe.write);
    });
  }

  WaitForSingleObject(pi.hProcess, INFINITE);

  DWORD exitCode = 0;
  GetExitCodeProcess(pi.hProcess, &exitCode);

  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);

  if (outThread.joinable()) {
    outThread.join();
  }
  if (errThread.joinable()) {
    errThread.join();
  }
  if (inThread.joinable()) {
    inThread.join();
  }

  return CommandResult{.exitCode = static_cast<int>(exitCode)};
#else
  std::vector<std::string> argvStrings;
  argvStrings.reserve(1 + args_.size());
  argvStrings.push_back(program_);
  argvStrings.insert(argvStrings.end(), args_.begin(), args_.end());

  std::vector<char *> argv;
  argv.reserve(argvStrings.size() + 1);
  for (const auto &s : argvStrings) {
    argv.push_back(const_cast<char *>(s.c_str()));
  }
  argv.push_back(nullptr);

  std::vector<std::string> envStrings = env_.ToEnvStrings();
  std::vector<char *> envp;
  envp.reserve(envStrings.size() + 1);
  for (const auto &s : envStrings) {
    envp.push_back(const_cast<char *>(s.c_str()));
  }
  envp.push_back(nullptr);

  PipePair stdinPipe{};
  PipePair stdoutPipe{};
  PipePair stderrPipe{};

  const bool needRedirectIn = !inheritIn;
  const bool needRedirectOut = !inheritOut;
  const bool needRedirectErr = !inheritErr;

  if (needRedirectIn && !CreatePipe(stdinPipe)) {
    return CommandResult{.exitCode = 127};
  }
  if (needRedirectOut && !CreatePipe(stdoutPipe)) {
    CloseFdIfValid(stdinPipe.read);
    CloseFdIfValid(stdinPipe.write);
    return CommandResult{.exitCode = 127};
  }
  if (needRedirectErr && !CreatePipe(stderrPipe)) {
    CloseFdIfValid(stdinPipe.read);
    CloseFdIfValid(stdinPipe.write);
    CloseFdIfValid(stdoutPipe.read);
    CloseFdIfValid(stdoutPipe.write);
    return CommandResult{.exitCode = 127};
  }

  posix_spawn_file_actions_t actions;
  posix_spawn_file_actions_init(&actions);
  if (needRedirectIn) {
    posix_spawn_file_actions_adddup2(&actions, stdinPipe.read, STDIN_FILENO);
    posix_spawn_file_actions_addclose(&actions, stdinPipe.write);
  }
  if (needRedirectOut) {
    posix_spawn_file_actions_adddup2(&actions, stdoutPipe.write, STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&actions, stdoutPipe.read);
  }
  if (needRedirectErr) {
    posix_spawn_file_actions_adddup2(&actions, stderrPipe.write, STDERR_FILENO);
    posix_spawn_file_actions_addclose(&actions, stderrPipe.read);
  }

  pid_t pid{};
  const int rc = posix_spawnp(
      &pid, program_.c_str(),
      (needRedirectIn || needRedirectOut || needRedirectErr) ? &actions
                                                             : nullptr,
      nullptr, argv.data(), envp.data());
  posix_spawn_file_actions_destroy(&actions);
  if (rc != 0) {
    CloseFdIfValid(stdinPipe.read);
    CloseFdIfValid(stdinPipe.write);
    CloseFdIfValid(stdoutPipe.read);
    CloseFdIfValid(stdoutPipe.write);
    CloseFdIfValid(stderrPipe.read);
    CloseFdIfValid(stderrPipe.write);
    return CommandResult{.exitCode = 127};
  }

  if (needRedirectIn) {
    CloseFdIfValid(stdinPipe.read);
  }
  if (needRedirectOut) {
    CloseFdIfValid(stdoutPipe.write);
  }
  if (needRedirectErr) {
    CloseFdIfValid(stderrPipe.write);
  }

  std::thread outThread;
  std::thread errThread;
  std::thread inThread;

  if (needRedirectOut) {
    outThread = std::thread([&]() {
      PumpFdToStream(stdoutPipe.read, context.streams.out);
      CloseFdIfValid(stdoutPipe.read);
    });
  }
  if (needRedirectErr) {
    errThread = std::thread([&]() {
      PumpFdToStream(stderrPipe.read, context.streams.err);
      CloseFdIfValid(stderrPipe.read);
    });
  }
  if (needRedirectIn) {
    inThread = std::thread([&]() {
      PumpStreamToFd(context.streams.in, stdinPipe.write);
      CloseFdIfValid(stdinPipe.write);
    });
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    return CommandResult{.exitCode = 127};
  }

  if (outThread.joinable()) {
    outThread.join();
  }
  if (errThread.joinable()) {
    errThread.join();
  }
  if (inThread.joinable()) {
    inThread.join();
  }

  if (WIFEXITED(status)) {
    return CommandResult{.exitCode = WEXITSTATUS(status)};
  }
  if (WIFSIGNALED(status)) {
    return CommandResult{.exitCode = 128 + WTERMSIG(status)};
  }
  return CommandResult{.exitCode = 127};
#endif
}

} // namespace cppshell
