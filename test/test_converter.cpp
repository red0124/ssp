#include "../include/ss/converter.hpp"
#include "doctest.h"
#include <algorithm>

TEST_CASE("testing split") {
    ss::converter c;
    for (const auto& [s, expected, delim] :
         // clang-format off
                {std::tuple{"a,b,c,d", std::vector{"a", "b", "c", "d"}, ","},
                {"", {}, " "},
                {"a,b,c", {"a", "b", "c"}, ""},
                {" x x x x | x ", {" x x x x ", " x "}, "|"},
                {"a::b::c::d", {"a", "b", "c", "d"}, "::"},
                {"x\t-\ty", {"x", "y"}, "\t-\t"},
                {"x", {"x"}, ","}} // clang-format on
    ) {
        auto split = c.split(s, delim);
        CHECK(split.size() == expected.size());
        for (size_t i = 0; i < split.size(); ++i) {
            auto s = std::string(split[i].first, split[i].second);
            CHECK(s == expected[i]);
        }
    }
}

TEST_CASE("testing valid conversions") {
    ss::converter c;

    {
        auto tup = c.convert<int>("5");
        REQUIRE(c.valid());
        CHECK(tup == 5);
    }
    {
        auto tup = c.convert<int, void>("5,junk");
        REQUIRE(c.valid());
        CHECK(tup == 5);
    }
    {
        auto tup = c.convert<void, int>("junk,5");
        REQUIRE(c.valid());
        CHECK(tup == 5);
    }
    {
        auto tup = c.convert<int, void, void>("5\njunk\njunk", "\n");
        REQUIRE(c.valid());
        CHECK(tup == 5);
    }
    {
        auto tup = c.convert<void, int, void>("junk 5 junk", " ");
        REQUIRE(c.valid());
        CHECK(tup == 5);
    }
    {
        auto tup = c.convert<void, void, int>("junk\tjunk\t5", "\t");
        REQUIRE(c.valid());
        CHECK(tup == 5);
    }
    {
        auto tup =
            c.convert<void, void, std::optional<int>>("junk\tjunk\t5", "\t");
        REQUIRE(c.valid());
        REQUIRE(tup.has_value());
        CHECK(tup == 5);
    }
    {
        auto tup = c.convert<int, double, void>("5,6.6,junk");
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{5, 6.6});
    }
    {
        auto tup = c.convert<int, void, double>("5,junk,6.6");
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{5, 6.6});
    }
    {
        auto tup = c.convert<void, int, double>("junk;5;6.6", ";");
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{5, 6.6});
    }
    {
        auto tup =
            c.convert<void, std::optional<int>, double>("junk;5;6.6", ";");
        REQUIRE(c.valid());
        REQUIRE(std::get<0>(tup).has_value());
        CHECK(tup == std::tuple{5, 6.6});
    }
    {
        auto tup =
            c.convert<void, std::optional<int>, double>("junk;5.4;6.6", ";");
        REQUIRE(c.valid());
        REQUIRE(!std::get<0>(tup).has_value());
        CHECK(tup == std::tuple{std::nullopt, 6.6});
    }
    {
        auto tup =
            c.convert<void, std::variant<int, double>, double>("junk;5;6.6",
                                                               ";");
        REQUIRE(c.valid());
        REQUIRE(std::holds_alternative<int>(std::get<0>(tup)));
        CHECK(tup == std::tuple{std::variant<int, double>{5}, 6.6});
    }
    {
        auto tup =
            c.convert<void, std::variant<int, double>, double>("junk;5.5;6.6",
                                                               ";");
        REQUIRE(c.valid());
        REQUIRE(std::holds_alternative<double>(std::get<0>(tup)));
        CHECK(tup == std::tuple{std::variant<int, double>{5.5}, 6.6});
    }
}

TEST_CASE("testing invalid conversions") {
    ss::converter c;

    c.convert<int>("");
    REQUIRE(!c.valid());

    c.convert<int, void>("");
    REQUIRE(!c.valid());

    c.convert<int, void>(",junk");
    REQUIRE(!c.valid());

    c.convert<void, int>("junk,");
    REQUIRE(!c.valid());

    c.convert<int>("x");
    REQUIRE(!c.valid());

    c.convert<int, void>("x");
    REQUIRE(!c.valid());

    c.convert<int, void>("x,junk");
    REQUIRE(!c.valid());

    c.convert<void, int>("junk,x");
    REQUIRE(!c.valid());

    c.convert<void, std::variant<int, double>, double>("junk;.5.5;6", ";");
    REQUIRE(!c.valid());
}

TEST_CASE("testing ss:ax restriction (all except)") {
    ss::converter c;

    c.convert<ss::ax<int, 0>>("0");
    REQUIRE(!c.valid());

    c.convert<ss::ax<int, 0, 1, 2>>("1");
    REQUIRE(!c.valid());

    c.convert<void, char, ss::ax<int, 0, 1, 2>>("junk,c,1");
    REQUIRE(!c.valid());

    c.convert<ss::ax<int, 1>, char>("1,c");
    REQUIRE(!c.valid());
    {
        int tup = c.convert<ss::ax<int, 1>>("3");
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }
    {
        std::tuple<char, int> tup = c.convert<char, ss::ax<int, 1>>("c,3");
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{'c', 3});
    }
    {
        std::tuple<int, char> tup = c.convert<ss::ax<int, 1>, char>("3,c");
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{3, 'c'});
    }
}

TEST_CASE("testing ss:nx restriction (none except)") {
    ss::converter c;

    c.convert<ss::nx<int, 1>>("3");
    REQUIRE(!c.valid());

    c.convert<char, ss::nx<int, 1, 2, 69>>("c,3");
    REQUIRE(!c.valid());

    c.convert<ss::nx<int, 1>, char>("3,c");
    REQUIRE(!c.valid());

    {
        auto tup = c.convert<ss::nx<int, 3>>("3");
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }
    {
        auto tup = c.convert<ss::nx<int, 0, 1, 2>>("2");
        REQUIRE(c.valid());
        CHECK(tup == 2);
    }
    {
        auto tup = c.convert<char, void, ss::nx<int, 0, 1, 2>>("c,junk,1");
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{'c', 1});
    }
    {
        auto tup = c.convert<ss::nx<int, 1>, char>("1,c");
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{1, 'c'});
    }
}

TEST_CASE("testing ss:ir restriction (in range)") {
    ss::converter c;

    c.convert<ss::ir<int, 0, 2>>("3");
    REQUIRE(!c.valid());

    c.convert<char, ss::ir<int, 4, 69>>("c,3");
    REQUIRE(!c.valid());

    c.convert<ss::ir<int, 1, 2>, char>("3,c");
    REQUIRE(!c.valid());

    {
        auto tup = c.convert<ss::ir<int, 1, 5>>("3");
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }
    {
        auto tup = c.convert<ss::ir<int, 0, 2>>("2");
        REQUIRE(c.valid());
        CHECK(tup == 2);
    }
    {
        auto tup = c.convert<char, void, ss::ir<int, 0, 1>>("c,junk,1");
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{'c', 1});
    }
    {
        auto tup = c.convert<ss::ir<int, 1, 20>, char>("1,c");
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{1, 'c'});
    }
}

TEST_CASE("testing ss:oor restriction (out of range)") {
    ss::converter c;

    c.convert<ss::oor<int, 1, 5>>("3");
    REQUIRE(!c.valid());

    c.convert<ss::oor<int, 0, 2>>("2");
    REQUIRE(!c.valid());

    c.convert<char, ss::oor<int, 0, 1>, void>("c,1,junk");
    REQUIRE(!c.valid());

    c.convert<ss::oor<int, 1, 20>, char>("1,c");
    REQUIRE(!c.valid());

    {
        auto tup = c.convert<ss::oor<int, 0, 2>>("3");
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }

    {
        auto tup = c.convert<char, void, ss::oor<int, 4, 69>>("c,junk,3");
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{'c', 3});
    }

    {
        auto tup = c.convert<ss::oor<int, 1, 2>, char>("3,c");
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{3, 'c'});
    }
}

const std::vector<int> extracted_vector = {1, 2, 3};

// custom extract
template <>
inline bool ss::extract(const char* begin, const char* end,
                        std::vector<int>& value) {
    if (begin == end) {
        return false;
    }
    value = extracted_vector;
    return true;
}

TEST_CASE("testing ss:ne restriction (not empty)") {
    ss::converter c;

    c.convert<ss::ne<std::string>>("");
    REQUIRE(!c.valid());

    c.convert<int, ss::ne<std::string>>("3,");
    REQUIRE(!c.valid());

    c.convert<ss::ne<std::string>, int>(",3");
    REQUIRE(!c.valid());

    c.convert<void, ss::ne<std::string>, int>("junk,,3");
    REQUIRE(!c.valid());

    c.convert<ss::ne<std::vector<int>>>("");
    REQUIRE(!c.valid());

    {
        auto tup = c.convert<ss::ne<std::string>>("s");
        REQUIRE(c.valid());
        CHECK(tup == "s");
    }
    {
        auto tup = c.convert<std::optional<int>, ss::ne<std::string>>("1,s");
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{1, "s"});
    }
    {
        auto tup = c.convert<ss::ne<std::vector<int>>>("{1 2 3}");
        REQUIRE(c.valid());
        CHECK(tup == extracted_vector);
    }
}

TEST_CASE("testing ss:lt ss::lte ss::gt ss::gte restriction (in range)") {
    ss::converter c;

    c.convert<ss::lt<int, 3>>("3");
    REQUIRE(!c.valid());

    c.convert<ss::lt<int, 2>>("3");
    REQUIRE(!c.valid());

    c.convert<ss::gt<int, 3>>("3");
    REQUIRE(!c.valid());

    c.convert<ss::gt<int, 4>>("3");
    REQUIRE(!c.valid());

    c.convert<ss::lte<int, 2>>("3");
    REQUIRE(!c.valid());

    c.convert<ss::gte<int, 4>>("3");
    REQUIRE(!c.valid());

    {
        auto tup = c.convert<ss::lt<int, 4>>("3");
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }

    {
        auto tup = c.convert<ss::gt<int, 2>>("3");
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }

    {
        auto tup = c.convert<ss::lte<int, 4>>("3");
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }

    {
        auto tup = c.convert<ss::lte<int, 3>>("3");
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }

    {
        auto tup = c.convert<ss::gte<int, 2>>("3");
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }

    {
        auto tup = c.convert<ss::gte<int, 3>>("3");
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }
}

TEST_CASE("testing error mode") {
    ss::converter c;

    c.convert<int>("junk");
    CHECK(!c.valid());
    CHECK(c.error_msg().empty());

    c.set_error_mode(ss::error_mode::error_string);
    c.convert<int>("junk");
    CHECK(!c.valid());
    CHECK(!c.error_msg().empty());
}
