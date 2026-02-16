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

TEST_CASE("ParseLine: pipeline three commands") {
  const auto r = cppshell::ParseLine("cmd1 | cmd2 | cmd3");
  REQUIRE(r.Ok());
  REQUIRE(r.pipeline.has_value());
  REQUIRE(r.pipeline->commands.size() == 3);

  CHECK(r.pipeline->commands[0].command == "cmd1");
  CHECK(r.pipeline->commands[1].command == "cmd2");
  CHECK(r.pipeline->commands[2].command == "cmd3");
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

TEST_CASE("ParseLine: pipeline with assignments") {
  const auto r = cppshell::ParseLine("VAR=1 cmd1 arg1 | VAR=2 cmd2");
  REQUIRE(r.Ok());
  REQUIRE(r.pipeline.has_value());
  REQUIRE(r.pipeline->commands.size() == 2);

  const auto &cmd1 = r.pipeline->commands[0];
  CHECK(cmd1.command == "cmd1");
  CHECK(cmd1.assignments.at("VAR") == "1");

  const auto &cmd2 = r.pipeline->commands[1];
  CHECK(cmd2.command == "cmd2");
  CHECK(cmd2.assignments.at("VAR") == "2");
}

TEST_CASE("ParseLine: quotes in pipeline") {
  // echo "a | b" | cat -> echo gets "a | b" as arg
  const auto r = cppshell::ParseLine("echo \"a | b\" | cat");
  REQUIRE(r.Ok());
  REQUIRE(r.pipeline.has_value());
  REQUIRE(r.pipeline->commands.size() == 2);

  const auto &cmd1 = r.pipeline->commands[0];
  CHECK(cmd1.command == "echo");
  CHECK(cmd1.args.size() == 1);
  CHECK(cmd1.args[0] == "a | b"); // Pipe inside quotes preserved

  const auto &cmd2 = r.pipeline->commands[1];
  CHECK(cmd2.command == "cat");
}

TEST_CASE("ParseLine: grep arguments") {
  // Check that grep flags and quoted patterns are parsed correctly as arguments
  const auto r = cppshell::ParseLine("grep -i -A 2 \"some pattern\" file.txt");
  REQUIRE(r.Ok());
  REQUIRE(r.pipeline.has_value());
  REQUIRE(r.pipeline->commands.size() == 1);

  const auto &cmd = r.pipeline->commands[0];
  CHECK(cmd.command == "grep");
  // Expected args: -i, -A, 2, "some pattern", "file.txt"
  REQUIRE(cmd.args.size() == 5);
  CHECK(cmd.args[0] == "-i");
  CHECK(cmd.args[1] == "-A");
  CHECK(cmd.args[2] == "2");
  CHECK(cmd.args[3] == "some pattern");
  CHECK(cmd.args[4] == "file.txt");
}

TEST_CASE("ParseLine: grep complex pipeline on windows") {
  // Pipeline with grep and multiple flags
  const auto r = cppshell::ParseLine("cat input.txt | grep -w \"valid\" | wc");
  REQUIRE(r.Ok());
  REQUIRE(r.pipeline.has_value());
  REQUIRE(r.pipeline->commands.size() == 3);

  CHECK(r.pipeline->commands[0].command == "cat");

  const auto &grepCmd = r.pipeline->commands[1];
  CHECK(grepCmd.command == "grep");
  REQUIRE(grepCmd.args.size() == 2);
  CHECK(grepCmd.args[0] == "-w");
  CHECK(grepCmd.args[1] == "valid");

  CHECK(r.pipeline->commands[2].command == "wc");
}
