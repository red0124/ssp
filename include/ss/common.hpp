#pragma once
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace ss {

struct none {};

using string_range = std::pair<const char*, const char*>;
using split_data = std::vector<string_range>;

constexpr inline auto default_delimiter = ",";
constexpr inline auto get_line_initial_buffer_size = 128;

template <bool StringError>
inline void assert_string_error_defined() {
    static_assert(StringError,
                  "'string_error' needs to be enabled to use 'error_msg'");
}

template <bool ThrowOnError>
inline void assert_throw_on_error_not_defined() {
    static_assert(!ThrowOnError, "cannot handle errors manually if "
                                 "'throw_on_error' is enabled");
}

#if __unix__
inline ssize_t get_line(char** lineptr, size_t* n, FILE* stream) {
    return getline(lineptr, n, stream);
}
#else

using ssize_t = int64_t;

ssize_t get_line(char** lineptr, size_t* n, FILE* fp) {
    if (lineptr == nullptr || n == nullptr || fp == nullptr) {
        errno = EINVAL;
        return -1;
    }

    char buff[get_line_initial_buffer_size];

    if (*lineptr == nullptr || *n < sizeof(buff)) {
        *n = sizeof(buff);
        auto new_lineptr = static_cast<char*>(realloc(*lineptr, *n));
        if (new_lineptr == nullptr) {
            errno = ENOMEM;
            return -1;
        }

        *lineptr = new_lineptr;
    }

    (*lineptr)[0] = '\0';

    while (fgets(buff, sizeof(buff), fp) != nullptr) {
        size_t line_used = strlen(*lineptr);
        size_t buff_used = strlen(buff);

        if (*n < buff_used + line_used) {
            *n *= 2;

            auto new_lineptr = static_cast<char*>(realloc(*lineptr, *n));
            if (new_lineptr == nullptr) {
                errno = ENOMEM;
                return -1;
            }

            *lineptr = new_lineptr;
        }

        memcpy(*lineptr + line_used, buff, buff_used);
        line_used += buff_used;
        (*lineptr)[line_used] = '\0';

        if ((*lineptr)[line_used - 1] == '\n') {
            return line_used;
        }
    }

    return -1;
}

#endif

} /* ss */
