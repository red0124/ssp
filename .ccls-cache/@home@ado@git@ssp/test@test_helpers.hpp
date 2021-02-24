#pragma once
#include <cstdlib>
#include <cstring>

#ifdef CMAKE_GITHUB_CI
#include <doctest/doctest.h>
#else
#include <doctest.h>
#endif

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
