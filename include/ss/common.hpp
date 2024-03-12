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
    ptr = std::realloc(ptr, size);
    if (!ptr) {
        throw std::bad_alloc{};
    }

    return ptr;
}

#if __unix__
inline ssize_t get_line_file(char*& lineptr, size_t& n, FILE* file) {
    return getline(&lineptr, &n, file);
}
#else

using ssize_t = intptr_t;

inline ssize_t get_line_file(char*& lineptr, size_t& n, FILE* file) {
    char buff[get_line_initial_buffer_size];

    if (lineptr == nullptr || n < sizeof(buff)) {
        size_t new_n = sizeof(buff);
        lineptr = static_cast<char*>(strict_realloc(lineptr, new_n));
        n = new_n;
    }

    lineptr[0] = '\0';

    size_t line_used = 0;
    while (std::fgets(buff, sizeof(buff), file) != nullptr) {
        line_used = std::strlen(lineptr);
        size_t buff_used = std::strlen(buff);

        if (n <= buff_used + line_used) {
            size_t new_n = n * 2;
            lineptr = static_cast<char*>(strict_realloc(lineptr, new_n));
            n = new_n;
        }

        std::memcpy(lineptr + line_used, buff, buff_used);
        line_used += buff_used;
        lineptr[line_used] = '\0';

        if (lineptr[line_used - 1] == '\n') {
            return line_used;
        }
    }

    return (line_used != 0) ? line_used : -1;
}

#endif

inline ssize_t get_line_buffer(char*& lineptr, size_t& n,
                        const char* const csv_data_buffer, size_t csv_data_size,
                        size_t& curr_char) {
    if (curr_char >= csv_data_size) {
        return -1;
    }

    if (lineptr == nullptr || n < get_line_initial_buffer_size) {
        auto new_lineptr = static_cast<char*>(
            strict_realloc(lineptr, get_line_initial_buffer_size));
        lineptr = new_lineptr;
        n = get_line_initial_buffer_size;
    }

    size_t line_used = 0;
    while (curr_char < csv_data_size) {
        if (line_used + 1 >= n) {
            size_t new_n = n * 2;

            char* new_lineptr =
                static_cast<char*>(strict_realloc(lineptr, new_n));
            n = new_n;
            lineptr = new_lineptr;
        }

        auto c = csv_data_buffer[curr_char++];
        lineptr[line_used++] = c;
        if (c == '\n') {
            lineptr[line_used] = '\0';
            return line_used;
        }
    }

    lineptr[line_used] = '\0';
    return line_used;
}

inline std::tuple<ssize_t, bool> get_line(char*& buffer, size_t& buffer_size,
                                   FILE* file,
                                   const char* const csv_data_buffer,
                                   size_t csv_data_size, size_t& curr_char) {
    ssize_t ssize;
    if (file) {
        ssize = get_line_file(buffer, buffer_size, file);
        curr_char += ssize;
    } else {
        ssize = get_line_buffer(buffer, buffer_size, csv_data_buffer,
                                csv_data_size, curr_char);
    }

    if (ssize == -1) {
        if (errno == ENOMEM) {
            throw std::bad_alloc{};
        }
        return {ssize, true};
    }

    return {ssize, false};
}

} /* ss */
