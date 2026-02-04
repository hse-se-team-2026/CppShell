#include "cppshell/tokenizer.hpp"

#include <doctest/doctest.h>

TEST_CASE("Tokenize: splits by whitespace") {
  const auto r = cppshell::Tokenize("echo hello   world");
  REQUIRE(r.Ok());
  REQUIRE(r.tokens.size() == 3);
  CHECK(r.tokens[0] == "echo");
  CHECK(r.tokens[1] == "hello");
  CHECK(r.tokens[2] == "world");
}

TEST_CASE("Tokenize: supports double quotes") {
  const auto r = cppshell::Tokenize("echo \"a b\" c");
  REQUIRE(r.Ok());
  REQUIRE(r.tokens.size() == 3);
  CHECK(r.tokens[1] == "a b");
  CHECK(r.tokens[2] == "c");
}

TEST_CASE("Tokenize: supports single quotes") {
  const auto r = cppshell::Tokenize("echo 'a b' c");
  REQUIRE(r.Ok());
  REQUIRE(r.tokens.size() == 3);
  CHECK(r.tokens[1] == "a b");
  CHECK(r.tokens[2] == "c");
}

TEST_CASE("Tokenize: unterminated quote is an error") {
  const auto r = cppshell::Tokenize("echo \"oops");
  CHECK(!r.Ok());
}
