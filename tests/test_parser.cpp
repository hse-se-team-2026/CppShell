#include "cppshell/parser.hpp"

#include <doctest/doctest.h>

TEST_CASE("ParseLine: empty input") {
  const auto r = cppshell::ParseLine("   \t");
  REQUIRE(r.Ok());
  CHECK(!r.pipeline.has_value());
}

TEST_CASE("ParseLine: command and args") {
  const auto r = cppshell::ParseLine("echo hello world");
  REQUIRE(r.Ok());
  REQUIRE(r.pipeline.has_value());
  REQUIRE(r.pipeline->commands.size() == 1);
  const auto &cmd = r.pipeline->commands[0];
  CHECK(cmd.command == "echo");
  REQUIRE(cmd.args.size() == 2);
  CHECK(cmd.args[0] == "hello");
  CHECK(cmd.args[1] == "world");
}

TEST_CASE("ParseLine: pipeline two commands") {
  const auto r = cppshell::ParseLine("cmd1 arg1 | cmd2 arg2");
  REQUIRE(r.Ok());
  REQUIRE(r.pipeline.has_value());
  REQUIRE(r.pipeline->commands.size() == 2);

  const auto &cmd1 = r.pipeline->commands[0];
  CHECK(cmd1.command == "cmd1");
  CHECK(cmd1.args.size() == 1);
  CHECK(cmd1.args[0] == "arg1");

  const auto &cmd2 = r.pipeline->commands[1];
  CHECK(cmd2.command == "cmd2");
  CHECK(cmd2.args.size() == 1);
  CHECK(cmd2.args[0] == "arg2");
}

TEST_CASE("ParseLine: leading env assignments") {
  const auto r = cppshell::ParseLine("FOO=bar BAZ=qux echo hi");
  REQUIRE(r.Ok());
  REQUIRE(r.pipeline.has_value());
  const auto &cmd = r.pipeline->commands[0];
  CHECK(cmd.command == "echo");
  CHECK(cmd.args.size() == 1);
  CHECK(cmd.args[0] == "hi");
  CHECK(cmd.assignments.at("FOO") == "bar");
  CHECK(cmd.assignments.at("BAZ") == "qux");
}

TEST_CASE("ParseLine: assignments only") {
  const auto r = cppshell::ParseLine("FOO=bar");
  REQUIRE(r.Ok());
  REQUIRE(r.pipeline.has_value());
  const auto &cmd = r.pipeline->commands[0];
  CHECK(cmd.command.empty());
  CHECK(cmd.assignments.at("FOO") == "bar");
}
