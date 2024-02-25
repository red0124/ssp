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

inline void* strict_realloc(void* ptr, size_t size) {
    ptr = realloc(ptr, size);
    if (!ptr) {
        throw std::bad_alloc{};
    }

    return ptr;
}

#if __unix__
inline ssize_t get_line_file(char** lineptr, size_t* n, FILE* stream) {
    return getline(lineptr, n, stream);
}
#else

using ssize_t = intptr_t;

ssize_t get_line_file(char** lineptr, size_t* n, FILE* fp) {
    if (lineptr == nullptr || n == nullptr || fp == nullptr) {
        errno = EINVAL;
        return -1;
    }

    char buff[get_line_initial_buffer_size];

    if (*lineptr == nullptr || *n < sizeof(buff)) {
        size_t new_n = sizeof(buff);
        lineptr = static_cast<char*>(strict_realloc(*lineptr, new_n));
        *n = new_n;
    }

    (*lineptr)[0] = '\0';

    size_t line_used = 0;
    while (fgets(buff, sizeof(buff), fp) != nullptr) {
        line_used = strlen(*lineptr);
        size_t buff_used = strlen(buff);

        if (*n <= buff_used + line_used) {
            size_t new_n = *n * 2;

            lineptr = static_cast<char*>(realloc(*lineptr, new_n));
            *n = new_n;
        }

        memcpy(*lineptr + line_used, buff, buff_used);
        line_used += buff_used;
        (*lineptr)[line_used] = '\0';

        if ((*lineptr)[line_used - 1] == '\n') {
            return line_used;
        }
    }

    return (line_used != 0) ? line_used : -1;
}

#endif

} /* ss */
