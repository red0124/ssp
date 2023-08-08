#pragma once

#include "type_traits.hpp"
#include <charconv>
#include <cstring>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#ifndef SSP_DISABLE_FAST_FLOAT
#include <fast_float/fast_float.h>
#else
#include <algorithm>
#include <cstdlib>
#endif

namespace ss {

////////////////
// number converters
////////////////

#ifndef SSP_DISABLE_FAST_FLOAT

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, std::optional<T>> to_num(
    const char* const begin, const char* const end) {
    T ret;
    auto [ptr, ec] = fast_float::from_chars(begin, end, ret);

    if (ec != std::errc() || ptr != end) {
        return std::nullopt;
    }
    return ret;
}

#else

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, std::optional<T>> to_num(
    const char* const begin, const char* const end) {
    static_assert(!std::is_same_v<T, long double>,
                  "Conversion to long double is disabled");

    constexpr static auto buff_max = 64;
    char short_buff[buff_max];
    size_t string_range = std::distance(begin, end);
    std::string long_buff;

    char* buff;
    if (string_range > buff_max) {
        long_buff = std::string{begin, end};
        buff = long_buff.data();
    } else {
        buff = short_buff;
        buff[string_range] = '\0';
        std::copy_n(begin, string_range, buff);
    }

    T ret;
    char* parse_end = nullptr;

    if constexpr (std::is_same_v<T, float>) {
        ret = std::strtof(buff, &parse_end);
    } else if constexpr (std::is_same_v<T, double>) {
        ret = std::strtod(buff, &parse_end);
    }

    if (parse_end != buff + string_range) {
        return std::nullopt;
    }

    return ret;
}

#endif

template <typename T>
std::enable_if_t<std::is_integral_v<T>, std::optional<T>> to_num(
    const char* const begin, const char* const end) {
    T ret;
    auto [ptr, ec] = std::from_chars(begin, end, ret);

    if (ec != std::errc() || ptr != end) {
        return std::nullopt;
    }
    return ret;
}

////////////////
// extract
////////////////

namespace error {
template <typename T>
struct unsupported_type {
    constexpr static bool value = false;
};
} /* namespace */

template <typename T>
std::enable_if_t<!std::is_integral_v<T> && !std::is_floating_point_v<T> &&
                     !is_instance_of_v<std::optional, T> &&
                     !is_instance_of_v<std::variant, T>,
                 bool>
extract(const char*, const char*, T&) {
    static_assert(error::unsupported_type<T>::value,
                  "Conversion for given type is not defined, an "
                  "\'extract\' function needs to be defined!");
}

template <typename T>
std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, bool>
extract(const char* begin, const char* end, T& value) {
    auto optional_value = to_num<T>(begin, end);
    if (!optional_value) {
        return false;
    }
    value = optional_value.value();
    return true;
}

template <typename T>
std::enable_if_t<is_instance_of_v<std::optional, T>, bool> extract(
    const char* begin, const char* end, T& value) {
    typename T::value_type raw_value;
    if (extract(begin, end, raw_value)) {
        value = raw_value;
    } else {
        value = std::nullopt;
    }
    return true;
}

template <typename T, size_t I>
bool extract_variant(const char* begin, const char* end, T& value) {
    using IthType = std::variant_alternative_t<I, std::decay_t<T>>;
    IthType ithValue;
    if (extract<IthType>(begin, end, ithValue)) {
        value = ithValue;
        return true;
    } else if constexpr (I + 1 < std::variant_size_v<T>) {
        return extract_variant<T, I + 1>(begin, end, value);
    }
    return false;
}

template <typename T>
std::enable_if_t<is_instance_of_v<std::variant, T>, bool> extract(
    const char* begin, const char* end, T& value) {
    return extract_variant<T, 0>(begin, end, value);
}

////////////////
// extract specialization
////////////////

template <>
inline bool extract(const char* begin, const char* end, bool& value) {
    if (end == begin + 1) {
        if (*begin == '1') {
            value = true;
        } else if (*begin == '0') {
            value = false;
        } else {
            return false;
        }
    } else {
        size_t size = end - begin;
        if (size == 4 && strncmp(begin, "true", size) == 0) {
            value = true;
        } else if (size == 5 && strncmp(begin, "false", size) == 0) {
            value = false;
        } else {
            return false;
        }
    }

    return true;
}

template <>
inline bool extract(const char* begin, const char* end, char& value) {
    value = *begin;
    return (end == begin + 1);
}

template <>
inline bool extract(const char* begin, const char* end, std::string& value) {
    value = std::string{begin, end};
    return true;
}

template <>
inline bool extract(const char* begin, const char* end,
                    std::string_view& value) {
    value = std::string_view{begin, static_cast<size_t>(end - begin)};
    return true;
}

} /* ss */
