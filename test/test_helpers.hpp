#pragma once

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
