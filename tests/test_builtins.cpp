#include "cppshell/builtins.hpp"
#include "cppshell/environment.hpp"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

cppshell::CommandContext MakeCtx(std::istream& in, std::ostream& out, std::ostream& err,
                                 const cppshell::Environment& env) {
  cppshell::CommandStreams s{in, out, err};
  return cppshell::CommandContext{s, env};
}

}  // namespace

TEST_CASE("echo prints arguments") {
  std::istringstream in("");
  std::ostringstream out;
  std::ostringstream err;
  const cppshell::Environment env;

  cppshell::EchoCommand cmd({"hello", "world"});
  auto ctx = MakeCtx(in, out, err, env);
  const auto r = cmd.Execute(ctx);

  CHECK(r.exitCode == 0);
  CHECK(out.str() == "hello world\n");
}

TEST_CASE("pwd prints current directory") {
  std::istringstream in("");
  std::ostringstream out;
  std::ostringstream err;
  const cppshell::Environment env;

  cppshell::PwdCommand cmd(std::vector<std::string>{});
  auto ctx = MakeCtx(in, out, err, env);
  const auto r = cmd.Execute(ctx);

  CHECK(r.exitCode == 0);
  const auto expected = std::filesystem::current_path().string() + "\n";
  CHECK(out.str() == expected);
}

TEST_CASE("cat outputs file") {
  const auto tmp = std::filesystem::temp_directory_path() / "cppshell_cat_test.txt";
  {
    std::ofstream f(tmp, std::ios::binary);
    f << "abc\n123";
  }

  std::istringstream in("");
  std::ostringstream out;
  std::ostringstream err;
  const cppshell::Environment env;

  cppshell::CatCommand cmd(std::vector<std::string>{tmp.string()});
  auto ctx = MakeCtx(in, out, err, env);
  const auto r = cmd.Execute(ctx);

  CHECK(r.exitCode == 0);
  CHECK(out.str() == "abc\n123");

  std::error_code ec;
  std::filesystem::remove(tmp, ec);
}

TEST_CASE("wc reports lines words bytes") {
  const auto tmp = std::filesystem::temp_directory_path() / "cppshell_wc_test.txt";
  // Content: "a b\ncd\n" -> lines=2, words=3, bytes=7
  {
    std::ofstream f(tmp, std::ios::binary);
    f << "a b\ncd\n";
  }

  std::istringstream in("");
  std::ostringstream out;
  std::ostringstream err;
  const cppshell::Environment env;

  cppshell::WcCommand cmd(std::vector<std::string>{tmp.string()});
  auto ctx = MakeCtx(in, out, err, env);
  const auto r = cmd.Execute(ctx);

  CHECK(r.exitCode == 0);
  CHECK(out.str() == "2 3 7\n");

  std::error_code ec;
  std::filesystem::remove(tmp, ec);
}

TEST_CASE("exit requests termination") {
  std::istringstream in("");
  std::ostringstream out;
  std::ostringstream err;
  const cppshell::Environment env;

  cppshell::ExitCommand cmd({"42"});
  auto ctx = MakeCtx(in, out, err, env);
  const auto r = cmd.Execute(ctx);

  CHECK(r.exitCode == 0);
  CHECK(r.shouldExit);
  CHECK(r.shellExitCode == 42);
}
