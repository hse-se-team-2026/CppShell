#include "cppshell/grep_command.hpp"
#include "doctest/doctest.h"
#include "cppshell/environment.hpp"
#include <sstream>

using namespace cppshell;

TEST_CASE("GrepCommand") {
    std::stringstream out;
    std::stringstream err;
    std::stringstream in;
    Environment env;
    CommandStreams streams{in, out, err};
    CommandContext ctx{streams, env};

    SUBCASE("Simple match from stdin") {
        in.str("apple\nbanana\ncherry\n");
        GrepCommand cmd({"banana"}); // grep "banana"
        CommandResult res = cmd.Execute(ctx);
        CHECK(res.exitCode == 0);
        CHECK(out.str() == "banana\n");
        CHECK(err.str() == "");
    }

    SUBCASE("No match from stdin") {
        in.str("apple\ncherry\n");
        GrepCommand cmd({"banana"}); // grep "banana"
        CommandResult res = cmd.Execute(ctx);
        CHECK(res.exitCode == 1);
        CHECK(out.str() == "");
    }

    SUBCASE("Case insensitive match") {
        in.str("Apple\nBANANA\nCherry\n");
        GrepCommand cmd({"-i", "banana"}); // grep -i "banana"
        CommandResult res = cmd.Execute(ctx);
        CHECK(res.exitCode == 0);
        CHECK(out.str() == "BANANA\n");
    }

    SUBCASE("Whole word match") {
        in.str("apple pie\npineapple\napple\n");
        
        SUBCASE("Match word") {
            // Need to reset streams for subcase reuse or careful setup
            // Easier to just let each subcase run independently
            std::stringstream in2;
            std::stringstream out2;
            std::stringstream err2;
            CommandStreams streams2{in2, out2, err2};
            CommandContext ctx2{streams2, env};
            
            in2.str("apple pie\npineapple\napple\n");
            GrepCommand cmd({"-w", "apple"}); 
            CommandResult res = cmd.Execute(ctx2);
            CHECK(res.exitCode == 0);
            CHECK(out2.str() == "apple pie\napple\n");
        }
        
        SUBCASE("Fail partial word") {
            std::stringstream in2;
            std::stringstream out2;
            std::stringstream err2;
            CommandStreams streams2{in2, out2, err2};
            CommandContext ctx2{streams2, env};

            in2.str("pineapple\n");
            GrepCommand cmd({"-w", "apple"});
            CommandResult res = cmd.Execute(ctx2);
            CHECK(res.exitCode == 1); // No match
            CHECK(out2.str() == "");
        }
    }

    SUBCASE("Context printing (-A)") {
        in.str("1\n2\nmatch\n3\n4\n5\n");
        GrepCommand cmd({"-A", "1", "match"});
        CommandResult res = cmd.Execute(ctx);
        CHECK(res.exitCode == 0);
        CHECK(out.str() == "match\n3\n");
    }

    SUBCASE("Context printing overlap") {
        // "Contexts ... must not overlap" logic check
        // match1\n context1\n match2\n context2
        // grep -A 1 "match"
        in.str("match1\nmatch2\nend\n");
        GrepCommand cmd({"-A", "1", "match"});
        CommandResult res = cmd.Execute(ctx);
        CHECK(res.exitCode == 0);
        // match1 matches, print match1. Context=1 (print match2).
        // match2 matches, print match2. Context=1 (print end).
        // Should print match1, match2, end.
        CHECK(out.str() == "match1\nmatch2\nend\n");
    }

    SUBCASE("Invalid Regex") {
        GrepCommand cmd({"["}); // Invalid regex
        CommandResult res = cmd.Execute(ctx);
        CHECK(res.exitCode == 2);
        CHECK_FALSE(err.str().empty());
    }
}
