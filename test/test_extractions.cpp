#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../include/ss/extract.hpp"
#include "doctest.h"
#include <algorithm>

constexpr auto eps = 0.000001;
using ld = long double;

#define CHECK_FLOATING_CONVERSION(input, type)                                 \
        {                                                                      \
                std::string s = #input;                                        \
                auto t = ss::to_num<type>(s.c_str(), s.c_str() + s.size());    \
                REQUIRE(t.has_value());                                        \
                CHECK(std::abs(t.value() - type(input)) < eps);                \
        }                                                                      \
        {                                                                      \
                /* check negative too */                                       \
                auto s = std::string("-") + #input;                            \
                auto t = ss::to_num<type>(s.c_str(), s.c_str() + s.size());    \
                REQUIRE(t.has_value());                                        \
                CHECK(std::abs(t.value() - type(-input)) < eps);               \
        }

TEST_CASE("testing extract functions for floating point values") {
        CHECK_FLOATING_CONVERSION(123.456, float);
        CHECK_FLOATING_CONVERSION(123.456, double);
        CHECK_FLOATING_CONVERSION(123.456, ld);

        CHECK_FLOATING_CONVERSION(69, float);
        CHECK_FLOATING_CONVERSION(69, double);
        CHECK_FLOATING_CONVERSION(69, ld);

        CHECK_FLOATING_CONVERSION(420., float);
        CHECK_FLOATING_CONVERSION(420., double);
        CHECK_FLOATING_CONVERSION(420., ld);

        CHECK_FLOATING_CONVERSION(0.123, float);
        CHECK_FLOATING_CONVERSION(0.123, double);
        CHECK_FLOATING_CONVERSION(0.123, ld);

        CHECK_FLOATING_CONVERSION(123e4, float);
        CHECK_FLOATING_CONVERSION(123e4, double);
        CHECK_FLOATING_CONVERSION(123e4, ld);
}

#define CHECK_DECIMAL_CONVERSION(input, type)                                  \
        {                                                                      \
                std::string s = #input;                                        \
                auto t = ss::to_num<type>(s.c_str(), s.c_str() + s.size());    \
                REQUIRE(t.has_value());                                        \
                CHECK(t.value() == type(input));                               \
        }                                                                      \
        {                                                                      \
                /* check negative too */                                       \
                if (std::is_signed_v<type>) {                                  \
                        auto s = std::string("-") + #input;                    \
                        auto t =                                               \
                            ss::to_num<type>(s.c_str(), s.c_str() + s.size()); \
                        REQUIRE(t.has_value());                                \
                        CHECK(t.value() == type(-input));                      \
                }                                                              \
        }

using us = unsigned short;
using ui = unsigned int;
using ul = unsigned long;
using ll = long long;
using ull = unsigned long long;

TEST_CASE("testing extract functions for decimal values") {
        CHECK_DECIMAL_CONVERSION(1234, short);
        CHECK_DECIMAL_CONVERSION(1234, us);
        CHECK_DECIMAL_CONVERSION(1234, int);
        CHECK_DECIMAL_CONVERSION(1234, ui);
        CHECK_DECIMAL_CONVERSION(1234, long);
        CHECK_DECIMAL_CONVERSION(1234, ul);
        CHECK_DECIMAL_CONVERSION(1234, ll);
        CHECK_DECIMAL_CONVERSION(1234567891011, ull);
}

#define CHECK_INVALID_CONVERSION(input, type)                                  \
        {                                                                      \
                std::string s = input;                                         \
                auto t = ss::to_num<type>(s.c_str(), s.c_str() + s.size());    \
                CHECK(!t.has_value());                                         \
        }

TEST_CASE("testing extract functions for numbers with invalid inputs") {
        // negative unsigned value
        CHECK_INVALID_CONVERSION("-1234", ul);

        // floating pint for int
        CHECK_INVALID_CONVERSION("123.4", int);

        // random input for float
        CHECK_INVALID_CONVERSION("xxx1", float);

        // random input for int
        CHECK_INVALID_CONVERSION("xxx1", int);

        // empty field for int
        CHECK_INVALID_CONVERSION("", int);
}

#define CHECK_OUT_OF_RANGE_CONVERSION(type)                                    \
        {                                                                      \
                std::string s =                                                \
                    std::to_string(std::numeric_limits<type>::max());          \
                auto t = ss::to_num<type>(s.c_str(), s.c_str() + s.size());    \
                CHECK(t.has_value());                                          \
                for (auto& i : s) {                                            \
                        if (i != '9' && i != '.') {                            \
                                i = '9';                                       \
                                break;                                         \
                        }                                                      \
                }                                                              \
                t = ss::to_num<type>(s.c_str(), s.c_str() + s.size());         \
                CHECK(!t.has_value());                                         \
        }                                                                      \
        {                                                                      \
                std::string s =                                                \
                    std::to_string(std::numeric_limits<type>::min());          \
                auto t = ss::to_num<type>(s.c_str(), s.c_str() + s.size());    \
                CHECK(t.has_value());                                          \
                for (auto& i : s) {                                            \
                        if (std::is_signed_v<type> && i != '9' && i != '.') {  \
                                i = '9';                                       \
                                break;                                         \
                        } else if (std::is_unsigned_v<type>) {                 \
                                s = "-1";                                      \
                                break;                                         \
                        }                                                      \
                }                                                              \
                t = ss::to_num<type>(s.c_str(), s.c_str() + s.size());         \
                CHECK(!t.has_value());                                         \
        }

TEST_CASE("testing extract functions for numbers with out of range inputs") {
        CHECK_OUT_OF_RANGE_CONVERSION(short);
        CHECK_OUT_OF_RANGE_CONVERSION(us);
        CHECK_OUT_OF_RANGE_CONVERSION(int);
        CHECK_OUT_OF_RANGE_CONVERSION(ui);
        CHECK_OUT_OF_RANGE_CONVERSION(long);
        CHECK_OUT_OF_RANGE_CONVERSION(ul);
        CHECK_OUT_OF_RANGE_CONVERSION(ll);
        CHECK_OUT_OF_RANGE_CONVERSION(ull);
}

TEST_CASE("testing extract functions for boolean values") {
        for (const auto& [b, s] : {std::pair<bool, std::string>{true, "1"},
                                   {false, "0"},
                                   {true, "true"},
                                   {false, "false"}}) {
                bool v;
                REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), v));
                CHECK(v == b);
        }

        for (const std::string& s : {"2", "tru", "truee", "xxx", ""}) {
                bool v;
                REQUIRE(!ss::extract(s.c_str(), s.c_str() + s.size(), v));
        }
}

TEST_CASE("testing extract functions for char values") {
        for (const auto& [c, s] :
             {std::pair<char, std::string>{'a', "a"}, {'x', "x"}, {' ', " "}}) {
                char v;
                REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), v));
                CHECK(v == c);
        }

        for (const std::string& s : {"aa", "xxx", ""}) {
                char v;
                REQUIRE(!ss::extract(s.c_str(), s.c_str() + s.size(), v));
        }
}
