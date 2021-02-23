#pragma once

#include "type_traits.hpp"
#include <cstring>
#include <fast_float/fast_float.h>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>

namespace ss {

////////////////
// number converters
////////////////

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, std::optional<T>> to_num(
    const char* begin, const char* const end) {
    T ret;
    auto answer = fast_float::from_chars(begin, end, ret);

    if (answer.ec != std::errc() || answer.ptr != end) {
        return std::nullopt;
    }
    return ret;
}

inline std::optional<short> from_char(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    return std::nullopt;
}

#define MING32_CLANG                                                           \
    defined(__clang__) && defined(__MINGW32__) && !defined(__MINGW64__)

// mingw32 clang does not support some of the builtin functions
#if (defined(__clang__) || defined(__GNUC__) || defined(__GUNG__)) &&          \
    (MING32_CLANG == false)
#warning "using mul functions"
////////////////
// mul overflow detection
////////////////
template <typename T>
bool mul_overflow(T& result, T operand) {
    return __builtin_mul_overflow(result, operand, &result);
}

template <>
inline bool mul_overflow(int& result, int operand) {
    return __builtin_smul_overflow(result, operand, &result);
}

template <>
inline bool mul_overflow(long& result, long operand) {
    return __builtin_smull_overflow(result, operand, &result);
}

template <>
inline bool mul_overflow(long long& result, long long operand) {
    return __builtin_smulll_overflow(result, operand, &result);
}

template <>
inline bool mul_overflow(unsigned int& result, unsigned int operand) {
    return __builtin_umul_overflow(result, operand, &result);
}

template <>
inline bool mul_overflow(unsigned long& result, unsigned long operand) {
    return __builtin_umull_overflow(result, operand, &result);
}

template <>
inline bool mul_overflow(unsigned long long& result,
                         unsigned long long operand) {
    return __builtin_umulll_overflow(result, operand, &result);
}

////////////////
// addition overflow detection
////////////////

template <typename T>
inline bool add_overflow(T& result, T operand) {
    return __builtin_add_overflow(result, operand, &result);
}

template <>
inline bool add_overflow(int& result, int operand) {
    return __builtin_sadd_overflow(result, operand, &result);
}

template <>
inline bool add_overflow(long& result, long operand) {
    return __builtin_saddl_overflow(result, operand, &result);
}

template <>
inline bool add_overflow(long long& result, long long operand) {
    return __builtin_saddll_overflow(result, operand, &result);
}

template <>
inline bool add_overflow(unsigned int& result, unsigned int operand) {
    return __builtin_uadd_overflow(result, operand, &result);
}

template <>
inline bool add_overflow(unsigned long& result, unsigned long operand) {
    return __builtin_uaddl_overflow(result, operand, &result);
}

template <>
inline bool add_overflow(unsigned long long& result,
                         unsigned long long operand) {
    return __builtin_uaddll_overflow(result, operand, &result);
}

////////////////
// substraction overflow detection
////////////////
template <typename T>
inline bool sub_overflow(T& result, T operand) {
    return __builtin_sub_overflow(result, operand, &result);
}

template <>
inline bool sub_overflow(int& result, int operand) {
    return __builtin_ssub_overflow(result, operand, &result);
}

template <>
inline bool sub_overflow(long& result, long operand) {
    return __builtin_ssubl_overflow(result, operand, &result);
}

template <>
inline bool sub_overflow(long long& result, long long operand) {
    return __builtin_ssubll_overflow(result, operand, &result);
}

template <>
inline bool sub_overflow(unsigned int& result, unsigned int operand) {
    return __builtin_usub_overflow(result, operand, &result);
}

template <>
inline bool sub_overflow(unsigned long& result, unsigned long operand) {
    return __builtin_usubl_overflow(result, operand, &result);
}

template <>
inline bool sub_overflow(unsigned long long& result,
                         unsigned long long operand) {
    return __builtin_usubll_overflow(result, operand, &result);
}

template <typename T, typename F>
bool shift_and_add_overflow(T& value, T digit, F add_last_digit_owerflow) {
    if (mul_overflow<T>(value, 10) || add_last_digit_owerflow(value, digit)) {
        return true;
    }
    return false;
}
#else

#warning "Use clang or gcc if possible."
template <typename T, typename U>
bool shift_and_add_overflow(T& value, T digit, U is_negative) {
    digit = (is_negative) ? -digit : digit;
    T old_value = value;
    value = 10 * value + digit;

    T expected_old_value = (value - digit) / 10;
    if (old_value != expected_old_value) {
        return true;
    }
    return false;
}

#endif

template <typename T>
std::enable_if_t<std::is_integral_v<T>, std::optional<T>> to_num(
    const char* begin, const char* end) {
    if (begin == end) {
        return std::nullopt;
    }
    bool is_negative = false;
    if constexpr (std::is_signed_v<T>) {
        is_negative = *begin == '-';
        if (is_negative) {
            ++begin;
        }
    }

#if (defined(__clang__) || defined(__GNUC__) || defined(__GUNG__)) &&          \
    (MING32_CLANG == false)
    auto add_last_digit_owerflow =
        (is_negative) ? sub_overflow<T> : add_overflow<T>;
#else
    auto add_last_digit_owerflow = is_negative;
#endif

    T value = 0;
    for (auto i = begin; i != end; ++i) {
        if (auto digit = from_char(*i);
            !digit || shift_and_add_overflow<T>(value, digit.value(),
                                                add_last_digit_owerflow)) {
            return std::nullopt;
        }
    }

    return value;
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
    value = std::string(begin, end);
    return true;
}

} /* ss */
