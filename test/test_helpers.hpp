#pragma once
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ss/common.hpp>
#include <ss/setup.hpp>
#include <sstream>
#include <string>
#include <vector>

#ifdef CMAKE_GITHUB_CI
#include <doctest/doctest.h>
#else
#include <doctest.h>
#endif

namespace ss {
template <typename... Ts>
class parser;
} /* ss */

namespace {

struct bool_error {};

template <typename T, typename U = bool_error>
struct config {
    using BufferMode = T;
    using ErrorMode = U;

    constexpr static auto ThrowOnError = std::is_same_v<U, ss::throw_on_error>;
    constexpr static auto StringError = std::is_same_v<U, ss::string_error>;
};

#define ParserOptionCombinations                                               \
    config<std::true_type>, config<std::true_type, ss::string_error>,          \
        config<std::true_type, ss::throw_on_error>, config<std::false_type>,   \
        config<std::false_type, ss::string_error>,                             \
        config<std::false_type, ss::throw_on_error>

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

[[maybe_unused]] std::string time_now_rand() {
    srand(time(nullptr));
    std::stringstream ss;
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    ss << std::put_time(&tm, "%d%m%Y%H%M%S");
    srand(time(nullptr));
    return ss.str() + std::to_string(rand());
}

struct unique_file_name {
    static inline int i = 0;

    std::string name;

    unique_file_name(const std::string& test) {
        do {
            name = "random_file_test_" + test + "_" + std::to_string(i++) +
                   "_" + time_now_rand() + "_file.csv";
        } while (std::filesystem::exists(name));
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
[[maybe_unused]] std::vector<std::vector<T>> vector_combinations(
    const std::vector<T>& v, size_t n) {
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
            ret.push_back(std::move(j));
        }
    }
    return ret;
}

[[maybe_unused]] std::string make_buffer(const std::string& file_name) {
    std::ifstream in{file_name, std::ios::binary};
    std::string tmp;
    std::string out;

    auto copy_if_whitespaces = [&] {
        std::string matches = "\n\r\t ";
        while (std::any_of(matches.begin(), matches.end(),
                           [&](auto c) { return in.peek() == c; })) {
            if (in.peek() == '\r') {
                out += "\r\n";
                in.ignore(2);
            } else {
                out += std::string{static_cast<char>(in.peek())};
                in.ignore(1);
            }
        }
    };

    out.reserve(sizeof(out) + 1);

    copy_if_whitespaces();
    while (in >> tmp) {
        out += tmp;
        copy_if_whitespaces();
    }
    return out;
}

template <bool buffer_mode, typename... Ts>
std::tuple<ss::parser<Ts...>, std::string> make_parser_impl(
    const std::string& file_name, std::string delim = ss::default_delimiter) {
    if (buffer_mode) {
        auto buffer = make_buffer(file_name);
        return {ss::parser<Ts...>{buffer.data(), buffer.size(), delim},
                std::move(buffer)};
    } else {
        return {ss::parser<Ts...>{file_name, delim}, std::string{}};
    }
}

template <bool buffer_mode, typename ErrorMode, typename... Ts>
[[maybe_unused]] std::enable_if_t<
    !std::is_same_v<ErrorMode, bool_error>,
    std::tuple<ss::parser<ErrorMode, Ts...>, std::string>>
make_parser(const std::string& file_name,
            std::string delim = ss::default_delimiter) {
    return make_parser_impl<buffer_mode, ErrorMode, Ts...>(file_name, delim);
}

template <bool buffer_mode, typename ErrorMode, typename... Ts>
[[maybe_unused]] std::enable_if_t<std::is_same_v<ErrorMode, bool_error>,
                                  std::tuple<ss::parser<Ts...>, std::string>>
make_parser(const std::string& file_name,
            std::string delim = ss::default_delimiter) {
    return make_parser_impl<buffer_mode, Ts...>(file_name, delim);
}

} /* namespace */
