#pragma once
#include <cstring>

#ifdef CMAKE_GITHUB_CI
#include <doctest/doctest.h>
#else
#include <doctest.h>
#endif

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

[[maybe_unused]] inline buffer buff;
