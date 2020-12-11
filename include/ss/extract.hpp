#pragma once

#include "type_traits.hpp"
#include <cstring>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

namespace ss {

// todo
// taken from
// https://gist.github.com/oschonrock/67fc870ba067ebf0f369897a9d52c2dd

////////////////
// number converters
////////////////
template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, T> pow10(int n) {
        T ret = 1.0;
        T r = 10.0;
        if (n < 0) {
                n = -n;
                r = 0.1;
        }

        while (n) {
                if (n & 1) {
                        ret *= r;
                }
                r *= r;
                n >>= 1;
        }
        return ret;
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, std::optional<T>> to_num(
    const char* begin, const char* const end) {
        if (begin == end) {
                return std::nullopt;
        }
        int sign = 1;
        T int_part = 0.0;
        T frac_part = 0.0;
        bool has_frac = false;
        bool has_exp = false;

        // +/- sign
        if (*begin == '-') {
                ++begin;
                sign = -1;
        }

        while (begin != end) {
                if (*begin >= '0' && *begin <= '9') {
                        int_part = int_part * 10 + (*begin - '0');
                } else if (*begin == '.') {
                        has_frac = true;
                        ++begin;
                        break;
                } else if (*begin == 'e') {
                        has_exp = true;
                        ++begin;
                        break;
                } else {
                        return std::nullopt;
                }
                ++begin;
        }

        if (has_frac) {
                T frac_exp = 0.1;

                while (begin != end) {
                        if (*begin >= '0' && *begin <= '9') {
                                frac_part += frac_exp * (*begin - '0');
                                frac_exp *= 0.1;
                        } else if (*begin == 'e') {
                                has_exp = true;
                                ++begin;
                                break;
                        } else {
                                return std::nullopt;
                        }
                        ++begin;
                }
        }

        // parsing exponent part
        T exp_part = 1.0;
        if (begin != end && has_exp) {
                int exp_sign = 1;
                if (*begin == '-') {
                        exp_sign = -1;
                        ++begin;
                } else if (*begin == '+') {
                        ++begin;
                }

                int e = 0;
                while (begin != end && *begin >= '0' && *begin <= '9') {
                        e = e * 10 + *begin - '0';
                        ++begin;
                }

                exp_part = pow10<T>(exp_sign * e);
        }

        if (begin != end) {
                return std::nullopt;
        }

        return sign * (int_part + frac_part) * exp_part;
}

inline std::optional<short> from_char(char c) {
        if (c >= '0' && c <= '9') {
                return c - '0';
        }
        return std::nullopt;
}

#if defined(__clang__) || defined(__GNUC__) || defined(__GUNG__)
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
        if (mul_overflow<T>(value, 10) ||
            add_last_digit_owerflow(value, digit)) {
                return true;
        }
        return false;
}
#else

#warning "use clang or gcc!!!"
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

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
        auto add_last_digit_owerflow =
            (is_negative) ? sub_overflow<T> : add_overflow<T>;
#else
        auto add_last_digit_owerflow = is_negative;
#endif

        T value = 0;
        for (auto i = begin; i != end; ++i) {
                if (auto digit = from_char(*i);
                    !digit ||
                    shift_and_add_overflow<T>(value, digit.value(),
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
                     !is_instance_of<T, std::optional>::value,
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
std::enable_if_t<is_instance_of<T, std::optional>::value, bool> extract(
    const char* begin, const char* end, T& value) {
        typename T::value_type raw_value;
        if (extract(begin, end, raw_value)) {
                value = raw_value;
        } else {
                value = std::nullopt;
        }
        return true;
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
