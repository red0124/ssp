#pragma once
#include <cstdlib>
#include <cstring>
#include <doctest/doctest.h>

struct buffer {
    char* data_{nullptr};

    char* operator()(const char* data) {
        if (data_) {
            delete[] data_;
        }
        data_ = new char[strlen(data) + 1];
        strcpy(data_, data);
        return data_;
    }

    char* append(const char* data) {
        if (data_) {
            char* new_data_ = new char[strlen(data_) + strlen(data) + 1];
            strcpy(new_data_, data_);
            strcat(new_data_, data);
            delete[] data_;
            data_ = new_data_;
            return data_;
        } else {
            return operator()(data);
        }
    }

    char* append_overwrite_last(const char* data, size_t size) {
        data_[strlen(data_) - size] = '\0';
        return append(data);
    }

    ~buffer() {
        if (data_) {
            delete[] data_;
        }
    }
};

[[maybe_unused]] inline buffer buff;

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
