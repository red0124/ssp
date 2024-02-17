#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace ss {

struct none {};

using string_range = std::pair<const char*, const char*>;
using split_data = std::vector<string_range>;

constexpr inline auto default_delimiter = ",";

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
inline ssize_t get_line_file(char** lineptr, size_t* n, FILE* stream) {
    return getline(lineptr, n, stream);
}
#else
using ssize_t = int64_t;
inline ssize_t get_line_file(char** lineptr, size_t* n, FILE* stream) {
    size_t pos;
    int c;

    if (lineptr == nullptr || stream == nullptr || n == nullptr) {
        errno = EINVAL;
        return -1;
    }

    c = getc(stream);
    if (c == EOF) {
        return -1;
    }

    if (*lineptr == nullptr) {
        *lineptr = static_cast<char*>(malloc(128));
        if (*lineptr == nullptr) {
            return -1;
        }
        *n = 128;
    }

    pos = 0;
    while (c != EOF) {
        if (pos + 1 >= *n) {
            size_t new_size = *n + (*n >> 2);
            if (new_size < 128) {
                new_size = 128;
            }
            char* new_ptr = static_cast<char*>(
                realloc(static_cast<void*>(*lineptr), new_size));
            if (new_ptr == nullptr) {
                return -1;
            }
            *n = new_size;
            *lineptr = new_ptr;
        }

        (*lineptr)[pos++] = c;
        if (c == '\n') {
            break;
        }
        c = getc(stream);
    }

    (*lineptr)[pos] = '\0';
    return pos;
}
#endif

} /* ss */
