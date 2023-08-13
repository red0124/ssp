#pragma once
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

} /* ss */
