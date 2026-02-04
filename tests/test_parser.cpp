#include "cppshell/parser.hpp"

#include <doctest/doctest.h>

TEST_CASE("ParseLine: empty input") {
  const auto r = cppshell::ParseLine("   \t");
  REQUIRE(r.Ok());
  CHECK(!r.line.has_value());
}

TEST_CASE("ParseLine: command and args") {
  const auto r = cppshell::ParseLine("echo hello world");
  REQUIRE(r.Ok());
  REQUIRE(r.line.has_value());
  CHECK(r.line->command == "echo");
  REQUIRE(r.line->args.size() == 2);
  CHECK(r.line->args[0] == "hello");
  CHECK(r.line->args[1] == "world");
}

TEST_CASE("ParseLine: leading env assignments") {
  const auto r = cppshell::ParseLine("FOO=bar BAZ=qux echo hi");
  REQUIRE(r.Ok());
  REQUIRE(r.line.has_value());
  CHECK(r.line->command == "echo");
  CHECK(r.line->args.size() == 1);
  CHECK(r.line->args[0] == "hi");
  CHECK(r.line->assignments.at("FOO") == "bar");
  CHECK(r.line->assignments.at("BAZ") == "qux");
}

TEST_CASE("ParseLine: assignments only") {
  const auto r = cppshell::ParseLine("FOO=bar");
  REQUIRE(r.Ok());
  REQUIRE(r.line.has_value());
  CHECK(r.line->command.empty());
  CHECK(r.line->assignments.at("FOO") == "bar");
}
