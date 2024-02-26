#include "test_helpers.hpp"
#include <algorithm>
#include <ss/extract.hpp>

namespace {

template <typename T>
struct numeric_limits : public std::numeric_limits<T> {};

template <typename T>
struct numeric_limits<ss::numeric_wrapper<T>> : public std::numeric_limits<T> {
};

template <typename T>
struct is_signed : public std::is_signed<T> {};

template <>
struct is_signed<ss::int8> : public std::true_type {};

template <typename T>
struct is_unsigned : public std::is_unsigned<T> {};

template <>
struct is_unsigned<ss::uint8> : public std::true_type {};

} /* namespace */

static_assert(is_signed<ss::int8>::value);
static_assert(is_unsigned<ss::uint8>::value);

TEST_CASE("testing extract functions for floating point values") {
    CHECK_FLOATING_CONVERSION(123.456, float);
    CHECK_FLOATING_CONVERSION(123.456, double);

    CHECK_FLOATING_CONVERSION(59, float);
    CHECK_FLOATING_CONVERSION(59, double);

    CHECK_FLOATING_CONVERSION(4210., float);
    CHECK_FLOATING_CONVERSION(4210., double);

    CHECK_FLOATING_CONVERSION(0.123, float);
    CHECK_FLOATING_CONVERSION(0.123, double);

    CHECK_FLOATING_CONVERSION(123e4, float);
    CHECK_FLOATING_CONVERSION(123e4, double);
}

#define CHECK_DECIMAL_CONVERSION(input, type)                                  \
    {                                                                          \
        std::string s = #input;                                                \
        type value;                                                            \
        bool valid = ss::extract(s.c_str(), s.c_str() + s.size(), value);      \
        REQUIRE(valid);                                                        \
        CHECK_EQ(value, type(input));                                          \
    }                                                                          \
    /* check negative too */                                                   \
    if (is_signed<type>::value) {                                              \
        std::string s = std::string("-") + #input;                             \
        type value;                                                            \
        bool valid = ss::extract(s.c_str(), s.c_str() + s.size(), value);      \
        REQUIRE(valid);                                                        \
        CHECK_EQ(value, type(-input));                                         \
    }

using us = unsigned short;
using ui = unsigned int;
using ul = unsigned long;
using ll = long long;
using ull = unsigned long long;

TEST_CASE("extract test functions for decimal values") {
    CHECK_DECIMAL_CONVERSION(12, ss::int8);
    CHECK_DECIMAL_CONVERSION(12, ss::uint8);
    CHECK_DECIMAL_CONVERSION(1234, short);
    CHECK_DECIMAL_CONVERSION(1234, us);
    CHECK_DECIMAL_CONVERSION(1234, int);
    CHECK_DECIMAL_CONVERSION(1234, ui);
    CHECK_DECIMAL_CONVERSION(1234, long);
    CHECK_DECIMAL_CONVERSION(1234, ul);
    CHECK_DECIMAL_CONVERSION(1234, ll);
    CHECK_DECIMAL_CONVERSION(1234567891011, ull);
}

TEST_CASE("extract test functions for numbers with invalid inputs") {
    // negative unsigned value for numeric_wrapper
    CHECK_INVALID_CONVERSION("-12", ss::uint8);

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

TEST_CASE_TEMPLATE(
    "extract test functions for numbers with out of range inputs", T, short, us,
    int, ui, long, ul, ll, ull, ss::uint8) {
    {
        std::string s = std::to_string(numeric_limits<T>::max());
        auto t = ss::to_num<T>(s.c_str(), s.c_str() + s.size());
        CHECK(t.has_value());
        for (auto& i : s) {
            if (i != '9' && i != '.') {
                i = '9';
                break;
            }
        }
        t = ss::to_num<T>(s.c_str(), s.c_str() + s.size());
        CHECK_FALSE(t.has_value());
    }
    {
        std::string s = std::to_string(numeric_limits<T>::min());
        auto t = ss::to_num<T>(s.c_str(), s.c_str() + s.size());
        CHECK(t.has_value());
        for (auto& i : s) {
            if (is_signed<T>::value && i != '9' && i != '.') {
                i = '9';
                break;
            } else if (is_unsigned<T>::value) {
                s = "-1";
                break;
            }
        }
        t = ss::to_num<T>(s.c_str(), s.c_str() + s.size());
        CHECK_FALSE(t.has_value());
    }
}

TEST_CASE("extract test functions for boolean values") {
    for (const auto& [b, s] : {std::pair<bool, std::string>{true, "1"},
                               {false, "0"},
                               {true, "true"},
                               {false, "false"}}) {
        bool v;
        REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), v));
        CHECK_EQ(v, b);
    }

    for (const std::string s : {"2", "tru", "truee", "xxx", ""}) {
        bool v;
        CHECK_FALSE(ss::extract(s.c_str(), s.c_str() + s.size(), v));
    }
}

TEST_CASE("extract test functions for char values") {
    for (const auto& [c, s] :
         {std::pair<char, std::string>{'a', "a"}, {'x', "x"}, {' ', " "}}) {
        char v;
        REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), v));
        CHECK_EQ(v, c);
    }

    for (const std::string s : {"aa", "xxx", ""}) {
        char v;
        CHECK_FALSE(ss::extract(s.c_str(), s.c_str() + s.size(), v));
    }
}

TEST_CASE_TEMPLATE("extract test functions for std::optional", T, int,
                   ss::int8) {
    for (const auto& [i, s] : {std::pair<std::optional<T>, std::string>{1, "1"},
                               {69, "69"},
                               {-4, "-4"}}) {
        std::optional<T> v;
        REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), v));
        REQUIRE(v.has_value());
        CHECK_EQ(*v, i);
    }

    for (const auto& [c, s] :
         {std::pair<std::optional<char>, std::string>{'a', "a"},
          {'x', "x"},
          {' ', " "}}) {
        std::optional<char> v;
        REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), v));
        REQUIRE(v.has_value());
        CHECK_EQ(*v, c);
    }

    for (const std::string s : {"aa", "xxx", ""}) {
        std::optional<T> v;
        REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), v));
        CHECK_FALSE(v.has_value());
    }

    for (const std::string s : {"aa", "xxx", ""}) {
        std::optional<char> v;
        REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), v));
        CHECK_FALSE(v.has_value());
    }
}

TEST_CASE_TEMPLATE("extract test functions for std::variant", T, int,
                   ss::uint8) {
    {
        std::string s = "22";
        {
            std::variant<T, double, std::string> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, double);
            CHECK_NOT_VARIANT(var, std::string);
            REQUIRE_VARIANT(var, 22, T);
        }
        {
            std::variant<double, T, std::string> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, T);
            CHECK_NOT_VARIANT(var, std::string);
            REQUIRE_VARIANT(var, 22, double);
        }
        {
            std::variant<std::string, double, T> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, T);
            CHECK_NOT_VARIANT(var, double);
            REQUIRE_VARIANT(var, "22", std::string);
        }
        {
            std::variant<T> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            REQUIRE_VARIANT(var, 22, T);
        }
    }
    {
        std::string s = "22.2";
        {
            std::variant<T, double, std::string> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, T);
            CHECK_NOT_VARIANT(var, std::string);
            REQUIRE_VARIANT(var, 22.2, double);
        }
        {
            std::variant<double, T, std::string> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, T);
            CHECK_NOT_VARIANT(var, std::string);
            REQUIRE_VARIANT(var, 22.2, double);
        }
        {
            std::variant<std::string, double, T> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, T);
            CHECK_NOT_VARIANT(var, double);
            REQUIRE_VARIANT(var, "22.2", std::string);
        }
    }
    {
        std::string s = "2.2.2";
        {
            std::variant<T, double, std::string> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, T);
            CHECK_NOT_VARIANT(var, double);
            REQUIRE_VARIANT(var, "2.2.2", std::string);
        }
        {
            std::variant<double, std::string, T> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, T);
            CHECK_NOT_VARIANT(var, double);
            REQUIRE_VARIANT(var, "2.2.2", std::string);
        }
        {
            std::variant<std::string, double, T> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, T);
            CHECK_NOT_VARIANT(var, double);
            REQUIRE_VARIANT(var, "2.2.2", std::string);
        }
        {
            std::variant<T, double> var;
            REQUIRE_FALSE(ss::extract(s.c_str(), s.c_str() + s.size(), var));

            REQUIRE_VARIANT(var, T{}, T);
            CHECK_NOT_VARIANT(var, double);
        }
        {
            std::variant<double, T> var;
            REQUIRE_FALSE(ss::extract(s.c_str(), s.c_str() + s.size(), var));

            REQUIRE_VARIANT(var, double{}, double);
            CHECK_NOT_VARIANT(var, T);
        }
        {
            std::variant<T> var;
            REQUIRE_FALSE(ss::extract(s.c_str(), s.c_str() + s.size(), var));

            REQUIRE_VARIANT(var, T{}, T);
        }
    }
}

TEST_CASE("extract test with long number string") {
    {
        std::string string_num =
            std::string(20, '1') + "." + std::string(20, '2');

        CHECK_FLOATING_CONVERSION_LONG_NUMBER(string_num, float, stof);
        CHECK_FLOATING_CONVERSION_LONG_NUMBER(string_num, double, stod);
    }

    {
        std::string string_num =
            std::string(50, '1') + "." + std::string(50, '2');

        CHECK_FLOATING_CONVERSION_LONG_NUMBER(string_num, double, stod);
    }
}
