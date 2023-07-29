#pragma once
#include <vector>
#include <string>

#ifdef CMAKE_GITHUB_CI
#include <doctest/doctest.h>
#else
#include <doctest.h>
#endif

struct buffer {
  std::string data_;

  char *operator()(const std::string &data) {
    data_ = data;
    return data_.data();
  }

  char *append(const std::string &data) {
    data_ += data;
    return data_.data();
  }

  char *append_overwrite_last(const std::string &data, size_t size) {
    data_.resize(data_.size() - size);
    return append(data);
  }
};

[[maybe_unused]] inline buffer buff;

#define CHECK_FLOATING_CONVERSION(input, type)                                 \
  {                                                                            \
    auto eps = std::numeric_limits<type>::min();                               \
    std::string s = #input;                                                    \
    auto t = ss::to_num<type>(s.c_str(), s.c_str() + s.size());                \
    REQUIRE(t.has_value());                                                    \
    CHECK_LT(std::abs(t.value() - type(input)), eps);                          \
  }                                                                            \
  {                                                                            \
    /* check negative too */                                                   \
    auto eps = std::numeric_limits<type>::min();                               \
    auto s = std::string("-") + #input;                                        \
    auto t = ss::to_num<type>(s.c_str(), s.c_str() + s.size());                \
    REQUIRE(t.has_value());                                                    \
    CHECK_LT(std::abs(t.value() - type(-input)), eps);                         \
  }

#define CHECK_INVALID_CONVERSION(input, type)                                  \
  {                                                                            \
    std::string s = input;                                                     \
    auto t = ss::to_num<type>(s.c_str(), s.c_str() + s.size());                \
    CHECK_FALSE(t.has_value());                                                \
  }

#define REQUIRE_VARIANT(var, el, type)                                         \
  {                                                                            \
    auto ptr = std::get_if<type>(&var);                                        \
    REQUIRE(ptr);                                                              \
    REQUIRE_EQ(el, *ptr);                                                      \
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
std::vector<std::vector<T>> vector_combinations(
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
            ret.push_back(move(j));
        }
    }
    return ret;
}

