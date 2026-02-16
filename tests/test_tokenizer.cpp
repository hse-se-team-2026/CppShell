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

TEST_CASE("Tokenize: pipes") {
  SUBCASE("Surrounded by spaces") {
    const auto r = cppshell::Tokenize("ls | wc");
    REQUIRE(r.Ok());
    REQUIRE(r.tokens.size() == 3);
    CHECK(r.tokens[0] == "ls");
    CHECK(r.tokens[1] == "|");
    CHECK(r.tokens[2] == "wc");
  }

  SUBCASE("No spaces") {
    const auto r = cppshell::Tokenize("ls|wc");
    REQUIRE(r.Ok());
    REQUIRE(r.tokens.size() == 3);
    CHECK(r.tokens[0] == "ls");
    CHECK(r.tokens[1] == "|");
    CHECK(r.tokens[2] == "wc");
  }

  SUBCASE("Multiple pipes") {
    const auto r = cppshell::Tokenize("a|b|c");
    REQUIRE(r.Ok());
    REQUIRE(r.tokens.size() == 5);
    CHECK(r.tokens[1] == "|");
    CHECK(r.tokens[3] == "|");
  }

  SUBCASE("Pipes with quotes") {
    // "a"|"b" -> a | b
    const auto r = cppshell::Tokenize("\"a\"|'b'");
    REQUIRE(r.Ok());
    REQUIRE(r.tokens.size() == 3);
    CHECK(r.tokens[0] == "a");
    CHECK(r.tokens[1] == "|");
    CHECK(r.tokens[1] == "|");
    CHECK(r.tokens[2] == "b");
  }
}

TEST_CASE("Tokenize: grep regex patterns") {
  // Test that special regex characters are preserved when quoted
  // grep "^test$" file
  const auto r1 = cppshell::Tokenize("grep \"^test$\" file");
  REQUIRE(r1.Ok());
  REQUIRE(r1.tokens.size() == 3);
  CHECK(r1.tokens[1] == "^test$");

  // Test single quotes for avoiding expansion (though tokenizer handles both
  // similarly for now) grep '.*' file
  const auto r2 = cppshell::Tokenize("grep '.*' file");
  REQUIRE(r2.Ok());
  CHECK(r2.tokens[1] == ".*");

  // Test escaping internal quotes
  // grep "foo\"bar"
  const auto r3 = cppshell::Tokenize("grep \"foo\\\"bar\"");
  REQUIRE(r3.Ok());
  // Tokenizer usually stores unescaped content if implemented that way,
  // or raw tokens. Let's check implementation behavior from other tests.
  // "echo \"a b\" c" -> "a b" (quotes stripped).
  // So "foo\"bar" inside quotes -> foo"bar.
  // Wait, C++ string literal "grep \"foo\\\"bar\"" means input is: grep
  // "foo\"bar" Parser/Tokenizer should strip outer quotes and unescape inner.
  CHECK(r3.tokens[1] == "foo\"bar");
}
