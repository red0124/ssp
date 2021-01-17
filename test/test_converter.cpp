#include <iostream>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../include/ss/converter.hpp"
#include "doctest.h"
#include <algorithm>

class buffer {
    constexpr static auto buff_size = 1024;
    char data_[buff_size];

public:
    char* operator()(const char* data) {
        memset(data_, '\0', buff_size);
        strcpy(data_, data);
        return data_;
    }
};

static buffer buff;

TEST_CASE("testing split") {
    ss::converter c;
    for (const auto& [s, expected, delim] :
         // clang-format off
                {std::tuple{"a,b,c,d", std::vector{"a", "b", "c", "d"}, ","},
                {"", {}, " "},
                {" x x x x | x ", {" x x x x ", " x "}, "|"},
                {"a::b::c::d", {"a", "b", "c", "d"}, "::"},
                {"x\t-\ty", {"x", "y"}, "\t-\t"},
                {"x", {"x"}, ","}} // clang-format on
    ) {
        auto split = c.split(buff(s), delim);
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
        auto tup = c.convert<int>(buff("5"));
        REQUIRE(c.valid());
        CHECK(tup == 5);
    }
    {
        auto tup = c.convert<int, void>(buff("5,junk"));
        REQUIRE(c.valid());
        CHECK(tup == 5);
    }
    {
        auto tup = c.convert<void, int>(buff("junk,5"));
        REQUIRE(c.valid());
        CHECK(tup == 5);
    }
    {
        auto tup = c.convert<int, void, void>(buff("5\njunk\njunk"), "\n");
        REQUIRE(c.valid());
        CHECK(tup == 5);
    }
    {
        // TODO make \t -> ' '
        auto tup = c.convert<void, int, void>(buff("junk\t5\tjunk"), "\t");
        REQUIRE(c.valid());
        CHECK(tup == 5);
    }
    {
        auto tup = c.convert<void, void, int>(buff("junk\tjunk\t5"), "\t");
        REQUIRE(c.valid());
        CHECK(tup == 5);
    }
    {
        auto tup =
            c.convert<void, void, std::optional<int>>(buff("junk\tjunk\t5"),
                                                      "\t");
        REQUIRE(c.valid());
        REQUIRE(tup.has_value());
        CHECK(tup == 5);
    }
    {
        auto tup = c.convert<int, double, void>(buff("5,6.6,junk"));
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{5, 6.6});
    }
    {
        auto tup = c.convert<int, void, double>(buff("5,junk,6.6"));
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{5, 6.6});
    }
    {
        auto tup = c.convert<void, int, double>(buff("junk;5;6.6"), ";");
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{5, 6.6});
    }
    {
        auto tup =
            c.convert<void, std::optional<int>, double>(buff("junk;5;6.6"),
                                                        ";");
        REQUIRE(c.valid());
        REQUIRE(std::get<0>(tup).has_value());
        CHECK(tup == std::tuple{5, 6.6});
    }
    {
        auto tup =
            c.convert<void, std::optional<int>, double>(buff("junk;5.4;6.6"),
                                                        ";");
        REQUIRE(c.valid());
        REQUIRE(!std::get<0>(tup).has_value());
        CHECK(tup == std::tuple{std::nullopt, 6.6});
    }
    {
        auto tup = c.convert<void, std::variant<int, double>,
                             double>(buff("junk;5;6.6"), ";");
        REQUIRE(c.valid());
        REQUIRE(std::holds_alternative<int>(std::get<0>(tup)));
        CHECK(tup == std::tuple{std::variant<int, double>{5}, 6.6});
    }
    {
        auto tup = c.convert<void, std::variant<int, double>,
                             double>(buff("junk;5.5;6.6"), ";");
        REQUIRE(c.valid());
        REQUIRE(std::holds_alternative<double>(std::get<0>(tup)));
        CHECK(tup == std::tuple{std::variant<int, double>{5.5}, 6.6});
    }
}

TEST_CASE("testing invalid conversions") {
    ss::converter c;

    c.convert<int>(buff(""));
    REQUIRE(!c.valid());

    c.convert<int, void>(buff(""));
    REQUIRE(!c.valid());

    c.convert<int, void>(buff(",junk"));
    REQUIRE(!c.valid());

    c.convert<void, int>(buff("junk,"));
    REQUIRE(!c.valid());

    c.convert<int>(buff("x"));
    REQUIRE(!c.valid());

    c.convert<int, void>(buff("x"));
    REQUIRE(!c.valid());

    c.convert<int, void>(buff("x,junk"));
    REQUIRE(!c.valid());

    c.convert<void, int>(buff("junk,x"));
    REQUIRE(!c.valid());

    c.convert<void, std::variant<int, double>, double>(buff("junk;.5.5;6"),
                                                       ";");
    REQUIRE(!c.valid());
}

TEST_CASE("testing ss:ax restriction (all except)") {
    ss::converter c;

    c.convert<ss::ax<int, 0>>(buff("0"));
    REQUIRE(!c.valid());

    c.convert<ss::ax<int, 0, 1, 2>>(buff("1"));
    REQUIRE(!c.valid());

    c.convert<void, char, ss::ax<int, 0, 1, 2>>(buff("junk,c,1"));
    REQUIRE(!c.valid());

    c.convert<ss::ax<int, 1>, char>(buff("1,c"));
    REQUIRE(!c.valid());
    {
        int tup = c.convert<ss::ax<int, 1>>(buff("3"));
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }
    {
        std::tuple<char, int> tup =
            c.convert<char, ss::ax<int, 1>>(buff("c,3"));
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{'c', 3});
    }
    {
        std::tuple<int, char> tup =
            c.convert<ss::ax<int, 1>, char>(buff("3,c"));
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{3, 'c'});
    }
}

TEST_CASE("testing ss:nx restriction (none except)") {
    ss::converter c;

    c.convert<ss::nx<int, 1>>(buff("3"));
    REQUIRE(!c.valid());

    c.convert<char, ss::nx<int, 1, 2, 69>>(buff("c,3"));
    REQUIRE(!c.valid());

    c.convert<ss::nx<int, 1>, char>(buff("3,c"));
    REQUIRE(!c.valid());

    {
        auto tup = c.convert<ss::nx<int, 3>>(buff("3"));
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }
    {
        auto tup = c.convert<ss::nx<int, 0, 1, 2>>(buff("2"));
        REQUIRE(c.valid());
        CHECK(tup == 2);
    }
    {
        auto tup =
            c.convert<char, void, ss::nx<int, 0, 1, 2>>(buff("c,junk,1"));
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{'c', 1});
    }
    {
        auto tup = c.convert<ss::nx<int, 1>, char>(buff("1,c"));
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{1, 'c'});
    }
}

TEST_CASE("testing ss:ir restriction (in range)") {
    ss::converter c;

    c.convert<ss::ir<int, 0, 2>>(buff("3"));
    REQUIRE(!c.valid());

    c.convert<char, ss::ir<int, 4, 69>>(buff("c,3"));
    REQUIRE(!c.valid());

    c.convert<ss::ir<int, 1, 2>, char>(buff("3,c"));
    REQUIRE(!c.valid());

    {
        auto tup = c.convert<ss::ir<int, 1, 5>>(buff("3"));
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }
    {
        auto tup = c.convert<ss::ir<int, 0, 2>>(buff("2"));
        REQUIRE(c.valid());
        CHECK(tup == 2);
    }
    {
        auto tup = c.convert<char, void, ss::ir<int, 0, 1>>(buff("c,junk,1"));
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{'c', 1});
    }
    {
        auto tup = c.convert<ss::ir<int, 1, 20>, char>(buff("1,c"));
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{1, 'c'});
    }
}

TEST_CASE("testing ss:oor restriction (out of range)") {
    ss::converter c;

    c.convert<ss::oor<int, 1, 5>>(buff("3"));
    REQUIRE(!c.valid());

    c.convert<ss::oor<int, 0, 2>>(buff("2"));
    REQUIRE(!c.valid());

    c.convert<char, ss::oor<int, 0, 1>, void>(buff("c,1,junk"));
    REQUIRE(!c.valid());

    c.convert<ss::oor<int, 1, 20>, char>(buff("1,c"));
    REQUIRE(!c.valid());

    {
        auto tup = c.convert<ss::oor<int, 0, 2>>(buff("3"));
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }

    {
        auto tup = c.convert<char, void, ss::oor<int, 4, 69>>(buff("c,junk,3"));
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{'c', 3});
    }

    {
        auto tup = c.convert<ss::oor<int, 1, 2>, char>(buff("3,c"));
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

    c.convert<ss::ne<std::string>>(buff(""));
    REQUIRE(!c.valid());

    c.convert<int, ss::ne<std::string>>(buff("3,"));
    REQUIRE(!c.valid());

    c.convert<ss::ne<std::string>, int>(buff(",3"));
    REQUIRE(!c.valid());

    c.convert<void, ss::ne<std::string>, int>(buff("junk,,3"));
    REQUIRE(!c.valid());

    c.convert<ss::ne<std::vector<int>>>(buff(""));
    REQUIRE(!c.valid());

    {
        auto tup = c.convert<ss::ne<std::string>>(buff("s"));
        REQUIRE(c.valid());
        CHECK(tup == "s");
    }
    {
        auto tup =
            c.convert<std::optional<int>, ss::ne<std::string>>(buff("1,s"));
        REQUIRE(c.valid());
        CHECK(tup == std::tuple{1, "s"});
    }
    {
        auto tup = c.convert<ss::ne<std::vector<int>>>(buff("{1 2 3}"));
        REQUIRE(c.valid());
        CHECK(tup == extracted_vector);
    }
}

TEST_CASE("testing ss:lt ss::lte ss::gt ss::gte restriction (in range)") {
    ss::converter c;

    c.convert<ss::lt<int, 3>>(buff("3"));
    REQUIRE(!c.valid());

    c.convert<ss::lt<int, 2>>(buff("3"));
    REQUIRE(!c.valid());

    c.convert<ss::gt<int, 3>>(buff("3"));
    REQUIRE(!c.valid());

    c.convert<ss::gt<int, 4>>(buff("3"));
    REQUIRE(!c.valid());

    c.convert<ss::lte<int, 2>>(buff("3"));
    REQUIRE(!c.valid());

    c.convert<ss::gte<int, 4>>(buff("3"));
    REQUIRE(!c.valid());

    {
        auto tup = c.convert<ss::lt<int, 4>>(buff("3"));
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }

    {
        auto tup = c.convert<ss::gt<int, 2>>(buff("3"));
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }

    {
        auto tup = c.convert<ss::lte<int, 4>>(buff("3"));
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }

    {
        auto tup = c.convert<ss::lte<int, 3>>(buff("3"));
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }

    {
        auto tup = c.convert<ss::gte<int, 2>>(buff("3"));
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }

    {
        auto tup = c.convert<ss::gte<int, 3>>(buff("3"));
        REQUIRE(c.valid());
        CHECK(tup == 3);
    }
}

TEST_CASE("testing error mode") {
    ss::converter c;

    c.convert<int>(buff("junk"));
    CHECK(!c.valid());
    CHECK(c.error_msg().empty());

    c.set_error_mode(ss::error_mode::error_string);
    c.convert<int>(buff("junk"));
    CHECK(!c.valid());
    CHECK(!c.error_msg().empty());
}
