#include "test_helpers.hpp"
#include <algorithm>

#define SSP_DISABLE_FAST_FLOAT
#include <ss/extract.hpp>

TEST_CASE(
    "testing extract functions for floating point values without fast float") {
    CHECK_FLOATING_CONVERSION(123.456, float);
    CHECK_FLOATING_CONVERSION(123.456, double);

    CHECK_FLOATING_CONVERSION(69, float);
    CHECK_FLOATING_CONVERSION(69, double);

    CHECK_FLOATING_CONVERSION(420., float);
    CHECK_FLOATING_CONVERSION(420., double);

    CHECK_FLOATING_CONVERSION(0.123, float);
    CHECK_FLOATING_CONVERSION(0.123, double);

    CHECK_FLOATING_CONVERSION(123e4, float);
    CHECK_FLOATING_CONVERSION(123e4, double);
}

TEST_CASE("extract test functions for numbers with invalid inputs without fast "
          "float") {
    // floating pint for int
    CHECK_INVALID_CONVERSION("123.4", int);

    // random input for float
    CHECK_INVALID_CONVERSION("xxx1", float);
}

TEST_CASE("extract test functions for std::variant without fast float") {
    {
        std::string s = "22";
        {
            std::variant<int, double, std::string> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, double);
            CHECK_NOT_VARIANT(var, std::string);
            REQUIRE_VARIANT(var, 22, int);
        }
        {
            std::variant<double, int, std::string> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, int);
            CHECK_NOT_VARIANT(var, std::string);
            REQUIRE_VARIANT(var, 22, double);
        }
        {
            std::variant<std::string, double, int> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, int);
            CHECK_NOT_VARIANT(var, double);
            REQUIRE_VARIANT(var, "22", std::string);
        }
        {
            std::variant<int> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            REQUIRE_VARIANT(var, 22, int);
        }
    }
    {
        std::string s = "22.2";
        {
            std::variant<int, double, std::string> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, int);
            CHECK_NOT_VARIANT(var, std::string);
            REQUIRE_VARIANT(var, 22.2, double);
        }
        {
            std::variant<double, int, std::string> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, int);
            CHECK_NOT_VARIANT(var, std::string);
            REQUIRE_VARIANT(var, 22.2, double);
        }
        {
            std::variant<std::string, double, int> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, int);
            CHECK_NOT_VARIANT(var, double);
            REQUIRE_VARIANT(var, "22.2", std::string);
        }
    }
    {
        std::string s = "2.2.2";
        {
            std::variant<int, double, std::string> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, int);
            CHECK_NOT_VARIANT(var, double);
            REQUIRE_VARIANT(var, "2.2.2", std::string);
        }
        {
            std::variant<double, std::string, int> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, int);
            CHECK_NOT_VARIANT(var, double);
            REQUIRE_VARIANT(var, "2.2.2", std::string);
        }
        {
            std::variant<std::string, double, int> var;
            REQUIRE(ss::extract(s.c_str(), s.c_str() + s.size(), var));
            CHECK_NOT_VARIANT(var, int);
            CHECK_NOT_VARIANT(var, double);
            REQUIRE_VARIANT(var, "2.2.2", std::string);
        }
        {
            std::variant<int, double> var;
            REQUIRE_FALSE(ss::extract(s.c_str(), s.c_str() + s.size(), var));

            REQUIRE_VARIANT(var, int{}, int);
            CHECK_NOT_VARIANT(var, double);
        }
        {
            std::variant<double, int> var;
            REQUIRE_FALSE(ss::extract(s.c_str(), s.c_str() + s.size(), var));

            REQUIRE_VARIANT(var, double{}, double);
            CHECK_NOT_VARIANT(var, int);
        }
        {
            std::variant<int> var;
            REQUIRE_FALSE(ss::extract(s.c_str(), s.c_str() + s.size(), var));

            REQUIRE_VARIANT(var, int{}, int);
        }
    }
}
