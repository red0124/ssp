#pragma once
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#ifdef CMAKE_GITHUB_CI
#include <doctest/doctest.h>
#else
#include <doctest.h>
#endif

namespace {
struct buffer {
    std::string data_;

    char* operator()(const std::string& data) {
        data_ = data;
        return data_.data();
    }

    char* append(const std::string& data) {
        data_ += data;
        return data_.data();
    }

    char* append_overwrite_last(const std::string& data, size_t size) {
        data_.resize(data_.size() - size);
        return append(data);
    }
};

[[maybe_unused]] inline buffer buff;

std::string time_now_rand() {
    std::stringstream ss;
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    ss << std::put_time(&tm, "%d%m%Y%H%M%S");
    srand(time(nullptr));
    return ss.str() + std::to_string(rand());
}

struct unique_file_name {
    static inline int i = 0;

    const std::string name;

    unique_file_name(const std::string& test)
        : name{"random_" + test + "_" + std::to_string(i++) + "_" +
               time_now_rand() + "_file.csv"} {
    }

    ~unique_file_name() {
        std::filesystem::remove(name);
    }
};

#define CHECK_FLOATING_CONVERSION(input, type)                                 \
    {                                                                          \
        auto eps = std::numeric_limits<type>::min();                           \
        std::string s = #input;                                                \
        auto t = ss::to_num<type>(s.c_str(), s.c_str() + s.size());            \
        REQUIRE(t.has_value());                                                \
        CHECK_LT(std::abs(t.value() - type(input)), eps);                      \
    }                                                                          \
    {                                                                          \
        /* check negative too */                                               \
        auto eps = std::numeric_limits<type>::min();                           \
        auto s = std::string("-") + #input;                                    \
        auto t = ss::to_num<type>(s.c_str(), s.c_str() + s.size());            \
        REQUIRE(t.has_value());                                                \
        CHECK_LT(std::abs(t.value() - type(-input)), eps);                     \
    }

#define CHECK_FLOATING_CONVERSION_LONG_NUMBER(STRING_NUMBER, TYPE, CONVERTER)  \
    {                                                                          \
        auto begin = STRING_NUMBER.c_str();                                    \
        auto end = begin + STRING_NUMBER.size();                               \
                                                                               \
        auto number = ss::to_num<TYPE>(begin, end);                            \
        REQUIRE(number.has_value());                                           \
                                                                               \
        auto expected_number = CONVERTER(STRING_NUMBER);                       \
        CHECK_EQ(number.value(), expected_number);                             \
    }

#define CHECK_INVALID_CONVERSION(input, type)                                  \
    {                                                                          \
        std::string s = input;                                                 \
        auto t = ss::to_num<type>(s.c_str(), s.c_str() + s.size());            \
        CHECK_FALSE(t.has_value());                                            \
    }

#define REQUIRE_VARIANT(var, el, type)                                         \
    {                                                                          \
        auto ptr = std::get_if<type>(&var);                                    \
        REQUIRE(ptr);                                                          \
        REQUIRE_EQ(el, *ptr);                                                  \
    }

#define CHECK_NOT_VARIANT(var, type) CHECK(!std::holds_alternative<type>(var));

#define REQUIRE_EXCEPTION(...)                                                 \
    try {                                                                      \
        __VA_ARGS__;                                                           \
        FAIL("Expected exception");                                            \
    } catch (ss::exception & e) {                                              \
        CHECK_FALSE(std::string{e.what()}.empty());                            \
    }

template <typename T>
std::vector<std::vector<T>> vector_combinations(const std::vector<T>& v,
                                                size_t n) {
    std::vector<std::vector<T>> ret;
    if (n <= 1) {
        for (const auto& i : v) {
            ret.push_back({i});
        }
        return ret;
    }

    auto inner_combinations = vector_combinations(v, n - 1);
    for (const auto& i : v) {
        for (auto j : inner_combinations) {
            j.insert(j.begin(), i);
            ret.push_back(move(j));
        }
    }
    return ret;
}
} /* namespace */
