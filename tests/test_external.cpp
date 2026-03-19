#include "cppshell/environment.hpp"
#include "cppshell/external_command.hpp"

#include <doctest/doctest.h>

#include <sstream>
#include <string>
#include <unordered_map>

#ifndef CPPSHELL_TEST_HELPER_PATH
#error "CPPSHELL_TEST_HELPER_PATH is not defined"
#endif

namespace {

cppshell::CommandContext MakeCtx(std::istream &in, std::ostream &out,
                                 std::ostream &err,
                                 const cppshell::Environment &env) {
  cppshell::CommandStreams s{in, out, err};
  return cppshell::CommandContext{s, env};
}

[[nodiscard]] std::string NormalizeNewlines(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '\r' && (i + 1) < s.size() && s[i + 1] == '\n') {
      continue;
    }
    out.push_back(s[i]);
  }
  return out;
}

} // namespace

TEST_CASE("ExternalCommand: passes environment variables") {
  std::istringstream in("");
  std::ostringstream out;
  std::ostringstream err;

  cppshell::Environment base;
  const auto env =
      base.WithOverrides(std::unordered_map<std::string, std::string>{
          {"CPPSHELL_TEST_FOO", "bar"}});

  cppshell::ExternalCommand cmd(CPPSHELL_TEST_HELPER_PATH,
                                {"printenv", "CPPSHELL_TEST_FOO"}, env);
  auto ctx = MakeCtx(in, out, err, env);
  const auto r = cmd.Execute(ctx);

  CHECK(r.exitCode == 0);
  CHECK(NormalizeNewlines(out.str()) == "bar\n");
}

TEST_CASE("ExternalCommand: captures stderr") {
  std::istringstream in("");
  std::ostringstream out;
  std::ostringstream err;

  cppshell::Environment env;
  cppshell::ExternalCommand cmd(CPPSHELL_TEST_HELPER_PATH, {"stderr"}, env);
  auto ctx = MakeCtx(in, out, err, env);
  const auto r = cmd.Execute(ctx);

  CHECK(r.exitCode == 0);
  CHECK(NormalizeNewlines(err.str()) == "hello from stderr\n");
}

TEST_CASE("ExternalCommand: forwards stdin to child and captures stdout") {
  std::istringstream in("abc\n123");
  std::ostringstream out;
  std::ostringstream err;

  cppshell::Environment env;
  cppshell::ExternalCommand cmd(CPPSHELL_TEST_HELPER_PATH, {"catstdin"}, env);
  auto ctx = MakeCtx(in, out, err, env);
  const auto r = cmd.Execute(ctx);

  CHECK(r.exitCode == 0);
  CHECK(NormalizeNewlines(out.str()) == "abc\n123");
}

TEST_CASE("ExternalCommand: returns process exit code") {
  std::istringstream in("");
  std::ostringstream out;
  std::ostringstream err;

  cppshell::Environment env;
  cppshell::ExternalCommand cmd(CPPSHELL_TEST_HELPER_PATH, {"exit", "7"}, env);
  auto ctx = MakeCtx(in, out, err, env);
  const auto r = cmd.Execute(ctx);

  CHECK(r.exitCode == 7);
}
