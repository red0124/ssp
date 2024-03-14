#pragma once
#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>
#define SSP_DISABLE_FAST_FLOAT


namespace ss {

////////////////
// tup merge/cat
////////////////

template <typename T, typename Ts>
struct tup_cat;

template <typename... Ts, typename... Us>
struct tup_cat<std::tuple<Ts...>, std::tuple<Us...>> {
    using type = std::tuple<Ts..., Us...>;
};

template <typename T, typename... Ts>
struct tup_cat<T, std::tuple<Ts...>> {
    using type = std::tuple<T, Ts...>;
};

template <typename... Ts>
using tup_cat_t = typename tup_cat<Ts...>::type;

////////////////
// tup first/head
////////////////

template <size_t N, typename T, typename... Ts>
struct left_of_impl;

template <size_t N, typename T, typename... Ts>
struct left_of_impl {
private:
    constexpr static auto recursion_limit = 128;

public:
    static_assert(N < recursion_limit, "recursion limit reached");
    static_assert(N != 0, "cannot take the whole tuple");
    using type = tup_cat_t<T, typename left_of_impl<N - 1, Ts...>::type>;
};

template <typename T, typename... Ts>
struct left_of_impl<0, T, Ts...> {
    using type = std::tuple<T>;
};

template <size_t N, typename... Ts>
using left_of_t = typename left_of_impl<N, Ts...>::type;

template <typename... Ts>
using first_t = typename left_of_impl<sizeof...(Ts) - 2, Ts...>::type;

template <typename... Ts>
using head_t = typename left_of_impl<0, Ts...>::type;

////////////////
// tup tail/last
////////////////

template <size_t N, typename T, typename... Ts>
struct right_of_impl;

template <size_t N, typename T, typename... Ts>
struct right_of_impl {
    using type = typename right_of_impl<N - 1, Ts...>::type;
};

template <typename T, typename... Ts>
struct right_of_impl<0, T, Ts...> {
    using type = std::tuple<T, Ts...>;
};

template <size_t N, typename... Ts>
using right_of_t = typename right_of_impl<N, Ts...>::type;

template <typename... Ts>
using tail_t = typename right_of_impl<1, Ts...>::type;

template <typename... Ts>
using last_t = typename right_of_impl<sizeof...(Ts) - 1, Ts...>::type;

////////////////
// apply trait
////////////////

template <template <typename...> class Trait, typename T>
struct apply_trait;

template <template <typename...> class Trait, typename T, typename... Ts>
struct apply_trait<Trait, std::tuple<T, Ts...>> {
    using type =
        tup_cat_t<typename Trait<T>::type,
                  typename apply_trait<Trait, std::tuple<Ts...>>::type>;
};

template <template <typename...> class Trait, typename T>
struct apply_trait {
    using type = std::tuple<typename Trait<T>::type>;
};

template <template <typename...> class Trait, typename T>
struct apply_trait<Trait, std::tuple<T>> {
    using type = std::tuple<typename Trait<T>::type>;
};

template <template <typename...> class Trait, typename... Ts>
using apply_trait_t = typename apply_trait<Trait, Ts...>::type;

////////////////
// apply optional trait
////////////////

// type is T if true, and std::false_type othervise
template <typename T, typename U>
struct optional_trait;

template <typename U>
struct optional_trait<std::true_type, U> {
    using type = U;
};

template <typename U>
struct optional_trait<std::false_type, U> {
    using type = std::false_type;
};

template <template <typename...> class Trait, typename T>
struct apply_optional_trait;

template <template <typename...> class Trait, typename T, typename... Ts>
struct apply_optional_trait<Trait, std::tuple<T, Ts...>> {
    using type = tup_cat_t<
        typename optional_trait<typename Trait<T>::type, T>::type,
        typename apply_optional_trait<Trait, std::tuple<Ts...>>::type>;
};

template <template <typename...> class Trait, typename T>
struct apply_optional_trait {
    using type =
        std::tuple<typename optional_trait<typename Trait<T>::type, T>::type>;
};

template <template <typename...> class Trait, typename T>
struct apply_optional_trait<Trait, std::tuple<T>> {
    using type =
        std::tuple<typename optional_trait<typename Trait<T>::type, T>::type>;
};

template <template <typename...> class Trait, typename... Ts>
using apply_trait_optional_t = apply_optional_trait<Trait, Ts...>;

////////////////
// filter false_type
////////////////

template <typename T, typename... Ts>
struct remove_false {
    using type = tup_cat_t<T, typename remove_false<Ts...>::type>;
};

template <typename... Ts>
struct remove_false<std::false_type, Ts...> {
    using type = typename remove_false<Ts...>::type;
};

template <typename T, typename... Ts>
struct remove_false<std::tuple<T, Ts...>> {
    using type = tup_cat_t<T, typename remove_false<Ts...>::type>;
};

template <typename... Ts>
struct remove_false<std::tuple<std::false_type, Ts...>> {
    using type = typename remove_false<Ts...>::type;
};

template <typename T>
struct remove_false<T> {
    using type = std::tuple<T>;
};

template <typename T>
struct remove_false<std::tuple<T>> {
    using type = std::tuple<T>;
};

template <>
struct remove_false<std::false_type> {
    using type = std::tuple<>;
};

////////////////
// negate trait
////////////////

template <template <typename...> class Trait>
struct negate_impl {
    template <typename... Ts>
    using type = std::integral_constant<bool, !Trait<Ts...>::value>;
};

template <template <typename...> class Trait>
using negate_impl_t = typename negate_impl<Trait>::type;

////////////////
// filter by trait
////////////////

template <template <typename...> class Trait, typename... Ts>
struct filter_if {
    using type = typename filter_if<Trait, std::tuple<Ts...>>::type;
};

template <template <typename...> class Trait, typename... Ts>
struct filter_if<Trait, std::tuple<Ts...>> {

    using type = typename remove_false<
        typename apply_optional_trait<Trait, std::tuple<Ts...>>::type>::type;
};

template <template <typename...> class Trait, typename... Ts>
using filter_if_t = typename filter_if<Trait, Ts...>::type;

template <template <typename...> class Trait, typename... Ts>
struct filter_not {
    using type = typename filter_not<Trait, std::tuple<Ts...>>::type;
};

template <template <typename...> class Trait, typename... Ts>
struct filter_not<Trait, std::tuple<Ts...>> {

    using type = typename remove_false<typename apply_optional_trait<
        negate_impl<Trait>::template type, std::tuple<Ts...>>::type>::type;
};

template <template <typename...> class Trait, typename... Ts>
using filter_not_t = typename filter_not<Trait, Ts...>::type;

////////////////
// count
////////////////

template <template <typename...> class Trait, typename... Ts>
struct count;

template <template <typename...> class Trait, typename T, typename... Ts>
struct count<Trait, T, Ts...> {
    static constexpr size_t value =
        std::tuple_size<filter_if_t<Trait, T, Ts...>>::value;
};

template <template <typename...> class Trait, typename T>
struct count<Trait, T> {
    static constexpr size_t value = Trait<T>::value;
};

template <template <typename...> class Trait>
struct count<Trait> {
    static constexpr size_t value = 0;
};

template <template <typename...> class Trait, typename... Ts>
constexpr size_t count_v = count<Trait, Ts...>::value;

////////////////
// count not
////////////////

template <template <typename...> class Trait, typename... Ts>
struct count_not;

template <template <typename...> class Trait, typename T, typename... Ts>
struct count_not<Trait, T, Ts...> {
    static constexpr size_t value =
        std::tuple_size<filter_not_t<Trait, T, Ts...>>::value;
};

template <template <typename...> class Trait, typename T>
struct count_not<Trait, T> {
    static constexpr size_t value = !Trait<T>::value;
};

template <template <typename...> class Trait>
struct count_not<Trait> {
    static constexpr size_t value = 0;
};

template <template <typename...> class Trait, typename... Ts>
constexpr size_t count_not_v = count_not<Trait, Ts...>::value;

////////////////
// all of
////////////////

template <template <typename...> class Trait, typename... Ts>
struct all_of {
    static constexpr bool value = count_v<Trait, Ts...> == sizeof...(Ts);
};

template <template <typename...> class Trait, typename... Ts>
struct all_of<Trait, std::tuple<Ts...>> {
    static constexpr bool value = count_v<Trait, Ts...> == sizeof...(Ts);
};

template <template <typename...> class Trait, typename... Ts>
constexpr bool all_of_v = all_of<Trait, Ts...>::value;

////////////////
// any of
////////////////

template <template <typename...> class Trait, typename... Ts>
struct any_of {
    static_assert(sizeof...(Ts) > 0);
    static constexpr bool value = count_v<Trait, Ts...> > 0;
};

template <template <typename...> class Trait, typename... Ts>
struct any_of<Trait, std::tuple<Ts...>> {
    static_assert(sizeof...(Ts) > 0);
    static constexpr bool value = count_v<Trait, Ts...> > 0;
};

template <template <typename...> class Trait, typename... Ts>
constexpr bool any_of_v = any_of<Trait, Ts...>::value;

////////////////
// none  of
////////////////

template <template <typename...> class Trait, typename... Ts>
struct none_of {
    static constexpr bool value = count_v<Trait, Ts...> == 0;
};

template <template <typename...> class Trait, typename... Ts>
struct none_of<Trait, std::tuple<Ts...>> {
    static constexpr bool value = count_v<Trait, Ts...> == 0;
};

template <template <typename...> class Trait, typename... Ts>
constexpr bool none_of_v = none_of<Trait, Ts...>::value;

////////////////
// is instance of
////////////////

template <template <typename...> class Template, typename T>
struct is_instance_of {
    constexpr static bool value = false;
};

template <template <typename...> class Template, typename... Ts>
struct is_instance_of<Template, Template<Ts...>> {
    constexpr static bool value = true;
};

template <template <typename...> class Template, typename... Ts>
constexpr bool is_instance_of_v = is_instance_of<Template, Ts...>::value;

////////////////
// tuple to struct
////////////////

template <class T, std::size_t... Is, class U>
[[nodiscard]] T to_object_impl(std::index_sequence<Is...>, U&& data) {
    return {std::get<Is>(std::forward<U>(data))...};
}

template <class T, class U>
[[nodiscard]] T to_object(U&& data) {
    using NoRefU = std::decay_t<U>;
    if constexpr (is_instance_of_v<std::tuple, NoRefU>) {
        return to_object_impl<
            T>(std::make_index_sequence<std::tuple_size<NoRefU>{}>{},
               std::forward<U>(data));
    } else {
        return T{std::forward<U>(data)};
    }
}

} /* namespace ss */

namespace ss {

////////////////
// exception
////////////////

class exception : public std::exception {
    std::string msg_;

public:
    exception(std::string msg) : msg_{std::move(msg)} {
    }

    [[nodiscard]] char const* what() const noexcept override {
        return msg_.c_str();
    }
};

} /* namespace ss */


namespace ss {

////////////////
// function traits
////////////////

template <size_t N, typename T, typename... Ts>
struct decayed_arg_n {
    static_assert(N - 1 != sizeof...(Ts), "index out of range");
    using type = typename decayed_arg_n<N - 1, Ts...>::type;
};

template <typename T, typename... Ts>
struct decayed_arg_n<0, T, Ts...> {
    using type = std::decay_t<T>;
};

template <typename T>
struct function_traits;

template <typename R, typename C, typename Arg>
struct function_traits<std::function<R(C&, const Arg&) const>> {
    using arg_type = Arg;
};

template <typename R, typename... Ts>
struct function_traits<R(Ts...)> {
    using arg0 = typename decayed_arg_n<0, Ts...>::type;
};

template <typename R, typename... Ts>
struct function_traits<R(Ts...) const> : function_traits<R(Ts...)> {};

template <typename R, typename... Ts>
struct function_traits<R(Ts...)&> : function_traits<R(Ts...)> {};

template <typename R, typename... Ts>
struct function_traits<R(Ts...) const&> : function_traits<R(Ts...)> {};

template <typename R, typename... Ts>
struct function_traits<R(Ts...) &&> : function_traits<R(Ts...)> {};

template <typename R, typename... Ts>
struct function_traits<R(Ts...) const&&> : function_traits<R(Ts...)> {};

template <typename MemberFunction>
struct member_wrapper;

template <typename R, typename T>
struct member_wrapper<R T::*> {
    using arg_type = typename function_traits<R>::arg0;
};

////////////////
// has method
////////////////

#define INIT_HAS_METHOD(method)                                                \
    template <typename T>                                                      \
    class has_m_##method {                                                     \
        template <typename C>                                                  \
        static std::true_type test(decltype(&C::method));                      \
                                                                               \
        template <typename C>                                                  \
        static std::false_type test(...);                                      \
                                                                               \
    public:                                                                    \
        constexpr static bool value = decltype(test<T>(0))::value;             \
    };                                                                         \
                                                                               \
    template <typename T>                                                      \
    constexpr bool has_m_##method##_t = has_m_##method<T>::value;

} /* namespace ss */

namespace ss {

////////////////
// all except
////////////////

template <typename T, auto... Values>
struct ax {
private:
    template <auto X, auto... Xs>
    [[nodiscard]] bool ss_valid_impl(const T& x) const {
        if constexpr (sizeof...(Xs) != 0) {
            return x != X && ss_valid_impl<Xs...>(x);
        }
        return x != X;
    }

public:
    [[nodiscard]] bool ss_valid(const T& value) const {
        return ss_valid_impl<Values...>(value);
    }

    [[nodiscard]] const char* error() const {
        return "value excluded";
    }
};

////////////////
// none except
////////////////

template <typename T, auto... Values>
struct nx {
private:
    template <auto X, auto... Xs>
    [[nodiscard]] bool ss_valid_impl(const T& x) const {
        if constexpr (sizeof...(Xs) != 0) {
            return x == X || ss_valid_impl<Xs...>(x);
        }
        return x == X;
    }

public:
    [[nodiscard]] bool ss_valid(const T& value) const {
        return ss_valid_impl<Values...>(value);
    }

    [[nodiscard]] const char* error() const {
        return "value excluded";
    }
};

////////////////
// greater than or equal to
// greater than
// less than
// less than or equal to
////////////////

template <typename T, auto N>
struct gt {
    [[nodiscard]] bool ss_valid(const T& value) const {
        return value > N;
    }
};

template <typename T, auto N>
struct gte {
    [[nodiscard]] bool ss_valid(const T& value) const {
        return value >= N;
    }
};

template <typename T, auto N>
struct lt {
    [[nodiscard]] bool ss_valid(const T& value) const {
        return value < N;
    }
};

template <typename T, auto N>
struct lte {
    [[nodiscard]] bool ss_valid(const T& value) const {
        return value <= N;
    }
};

////////////////
// in range
////////////////

template <typename T, auto Min, auto Max>
struct ir {
    [[nodiscard]] bool ss_valid(const T& value) const {
        return value >= Min && value <= Max;
    }
};

////////////////
// out of range
////////////////

template <typename T, auto Min, auto Max>
struct oor {
    [[nodiscard]] bool ss_valid(const T& value) const {
        return value < Min || value > Max;
    }
};

////////////////
// non empty
////////////////

template <typename T>
struct ne {
    [[nodiscard]] bool ss_valid(const T& value) const {
        return !value.empty();
    }

    [[nodiscard]] const char* error() const {
        return "empty field";
    }
};

} /* namespace ss */

#if !__unix__
#include <array>
#include <cstdint>
#endif

namespace ss {

struct none {};

using string_range = std::pair<const char*, const char*>;
using split_data = std::vector<string_range>;

constexpr inline auto default_delimiter = ",";
constexpr inline auto get_line_initial_buffer_size = 128;

template <bool StringError>
void assert_string_error_defined() {
    static_assert(StringError,
                  "'string_error' needs to be enabled to use 'error_msg'");
}

template <bool ThrowOnError>
void assert_throw_on_error_not_defined() {
    static_assert(!ThrowOnError, "cannot handle errors manually if "
                                 "'throw_on_error' is enabled");
}

[[nodiscard]] inline void* strict_realloc(void* ptr, size_t size) {
    ptr = std::realloc(ptr, size);
    if (!ptr) {
        throw std::bad_alloc{};
    }

    return ptr;
}

#if __unix__
[[nodiscard]] inline ssize_t get_line_file(char*& lineptr, size_t& n,
                                           FILE* file) {
    return getline(&lineptr, &n, file);
}
#else

using ssize_t = intptr_t;

[[nodiscard]] inline ssize_t get_line_file(char*& lineptr, size_t& n,
                                           FILE* file) {
    std::array<char, get_line_initial_buffer_size> buff;

    if (lineptr == nullptr || n < sizeof(buff)) {
        const size_t new_n = sizeof(buff);
        lineptr = static_cast<char*>(strict_realloc(lineptr, new_n));
        n = new_n;
    }

    lineptr[0] = '\0';

    size_t line_used = 0;
    while (std::fgets(buff.data(), sizeof(buff), file) != nullptr) {
        line_used = std::strlen(lineptr);
        size_t buff_used = std::strlen(buff.data());

        if (n <= buff_used + line_used) {
            const size_t new_n = n * 2;
            lineptr = static_cast<char*>(strict_realloc(lineptr, new_n));
            n = new_n;
        }

        std::memcpy(lineptr + line_used, buff.data(), buff_used);
        line_used += buff_used;
        lineptr[line_used] = '\0';

        if (lineptr[line_used - 1] == '\n') {
            return line_used;
        }
    }

    return (line_used != 0) ? line_used : -1;
}

#endif

[[nodiscard]] inline ssize_t get_line_buffer(char*& lineptr, size_t& n,
                                             const char* const csv_data_buffer,
                                             size_t csv_data_size,
                                             size_t& curr_char) {
    if (curr_char >= csv_data_size) {
        return -1;
    }

    if (lineptr == nullptr || n < get_line_initial_buffer_size) {
        auto* new_lineptr = static_cast<char*>(
            strict_realloc(lineptr, get_line_initial_buffer_size));
        lineptr = new_lineptr;
        n = get_line_initial_buffer_size;
    }

    size_t line_used = 0;
    while (curr_char < csv_data_size) {
        if (line_used + 1 >= n) {
            const size_t new_n = n * 2;

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

[[nodiscard]] inline std::tuple<ssize_t, bool> get_line(
    char*& buffer, size_t& buffer_size, FILE* file,
    const char* const csv_data_buffer, size_t csv_data_size,
    size_t& curr_char) {
    ssize_t ssize = 0;
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

} /* namespace ss */

namespace ss {

////////////////
// matcher
////////////////

template <char... Cs>
struct matcher {
private:
    template <char X, char... Xs>
    static bool match_impl(char c) {
        if constexpr (sizeof...(Xs) != 0) {
            return (c == X) || match_impl<Xs...>(c);
        }
        return (c == X);
    }

    constexpr static bool contains_string_terminator() {
        for (const auto& match : matches) {
            if (match == '\0') {
                return false;
            }
        }
        return true;
    }

public:
    static bool match(char c) {
        return match_impl<Cs...>(c);
    }

    constexpr static bool enabled = true;
    constexpr static std::array<char, sizeof...(Cs)> matches{Cs...};
    static_assert(contains_string_terminator(),
                  "string terminator cannot be used as a match character");
};

template <typename FirstMatcher, typename SecondMatcher>
inline constexpr bool matches_intersect() {
    for (const auto& first_match : FirstMatcher::matches) {
        for (const auto& second_match : SecondMatcher::matches) {
            if (first_match != '\0' && first_match == second_match) {
                return true;
            }
        }
    }
    return false;
}

template <typename FirstMatcher, typename SecondMatcher1,
          typename SecondMatcher2>
inline constexpr bool matches_intersect_union() {
    return matches_intersect<FirstMatcher, SecondMatcher1>() ||
           matches_intersect<FirstMatcher, SecondMatcher2>();
}

template <>
class matcher<'\0'> {
public:
    constexpr static bool enabled = false;
    constexpr static std::array<char, 1> matches{'\0'};
    static bool match(char c) = delete;
};

////////////////
// setup
////////////////

////////////////
// matcher
////////////////

template <char C>
struct quote : matcher<C> {};

template <char... Cs>
struct trim : matcher<Cs...> {};

template <char... Cs>
struct trim_left : matcher<Cs...> {};

template <char... Cs>
struct trim_right : matcher<Cs...> {};

template <char... Cs>
struct escape : matcher<Cs...> {};

template <typename T, template <char...> class Template>
struct is_instance_of_matcher : std::false_type {};

template <char... Ts, template <char...> class Template>
struct is_instance_of_matcher<Template<Ts...>, Template> : std::true_type {};

template <typename T, template <char...> class Template>
using is_instance_of_matcher_t =
    typename is_instance_of_matcher<T, Template>::type;

template <template <char...> class Matcher, typename... Ts>
struct get_matcher;

template <template <char...> class Matcher, typename T, typename... Ts>
struct get_matcher<Matcher, T, Ts...> {

    template <typename U>
    struct is_matcher : is_instance_of_matcher<U, Matcher> {};

    static_assert(count_v<is_matcher, T, Ts...> <= 1,
                  "the same matcher cannot "
                  "be defined multiple times");
    using type = std::conditional_t<is_matcher<T>::value, T,
                                    typename get_matcher<Matcher, Ts...>::type>;
};

template <template <char...> class Matcher>
struct get_matcher<Matcher> {
    using type = Matcher<'\0'>;
};

template <template <char...> class Matcher, typename... Ts>
using get_matcher_t = typename get_matcher<Matcher, Ts...>::type;

////////////////
// multiline
////////////////

template <size_t S, bool B = true>
struct multiline_restricted {
    constexpr static auto size = S;
    constexpr static auto enabled = B;
};

using multiline = multiline_restricted<0>;

template <typename T>
struct is_instance_of_multiline : std::false_type {};

template <size_t S, bool B>
struct is_instance_of_multiline<multiline_restricted<S, B>> : std::true_type {};

template <typename T>
using is_instance_of_multiline_t = typename is_instance_of_multiline<T>::type;

template <typename... Ts>
struct get_multiline;

template <typename T, typename... Ts>
struct get_multiline<T, Ts...> {
    using type = std::conditional_t<is_instance_of_multiline<T>::value, T,
                                    typename get_multiline<Ts...>::type>;
};

template <>
struct get_multiline<> {
    using type = multiline_restricted<0, false>;
};

template <typename... Ts>
using get_multiline_t = typename get_multiline<Ts...>::type;

////////////////
// string_error
////////////////

class string_error {};

////////////////
// ignore_header
////////////////

class ignore_header {};

////////////////
// ignore_empty
////////////////

class ignore_empty {};

////////////////
// throw_on_error
////////////////

class throw_on_error {};

////////////////
// setup implementation
////////////////

template <typename... Options>
struct setup {
private:
    template <typename Option>
    struct is_matcher
        : std::disjunction<is_instance_of_matcher_t<Option, quote>,
                           is_instance_of_matcher_t<Option, escape>,
                           is_instance_of_matcher_t<Option, trim>,
                           is_instance_of_matcher_t<Option, trim_left>,
                           is_instance_of_matcher_t<Option, trim_right>> {};

    template <typename T>
    struct is_string_error : std::is_same<T, string_error> {};

    template <typename T>
    struct is_ignore_header : std::is_same<T, ignore_header> {};

    template <typename T>
    struct is_ignore_empty : std::is_same<T, ignore_empty> {};

    template <typename T>
    struct is_throw_on_error : std::is_same<T, throw_on_error> {};

    constexpr static auto count_matcher = count_v<is_matcher, Options...>;

    constexpr static auto count_multiline =
        count_v<is_instance_of_multiline, Options...>;

    constexpr static auto count_string_error =
        count_v<is_string_error, Options...>;

    constexpr static auto count_ignore_header =
        count_v<is_ignore_header, Options...>;

    constexpr static auto count_throw_on_error =
        count_v<is_throw_on_error, Options...>;

    constexpr static auto count_ignore_empty =
        count_v<is_ignore_empty, Options...>;

    constexpr static auto number_of_valid_setup_types =
        count_matcher + count_multiline + count_string_error +
        count_ignore_header + count_ignore_empty + count_throw_on_error;

    using trim_left_only = get_matcher_t<trim_left, Options...>;
    using trim_right_only = get_matcher_t<trim_right, Options...>;
    using trim_all = get_matcher_t<trim, Options...>;

public:
    using quote = get_matcher_t<quote, Options...>;
    using escape = get_matcher_t<escape, Options...>;

    using trim_left =
        std::conditional_t<trim_all::enabled, trim_all, trim_left_only>;
    using trim_right =
        std::conditional_t<trim_all::enabled, trim_all, trim_right_only>;

    using multiline = get_multiline_t<Options...>;
    constexpr static bool string_error = (count_string_error == 1);
    constexpr static bool ignore_header = (count_ignore_header == 1);
    constexpr static bool ignore_empty = (count_ignore_empty == 1);
    constexpr static bool throw_on_error = (count_throw_on_error == 1);

private:
#define ASSERT_MSG "cannot have the same match character in multiple matchers"
    static_assert(!matches_intersect<escape, quote>(), ASSERT_MSG);

    constexpr static auto quote_trim_intersect =
        matches_intersect_union<quote, trim_left, trim_right>();
    static_assert(!quote_trim_intersect, ASSERT_MSG);

    constexpr static auto escape_trim_intersect =
        matches_intersect_union<escape, trim_left, trim_right>();
    static_assert(!escape_trim_intersect, ASSERT_MSG);

#undef ASSERT_MSG

    static_assert(
        !multiline::enabled ||
            (multiline::enabled && (quote::enabled || escape::enabled)),
        "to enable multiline either quote or escape needs to be enabled");

    static_assert(!(trim_all::enabled && trim_left_only::enabled) &&
                      !(trim_all::enabled && trim_right_only::enabled),
                  "ambiguous trim setup");

    static_assert(count_multiline <= 1, "mutliline defined multiple times");

    static_assert(count_string_error <= 1,
                  "string_error defined multiple times");

    static_assert(count_throw_on_error <= 1,
                  "throw_on_error defined multiple times");

    static_assert(count_throw_on_error + count_string_error <= 1,
                  "cannot define both throw_on_error and string_error");

    static_assert(number_of_valid_setup_types == sizeof...(Options),
                  "one or multiple invalid setup parameters defined");
};

template <typename... Options>
struct setup<setup<Options...>> : setup<Options...> {};

template <typename... Options>
struct setup<std::tuple<Options...>> : setup<Options...> {};

} /* namespace ss */

namespace ss {

template <typename... Options>
class splitter {
private:
    using quote = typename setup<Options...>::quote;
    using trim_left = typename setup<Options...>::trim_left;
    using trim_right = typename setup<Options...>::trim_right;
    using escape = typename setup<Options...>::escape;
    using multiline = typename setup<Options...>::multiline;

    constexpr static auto string_error = setup<Options...>::string_error;
    constexpr static auto throw_on_error = setup<Options...>::throw_on_error;
    constexpr static auto is_const_line = !quote::enabled && !escape::enabled;

    using error_type = std::conditional_t<string_error, std::string, bool>;

public:
    using line_ptr_type = std::conditional_t<is_const_line, const char*, char*>;

    [[nodiscard]] bool valid() const {
        if constexpr (string_error) {
            return error_.empty();
        } else if constexpr (throw_on_error) {
            return true;
        } else {
            return !error_;
        }
    }

    [[nodiscard]] const std::string& error_msg() const {
        assert_string_error_defined<string_error>();
        return error_;
    }

    [[nodiscard]] bool unterminated_quote() const {
        return unterminated_quote_;
    }

    const split_data& split(line_ptr_type new_line,
                            const std::string& delimiter = default_delimiter) {
        split_data_.clear();
        line_ = new_line;
        begin_ = line_;
        return split_impl_select_delim(delimiter);
    }

    [[nodiscard]] const split_data& get_split_data() const {
        return split_data_;
    }

    void clear_split_data() {
        split_data_.clear();
    }

private:
    ////////////////
    // resplit
    ////////////////

    // number of characters the end of line is shifted backwards
    [[nodiscard]] size_t size_shifted() const {
        return escaped_;
    }

    void adjust_ranges(const char* old_line) {
        for (auto& [begin, end] : split_data_) {
            begin = begin - old_line + line_;
            end = end - old_line + line_;
        }
    }

    const split_data& resplit(
        line_ptr_type new_line, ssize_t new_size,
        const std::string& delimiter = default_delimiter) {

        // resplitting, continue from last slice
        if (!quote::enabled || !multiline::enabled || split_data_.empty() ||
            !unterminated_quote()) {
            handle_error_invalid_resplit();
            return split_data_;
        }

        const auto [old_line, old_begin] = *std::prev(split_data_.end());
        const size_t begin = old_begin - old_line - 1;

        // safety measure
        if (new_size != -1 && static_cast<size_t>(new_size) < begin) {
            handle_error_invalid_resplit();
            return split_data_;
        }

        // if unterminated quote, the last element is junk
        split_data_.pop_back();

        line_ = new_line;
        adjust_ranges(old_line);

        begin_ = line_ + begin;
        end_ = line_ - old_line + end_ - escaped_;
        curr_ = end_;

        resplitting_ = true;

        return split_impl_select_delim(delimiter);
    }

    ////////////////
    // error
    ////////////////

    void clear_error() {
        if constexpr (string_error) {
            error_.clear();
        } else if constexpr (!throw_on_error) {
            error_ = false;
        }
        unterminated_quote_ = false;
    }

    void handle_error_empty_delimiter() {
        constexpr static auto error_msg = "empty delimiter";

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg);
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg};
        } else {
            error_ = true;
        }
    }

    void handle_error_mismatched_quote(size_t n) {
        constexpr static auto error_msg = "mismatched quote at position: ";

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg + std::to_string(n));
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg + std::to_string(n)};
        } else {
            error_ = true;
        }
    }

    void handle_error_unterminated_escape() {
        constexpr static auto error_msg =
            "unterminated escape at the end of the line";

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg);
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg};
        } else {
            error_ = true;
        }
    }

    void handle_error_unterminated_quote() {
        constexpr static auto error_msg = "unterminated quote";

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg);
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg};
        } else {
            error_ = true;
        }
    }

    void handle_error_invalid_resplit() {
        constexpr static auto error_msg =
            "invalid resplit, new line must be longer"
            "than the end of the last slice";

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg);
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg};
        } else {
            error_ = true;
        }
    }

    ////////////////
    // matching
    ////////////////

    [[nodiscard]] bool match(const char* const curr, char delim) {
        return *curr == delim;
    };

    [[nodiscard]] bool match(const char* const curr, const std::string& delim) {
        return std::strncmp(curr, delim.c_str(), delim.size()) == 0;
    };

    [[nodiscard]] size_t delimiter_size(char) {
        return 1;
    }

    [[nodiscard]] size_t delimiter_size(const std::string& delim) {
        return delim.size();
    }

    void trim_left_if_enabled(line_ptr_type& curr) {
        if constexpr (trim_left::enabled) {
            while (trim_left::match(*curr)) {
                ++curr;
            }
        }
    }

    void trim_right_if_enabled(line_ptr_type& curr) {
        if constexpr (trim_right::enabled) {
            while (trim_right::match(*curr)) {
                ++curr;
            }
        }
    }

    template <typename Delim>
    [[nodiscard]] std::tuple<size_t, bool> match_delimiter(line_ptr_type begin,
                                                           const Delim& delim) {
        line_ptr_type end = begin;

        trim_right_if_enabled(end);

        // just spacing
        if (*end == '\0') {
            return {0, false};
        }

        // not a delimiter
        if (!match(end, delim)) {
            shift_if_escaped(end);
            return {1 + end - begin, false};
        }

        end += delimiter_size(delim);
        trim_left_if_enabled(end);

        // delimiter
        return {end - begin, true};
    }

    ////////////////
    // shifting
    ////////////////

    void shift_if_escaped(line_ptr_type& curr) {
        if constexpr (escape::enabled) {
            if (escape::match(*curr)) {
                if (curr[1] == '\0') {
                    if constexpr (!multiline::enabled) {
                        handle_error_unterminated_escape();
                    }
                    done_ = true;
                    return;
                }
                shift_and_jump_escape();
            }
        }
    }

    void shift_and_jump_escape() {
        shift_and_set_current();
        if constexpr (!is_const_line) {
            ++escaped_;
        }
        ++end_;
    }

    void shift_push_and_start_next(size_t n) {
        shift_and_push();
        begin_ = end_ + n;
    }

    void shift_and_push() {
        shift_and_set_current();
        split_data_.emplace_back(begin_, curr_);
    }

    void shift_and_set_current() {
        if constexpr (!is_const_line) {
            if (escaped_ > 0) {
                std::copy_n(curr_ + escaped_, end_ - curr_ - escaped_, curr_);
                curr_ = end_ - escaped_;
                return;
            }
        }
        curr_ = end_;
    }

    ////////////////
    // split impl
    ////////////////

    const split_data& split_impl_select_delim(
        const std::string& delimiter = default_delimiter) {
        clear_error();
        switch (delimiter.size()) {
        case 0:
            handle_error_empty_delimiter();
            return split_data_;
        case 1:
            return split_impl(delimiter[0]);
        default:
            return split_impl(delimiter);
        }
    }

    template <typename Delim>
    const split_data& split_impl(const Delim& delim) {

        trim_left_if_enabled(begin_);

        for (done_ = false; !done_;) {
            read(delim);
        }

        return split_data_;
    }

    ////////////////
    // reading
    ////////////////

    template <typename Delim>
    void read(const Delim& delim) {
        escaped_ = 0;
        if constexpr (quote::enabled) {
            if constexpr (multiline::enabled) {
                if (resplitting_) {
                    resplitting_ = false;
                    ++begin_;
                    read_quoted(delim);
                    return;
                }
            }
            if (quote::match(*begin_)) {
                curr_ = end_ = ++begin_;
                read_quoted(delim);
                return;
            }
        }
        curr_ = end_ = begin_;
        read_normal(delim);
    }

    template <typename Delim>
    void read_normal(const Delim& delim) {
        while (true) {
            auto [width, valid] = match_delimiter(end_, delim);

            if (!valid) {
                // not a delimiter
                if (width == 0) {
                    // eol
                    shift_and_push();
                    done_ = true;
                    break;
                } else {
                    end_ += width;
                    continue;
                }
            } else {
                // found delimiter
                shift_push_and_start_next(width);
                break;
            }
        }
    }

    template <typename Delim>
    void read_quoted(const Delim& delim) {
        if constexpr (quote::enabled) {
            while (true) {
                if (!quote::match(*end_)) {
                    if constexpr (escape::enabled) {
                        if (escape::match(*end_)) {
                            if (end_[1] == '\0') {
                                // eol, unterminated escape
                                // eg: ... "hel\\0
                                if constexpr (!multiline::enabled) {
                                    handle_error_unterminated_escape();
                                }
                                done_ = true;
                                break;
                            }
                            // not eol

                            shift_and_jump_escape();
                            ++end_;
                            continue;
                        }
                    }
                    // not escaped

                    // eol, unterminated quote error
                    // eg: ..."hell\0 -> quote not terminated
                    if (*end_ == '\0') {
                        shift_and_set_current();
                        unterminated_quote_ = true;
                        if constexpr (!multiline::enabled) {
                            handle_error_unterminated_quote();
                        }
                        split_data_.emplace_back(line_, begin_);
                        done_ = true;
                        break;
                    }
                    // not eol

                    ++end_;
                    continue;
                }
                // quote found
                // ...

                auto [width, valid] = match_delimiter(end_ + 1, delim);

                // delimiter
                if (valid) {
                    shift_push_and_start_next(width + 1);
                    break;
                }
                // not delimiter

                // double quote
                // eg: ...,"hel""lo",... -> hel"lo
                if (quote::match(end_[1])) {
                    shift_and_jump_escape();
                    ++end_;
                    continue;
                }
                // not double quote

                if (width == 0) {
                    // eol
                    // eg: ...,"hello"   \0 -> hello
                    // eg no trim: ...,"hello"\0 -> hello
                    shift_and_push();
                } else {
                    // mismatched quote
                    // eg: ...,"hel"lo,... -> error
                    handle_error_mismatched_quote(end_ - line_);
                    split_data_.emplace_back(line_, begin_);
                }
                done_ = true;
                break;
            }
        }
    }

    ////////////////
    // members
    ////////////////

    error_type error_{};
    bool unterminated_quote_{false};
    bool done_{true};
    bool resplitting_{false};
    size_t escaped_{0};
    split_data split_data_;

    line_ptr_type begin_;
    line_ptr_type curr_;
    line_ptr_type end_;
    line_ptr_type line_;

    template <typename...>
    friend class converter;
};

} /* namespace ss */


#ifndef SSP_DISABLE_FAST_FLOAT
#else
#endif

namespace ss {

////////////////
// number converters
////////////////

#ifndef SSP_DISABLE_FAST_FLOAT

template <typename T>
[[nodiscard]] std::enable_if_t<std::is_floating_point_v<T>, std::optional<T>>
to_num(const char* const begin, const char* const end) {
    T ret;
    auto [ptr, ec] = fast_float::from_chars(begin, end, ret);

    if (ec != std::errc() || ptr != end) {
        return std::nullopt;
    }
    return ret;
}

#else

template <typename T>
[[nodiscard]] std::enable_if_t<std::is_floating_point_v<T>, std::optional<T>>
to_num(const char* const begin, const char* const end) {
    static_assert(!std::is_same_v<T, long double>,
                  "Conversion to long double is disabled");

    constexpr static auto buff_max = 64;
    std::array<char, buff_max> short_buff;

    const size_t string_range = std::distance(begin, end);
    std::string long_buff;

    char* buff = nullptr;
    if (string_range > buff_max) {
        long_buff = std::string{begin, end};
        buff = long_buff.data();
    } else {
        buff = short_buff.data();
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

////////////////
// numeric_wrapper
////////////////

template <typename T>
struct numeric_wrapper {
    using type = T;

    numeric_wrapper() = default;
    numeric_wrapper(numeric_wrapper&&) noexcept = default;
    numeric_wrapper(const numeric_wrapper&) = default;

    numeric_wrapper& operator=(numeric_wrapper&&) noexcept = default;
    numeric_wrapper& operator=(const numeric_wrapper&) = default;

    ~numeric_wrapper() = default;

    numeric_wrapper(T other) : value{other} {
    }

    operator T() {
        return value;
    }

    operator T() const {
        return value;
    }

    T value;
};

using int8 = numeric_wrapper<int8_t>;
using uint8 = numeric_wrapper<uint8_t>;

template <typename T>
[[nodiscard]] std::enable_if_t<std::is_integral_v<T>, std::optional<T>> to_num(
    const char* const begin, const char* const end) {
    T ret;
    auto [ptr, ec] = std::from_chars(begin, end, ret);

    if (ec != std::errc() || ptr != end) {
        return std::nullopt;
    }
    return ret;
}

template <typename T>
[[nodiscard]] std::enable_if_t<is_instance_of_v<numeric_wrapper, T>,
                               std::optional<T>>
to_num(const char* const begin, const char* const end) {
    T ret;
    auto [ptr, ec] = std::from_chars(begin, end, ret.value);

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
} /* namespace errors */

template <typename T>
[[nodiscard]] std::enable_if_t<!std::is_integral_v<T> &&
                                   !std::is_floating_point_v<T> &&
                                   !is_instance_of_v<std::optional, T> &&
                                   !is_instance_of_v<std::variant, T> &&
                                   !is_instance_of_v<numeric_wrapper, T>,
                               bool>
extract(const char*, const char*, T&) {
    static_assert(error::unsupported_type<T>::value,
                  "Conversion for given type is not defined, an "
                  "\'extract\' function needs to be defined!");
}

template <typename T>
[[nodiscard]] std::enable_if_t<std::is_integral_v<T> ||
                                   std::is_floating_point_v<T> ||
                                   is_instance_of_v<numeric_wrapper, T>,
                               bool>
extract(const char* begin, const char* end, T& value) {
    auto optional_value = to_num<T>(begin, end);
    if (!optional_value) {
        return false;
    }
    value = optional_value.value();
    return true;
}

template <typename T>
[[nodiscard]] std::enable_if_t<is_instance_of_v<std::optional, T>, bool>
extract(const char* begin, const char* end, T& value) {
    typename T::value_type raw_value;
    if (extract(begin, end, raw_value)) {
        value = raw_value;
    } else {
        value = std::nullopt;
    }
    return true;
}

template <typename T, size_t I>
[[nodiscard]] bool extract_variant(const char* begin, const char* end,
                                   T& value) {
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
[[nodiscard]] std::enable_if_t<is_instance_of_v<std::variant, T>, bool> extract(
    const char* begin, const char* end, T& value) {
    return extract_variant<T, 0>(begin, end, value);
}

////////////////
// extract specialization
////////////////

template <>
[[nodiscard]] inline bool extract(const char* begin, const char* end,
                                  bool& value) {
    if (end == begin + 1) {
        if (*begin == '1') {
            value = true;
        } else if (*begin == '0') {
            value = false;
        } else {
            return false;
        }
    } else {
        constexpr static auto true_size = 4;
        constexpr static auto false_size = 5;
        const size_t size = end - begin;
        if (size == true_size && std::strncmp(begin, "true", size) == 0) {
            value = true;
        } else if (size == false_size &&
                   std::strncmp(begin, "false", size) == 0) {
            value = false;
        } else {
            return false;
        }
    }

    return true;
}

template <>
[[nodiscard]] inline bool extract(const char* begin, const char* end,
                                  char& value) {
    value = *begin;
    return (end == begin + 1);
}

template <>
[[nodiscard]] inline bool extract(const char* begin, const char* end,
                                  std::string& value) {
    value = std::string{begin, end};
    return true;
}

template <>
[[nodiscard]] inline bool extract(const char* begin, const char* end,
                                  std::string_view& value) {
    value = std::string_view{begin, static_cast<size_t>(end - begin)};
    return true;
}

} /* namespace ss */

namespace ss {
INIT_HAS_METHOD(tied)
INIT_HAS_METHOD(ss_valid)
INIT_HAS_METHOD(error)

////////////////
// replace validator
////////////////

// replace 'validator' types with elements they operate on
// eg. no_validator_tup_t<int, ss::nx<char, 'A', 'B'>> <=> std::tuple<int, char>
// where ss::nx<char, 'A', 'B'> is a validator '(n)one e(x)cept' which
// checks if the returned character is either 'A' or 'B', returns error if not
// additionally if one element is left in the pack, it will be unwrapped from
// the tuple eg. no_void_validator_tup_t<int> <=> int instead of std::tuple<int>
template <typename T, typename U = void>
struct no_validator;

template <typename T>
struct no_validator<T, typename std::enable_if_t<has_m_ss_valid_t<T>>> {
    using type = typename member_wrapper<decltype(&T::ss_valid)>::arg_type;
};

template <typename T, typename U>
struct no_validator {
    using type = T;
};

template <typename T>
using no_validator_t = typename no_validator<T>::type;

template <typename... Ts>
struct no_validator_tup : apply_trait<no_validator, std::tuple<Ts...>> {};

template <typename... Ts>
struct no_validator_tup<std::tuple<Ts...>> : no_validator_tup<Ts...> {};

template <typename T>
struct no_validator_tup<std::tuple<T>> : no_validator<T> {};

template <typename... Ts>
using no_validator_tup_t = typename no_validator_tup<Ts...>::type;

////////////////
// no void tuple
////////////////

template <typename... Ts>
struct no_void_tup : filter_not<std::is_void, no_validator_tup_t<Ts...>> {};

template <typename... Ts>
using no_void_tup_t = filter_not_t<std::is_void, Ts...>;

////////////////
// no void or validator
////////////////

// replace 'validators' and remove void from tuple
template <typename... Ts>
struct no_void_validator_tup : no_validator_tup<no_void_tup_t<Ts...>> {};

template <typename... Ts>
struct no_void_validator_tup<std::tuple<Ts...>>
    : no_validator_tup<no_void_tup_t<Ts...>> {};

template <typename... Ts>
using no_void_validator_tup_t = typename no_void_validator_tup<Ts...>::type;

////////////////
// tied class
////////////////

// check if the parameter pack is only one element which is a class and has
// the 'tied' method which is to be used for type deduction when converting
template <typename T, typename... Ts>
struct tied_class {
    constexpr static bool value =
        (sizeof...(Ts) == 0 && std::is_class_v<T> && has_m_tied<T>::value);
};

template <typename... Ts>
constexpr bool tied_class_v = tied_class<Ts...>::value;

////////////////
// converter
////////////////

template <typename... Options>
class converter {
    using line_ptr_type = typename splitter<Options...>::line_ptr_type;

    constexpr static auto string_error = setup<Options...>::string_error;
    constexpr static auto throw_on_error = setup<Options...>::throw_on_error;
    constexpr static auto default_delimiter = ",";

    using error_type = std::conditional_t<string_error, std::string, bool>;

public:
    // parses line with given delimiter, returns a 'T' object created with
    // extracted values of type 'Ts'
    template <typename T, typename... Ts>
    [[nodiscard]] T convert_object(
        line_ptr_type line, const std::string& delim = default_delimiter) {
        return to_object<T>(convert<Ts...>(line, delim));
    }

    // parses line with given delimiter, returns tuple of objects with
    // extracted values of type 'Ts'
    template <typename... Ts>
    [[nodiscard]] no_void_validator_tup_t<Ts...> convert(
        line_ptr_type line, const std::string& delim = default_delimiter) {
        split(line, delim);
        if (splitter_.valid()) {
            return convert<Ts...>(splitter_.get_split_data());
        } else {
            handle_error_bad_split();
            return {};
        }
    }

    // parses already split line, returns 'T' object with extracted values
    template <typename T, typename... Ts>
    [[nodiscard]] T convert_object(const split_data& elems) {
        return to_object<T>(convert<Ts...>(elems));
    }

    // same as above, but uses cached split line
    template <typename T, typename... Ts>
    [[nodiscard]] T convert_object() {
        return to_object<T>(convert<Ts...>());
    }

    // parses already split line, returns either a tuple of objects with
    // parsed values (returns raw element (no tuple) if Ts is empty), or if
    // one argument is given which is a class which has a tied
    // method which returns a tuple, returns that type
    template <typename T, typename... Ts>
    [[nodiscard]] no_void_validator_tup_t<T, Ts...> convert(
        const split_data& elems) {
        if constexpr (sizeof...(Ts) == 0 && is_instance_of_v<std::tuple, T>) {
            return convert_impl(elems, static_cast<T*>(nullptr));
        } else if constexpr (tied_class_v<T, Ts...>) {
            using arg_ref_tuple = std::invoke_result_t<decltype(&T::tied), T>;
            using arg_tuple = apply_trait_t<std::decay, arg_ref_tuple>;

            return to_object<T>(
                convert_impl(elems, static_cast<arg_tuple*>(nullptr)));
        } else {
            return convert_impl<T, Ts...>(elems);
        }
    }

    // same as above, but uses cached split line
    template <typename T, typename... Ts>
    [[nodiscard]] no_void_validator_tup_t<T, Ts...> convert() {
        return convert<T, Ts...>(splitter_.get_split_data());
    }

    [[nodiscard]] bool valid() const {
        if constexpr (string_error) {
            return error_.empty();
        } else if constexpr (throw_on_error) {
            return true;
        } else {
            return !error_;
        }
    }

    [[nodiscard]] const std::string& error_msg() const {
        assert_string_error_defined<string_error>();
        return error_;
    }

    [[nodiscard]] bool unterminated_quote() const {
        return splitter_.unterminated_quote();
    }

    // 'splits' string by given delimiter, returns vector of pairs which
    // contain the beginnings and the ends of each column of the string
    const split_data& split(line_ptr_type line,
                            const std::string& delim = default_delimiter) {
        splitter_.clear_split_data();
        if (line[0] == '\0') {
            return splitter_.get_split_data();
        }

        return splitter_.split(line, delim);
    }

private:
    ////////////////
    // resplit
    ////////////////

    const split_data& resplit(line_ptr_type new_line, ssize_t new_size,
                              const std::string& delim) {
        return splitter_.resplit(new_line, new_size, delim);
    }

    [[nodiscard]] size_t size_shifted() {
        return splitter_.size_shifted();
    }

    ////////////////
    // error
    ////////////////

    void clear_error() {
        if constexpr (string_error) {
            error_.clear();
        } else {
            error_ = false;
        }
    }

    [[nodiscard]] std::string error_sufix(const string_range msg,
                                          size_t pos) const {
        constexpr static auto reserve_size = 32;
        std::string error;
        error.reserve(reserve_size);
        error.append("at column ")
            .append(std::to_string(pos + 1))
            .append(": \'")
            .append(msg.first, msg.second)
            .append("\'");
        return error;
    }

    void handle_error_bad_split() {
        if constexpr (string_error) {
            error_.clear();
            error_.append(splitter_.error_msg());
        } else if constexpr (!throw_on_error) {
            error_ = true;
        }
    }

    void handle_error_unterminated_escape() {
        if constexpr (string_error) {
            error_.clear();
            splitter_.handle_error_unterminated_escape();
            error_.append(splitter_.error_msg());
        } else if constexpr (throw_on_error) {
            splitter_.handle_error_unterminated_escape();
        } else {
            error_ = true;
        }
    }

    void handle_error_unterminated_quote() {
        if constexpr (string_error) {
            error_.clear();
            splitter_.handle_error_unterminated_quote();
            error_.append(splitter_.error_msg());
        } else if constexpr (throw_on_error) {
            splitter_.handle_error_unterminated_quote();
        } else {
            error_ = true;
        }
    }

    void handle_error_multiline_limit_reached() {
        constexpr static auto error_msg = "multiline limit reached";
        splitter_.unterminated_quote_ = false;

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg);
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg};
        } else {
            error_ = true;
        }
    }

    void handle_error_invalid_conversion(const string_range msg, size_t pos) {
        constexpr static auto error_msg = "invalid conversion for parameter ";

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg).append(error_sufix(msg, pos));
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg + error_sufix(msg, pos)};
        } else {
            error_ = true;
        }
    }

    void handle_error_validation_failed(const char* const error,
                                        const string_range msg, size_t pos) {
        if constexpr (string_error) {
            error_.clear();
            error_.append(error).append(" ").append(error_sufix(msg, pos));
        } else if constexpr (throw_on_error) {
            throw ss::exception{error + (" " + error_sufix(msg, pos))};
        } else {
            error_ = true;
        }
    }

    void handle_error_number_of_columns(size_t expected_pos, size_t pos) {
        constexpr static auto error_msg1 =
            "invalid number of columns, expected: ";
        constexpr static auto error_msg2 = ", got: ";

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg1)
                .append(std::to_string(expected_pos))
                .append(error_msg2)
                .append(std::to_string(pos));
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg1 + std::to_string(expected_pos) +
                                error_msg2 + std::to_string(pos)};
        } else {
            error_ = true;
        }
    }

    void handle_error_incompatible_mapping(size_t argument_size,
                                           size_t mapping_size) {
        constexpr static auto error_msg1 =
            "number of arguments does not match mapping, expected: ";
        constexpr static auto error_msg2 = ", got: ";

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg1)
                .append(std::to_string(mapping_size))
                .append(error_msg2)
                .append(std::to_string(argument_size));
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg1 + std::to_string(mapping_size) +
                                error_msg2 + std::to_string(argument_size)};
        } else {
            error_ = true;
        }
    }

    ////////////////
    // convert implementation
    ////////////////

    template <typename... Ts>
    [[nodiscard]] no_void_validator_tup_t<Ts...> convert_impl(
        const split_data& elems) {
        clear_error();

        if (!splitter_.valid()) {
            handle_error_bad_split();
            return {};
        }

        if (!columns_mapped()) {
            if (sizeof...(Ts) != elems.size()) {
                handle_error_number_of_columns(sizeof...(Ts), elems.size());
                return {};
            }
        } else {
            if (sizeof...(Ts) != column_mappings_.size()) {
                handle_error_incompatible_mapping(sizeof...(Ts),
                                                  column_mappings_.size());
                return {};
            }

            if (elems.size() != number_of_columns_) {
                handle_error_number_of_columns(number_of_columns_,
                                               elems.size());
                return {};
            }
        }

        return extract_tuple<Ts...>(elems);
    }

    template <typename... Ts>
    [[nodiscard]] no_void_validator_tup_t<std::tuple<Ts...>> convert_impl(
        const split_data& elems, const std::tuple<Ts...>*) {
        return convert_impl<Ts...>(elems);
    }

    ////////////////
    // column mapping
    ////////////////

    [[nodiscard]] bool columns_mapped() const {
        return !column_mappings_.empty();
    }

    [[nodiscard]] size_t column_position(size_t tuple_position) const {
        if (!columns_mapped()) {
            return tuple_position;
        }
        return column_mappings_[tuple_position];
    }

    // assumes positions are valid and the vector is not empty
    void set_column_mapping(std::vector<size_t> positions,
                            size_t number_of_columns) {
        column_mappings_ = std::move(positions);
        number_of_columns_ = number_of_columns;
    }

    void clear_column_positions() {
        column_mappings_.clear();
        number_of_columns_ = 0;
    }

    ////////////////
    // conversion
    ////////////////

    template <typename T>
    void extract_one(no_validator_t<T>& dst, const string_range msg,
                     size_t pos) {
        if (!valid()) {
            return;
        }

        if constexpr (std::is_same_v<T, std::string>) {
            static_cast<void>(extract(msg.first, msg.second, dst));
            return;
        }

        if (!extract(msg.first, msg.second, dst)) {
            handle_error_invalid_conversion(msg, pos);
            return;
        }

        if constexpr (has_m_ss_valid_t<T>) {
            if (T validator; !validator.ss_valid(dst)) {
                if constexpr (has_m_error_t<T>) {
                    handle_error_validation_failed(validator.error(), msg, pos);
                } else {
                    handle_error_validation_failed("validation error", msg,
                                                   pos);
                }
                return;
            }
        }
    }

    template <size_t ArgN, size_t TupN, typename... Ts>
    void extract_multiple(no_void_validator_tup_t<Ts...>& tup,
                          const split_data& elems) {
        using elem_t = std::tuple_element_t<ArgN, std::tuple<Ts...>>;

        constexpr bool not_void = !std::is_void_v<elem_t>;
        constexpr bool one_element = count_not_v<std::is_void, Ts...> == 1;

        if constexpr (not_void) {
            if constexpr (one_element) {
                extract_one<elem_t>(tup, elems[column_position(ArgN)], ArgN);
            } else {
                auto& el = std::get<TupN>(tup);
                extract_one<elem_t>(el, elems[column_position(ArgN)], ArgN);
            }
        }

        if constexpr (sizeof...(Ts) > ArgN + 1) {
            constexpr size_t NewTupN = (not_void) ? TupN + 1 : TupN;
            extract_multiple<ArgN + 1, NewTupN, Ts...>(tup, elems);
        }
    }

    template <typename... Ts>
    [[nodiscard]] no_void_validator_tup_t<Ts...> extract_tuple(
        const split_data& elems) {
        static_assert(!all_of_v<std::is_void, Ts...>,
                      "at least one parameter must be non void");
        no_void_validator_tup_t<Ts...> ret{};
        extract_multiple<0, 0, Ts...>(ret, elems);
        return ret;
    }

    ////////////////
    // members
    ////////////////

    error_type error_{};
    splitter<Options...> splitter_;

    template <typename...>
    friend class parser;

    std::vector<size_t> column_mappings_;
    size_t number_of_columns_{0};
};

} /* namespace ss */


namespace ss {

template <typename... Options>
class parser {
    constexpr static auto string_error = setup<Options...>::string_error;
    constexpr static auto throw_on_error = setup<Options...>::throw_on_error;

    using multiline = typename setup<Options...>::multiline;
    using error_type = std::conditional_t<string_error, std::string, bool>;

    constexpr static bool escaped_multiline_enabled =
        multiline::enabled && setup<Options...>::escape::enabled;

    constexpr static bool quoted_multiline_enabled =
        multiline::enabled && setup<Options...>::quote::enabled;

    constexpr static bool ignore_header = setup<Options...>::ignore_header;

    constexpr static bool ignore_empty = setup<Options...>::ignore_empty;

    using header_splitter = ss::splitter<
        ss::filter_not_t<ss::is_instance_of_multiline, Options...>>;

public:
    parser(std::string file_name, std::string delim = ss::default_delimiter)
        : file_name_{std::move(file_name)}, reader_{file_name_, delim} {
        if (reader_.file_) {
            read_line();
            if constexpr (ignore_header) {
                ignore_next();
            } else {
                raw_header_ = reader_.get_buffer();
            }
        } else {
            handle_error_file_not_open();
            eof_ = true;
        }
    }

    parser(const char* const csv_data_buffer, size_t csv_data_size,
           const std::string& delim = ss::default_delimiter)
        : file_name_{"CSV data buffer"},
          reader_{csv_data_buffer, csv_data_size, delim} {
        if (csv_data_buffer) {
            read_line();
            if constexpr (ignore_header) {
                ignore_next();
            } else {
                raw_header_ = reader_.get_buffer();
            }
        } else {
            handle_error_null_buffer();
            eof_ = true;
        }
    }

    parser(parser&& other) noexcept = default;
    parser& operator=(parser&& other) noexcept = default;
    ~parser() = default;

    parser() = delete;
    parser(const parser& other) = delete;
    parser& operator=(const parser& other) = delete;

    [[nodiscard]] bool valid() const {
        if constexpr (string_error) {
            return error_.empty();
        } else if constexpr (throw_on_error) {
            return true;
        } else {
            return !error_;
        }
    }

    [[nodiscard]] const std::string& error_msg() const {
        assert_string_error_defined<string_error>();
        return error_;
    }

    [[nodiscard]] bool eof() const {
        return eof_;
    }

    bool ignore_next() {
        return reader_.read_next();
    }

    template <typename T, typename... Ts>
    [[nodiscard]] T get_object() {
        return to_object<T>(get_next<Ts...>());
    }

    [[nodiscard]] size_t line() const {
        return reader_.line_number_ > 0 ? reader_.line_number_ - 1
                                        : reader_.line_number_;
    }

    [[nodiscard]] size_t position() const {
        return reader_.chars_read_;
    }

    template <typename T, typename... Ts>
    [[nodiscard]] no_void_validator_tup_t<T, Ts...> get_next() {
        if (!eof_) {
            if constexpr (throw_on_error) {
                try {
                    reader_.parse();
                } catch (const ss::exception& e) {
                    read_line();
                    decorate_rethrow(e);
                }
            } else {
                reader_.parse();
            }
        }

        reader_.update();
        if (!reader_.converter_.valid()) {
            handle_error_invalid_conversion();
            read_line();
            return {};
        }

        clear_error();

        if (eof_) {
            handle_error_eof_reached();
            return {};
        }

        if constexpr (throw_on_error) {
            try {
                auto value = reader_.converter_.template convert<T, Ts...>();
                read_line();
                return value;
            } catch (const ss::exception& e) {
                read_line();
                decorate_rethrow(e);
            }
        }

        auto value = reader_.converter_.template convert<T, Ts...>();

        if (!reader_.converter_.valid()) {
            handle_error_invalid_conversion();
        }

        read_line();
        return value;
    }

    [[nodiscard]] std::string raw_header() const {
        assert_ignore_header_not_defined();
        return raw_header_;
    }

    [[nodiscard]] std::vector<std::string> header() {
        assert_ignore_header_not_defined();
        clear_error();

        header_splitter splitter;
        std::string raw_header_copy = raw_header_;

        if (!strict_split(splitter, raw_header_copy)) {
            return {};
        }

        std::vector<std::string> split_header;
        for (const auto& [begin, end] : splitter.get_split_data()) {
            split_header.emplace_back(begin, end);
        }

        return split_header;
    }

    [[nodiscard]] bool field_exists(const std::string& field) {
        assert_ignore_header_not_defined();
        clear_error();

        if (header_.empty()) {
            split_header_data();
        }

        if (!valid()) {
            return false;
        }

        return header_index(field).has_value();
    }

    template <typename... Ts>
    void use_fields(const Ts&... fields_args) {
        assert_ignore_header_not_defined();
        clear_error();

        if (header_.empty() && !eof()) {
            split_header_data();
        }

        if (!valid()) {
            return;
        }

        auto fields = std::vector<std::string>{fields_args...};

        if (fields.empty()) {
            handle_error_invalid_use_fields_argument();
            return;
        }

        std::vector<size_t> column_mappings;

        for (const auto& field : fields) {
            if (std::count(fields.begin(), fields.end(), field) != 1) {
                handle_error_field_used_multiple_times(field);
                return;
            }

            auto index = header_index(field);

            if (!index) {
                handle_error_invalid_field(field);
                return;
            }

            column_mappings.push_back(*index);
        }

        reader_.converter_.set_column_mapping(column_mappings, header_.size());
        reader_.next_line_converter_.set_column_mapping(column_mappings,
                                                        header_.size());

        if (line() == 0) {
            ignore_next();
        }
    }

    ////////////////
    // iterator
    ////////////////

    template <bool get_object, typename T, typename... Ts>
    struct iterable {
        struct iterator {
            using value = std::conditional_t<get_object, T,
                                             no_void_validator_tup_t<T, Ts...>>;

            iterator() : parser_{nullptr}, value_{} {
            }

            iterator(parser<Options...>* parser) : parser_{parser}, value_{} {
            }

            iterator(const iterator& other) = default;
            iterator(iterator&& other) noexcept = default;
            ~iterator() = default;

            iterator& operator=(const iterator& other) = delete;
            iterator& operator=(iterator&& other) noexcept = delete;

            [[nodiscard]] value& operator*() {
                return value_;
            }

            [[nodiscard]] value* operator->() {
                return &value_;
            }

            iterator& operator++() {
                if (!parser_ || parser_->eof()) {
                    parser_ = nullptr;
                } else {
                    if constexpr (get_object) {
                        value_ =
                            std::move(parser_->template get_object<T, Ts...>());
                    } else {
                        value_ =
                            std::move(parser_->template get_next<T, Ts...>());
                    }
                }
                return *this;
            }

            iterator operator++(int) {
                auto result = *this;
                ++*this;
                return result;
            }

            [[nodiscard]] friend bool operator==(const iterator& lhs,
                                                 const iterator& rhs) {
                return (lhs.parser_ == nullptr && rhs.parser_ == nullptr) ||
                       (lhs.parser_ == rhs.parser_ &&
                        &lhs.value_ == &rhs.value_);
            }

            [[nodiscard]] friend bool operator!=(const iterator& lhs,
                                                 const iterator& rhs) {
                return !(lhs == rhs);
            }

        private:
            parser<Options...>* parser_;
            value value_;
        };

        iterable(parser<Options...>* parser) : parser_{parser} {
        }

        [[nodiscard]] iterator begin() {
            return ++iterator{parser_};
        }

        [[nodiscard]] iterator end() {
            return iterator{};
        }

    private:
        parser<Options...>* parser_;
    };

    template <typename... Ts>
    [[nodiscard]] auto iterate() {
        return iterable<false, Ts...>{this};
    }

    template <typename... Ts>
    [[nodiscard]] auto iterate_object() {
        return iterable<true, Ts...>{this};
    }

    ////////////////
    // composite conversion
    ////////////////
    template <typename... Ts>
    class composite {
    public:
        composite(std::tuple<Ts...>&& values, parser& parser)
            : values_{std::move(values)}, parser_{parser} {
        }

        // tries to convert the same line with a different output type
        // only if the previous conversion was not successful,
        // returns composite containing itself and the new output
        // as optional, additionally, if a parameter is passed, and
        // that parameter can be invoked using the converted value,
        // than it will be invoked in the case of a valid conversion
        template <typename... Us, typename Fun = none>
        composite<Ts..., std::optional<no_void_validator_tup_t<Us...>>> or_else(
            Fun&& fun = none{}) {
            using Value = no_void_validator_tup_t<Us...>;
            std::optional<Value> value;
            try_convert_and_invoke<Value, Us...>(value, std::forward<Fun>(fun));
            return composite_with(std::move(value));
        }

        // same as or_else, but saves the result into a 'U' object
        // instead of a tuple
        template <typename U, typename... Us, typename Fun = none>
        composite<Ts..., std::optional<U>> or_object(Fun&& fun = none{}) {
            std::optional<U> value;
            try_convert_and_invoke<U, Us...>(value, std::forward<Fun>(fun));
            return composite_with(std::move(value));
        }

        [[nodiscard]] std::tuple<Ts...> values() {
            return values_;
        }

        template <typename Fun>
        auto on_error(Fun&& fun) {
            assert_throw_on_error_not_defined<throw_on_error>();
            if (!parser_.valid()) {
                if constexpr (std::is_invocable_v<Fun>) {
                    fun();
                } else {
                    static_assert(string_error,
                                  "to enable error messages within the "
                                  "on_error method "
                                  "callback string_error needs to be enabled");
                    std::invoke(std::forward<Fun>(fun), parser_.error_msg());
                }
            }
            return *this;
        }

    private:
        template <typename T>
        [[nodiscard]] composite<Ts..., T> composite_with(T&& new_value) {
            auto merged_values =
                std::tuple_cat(std::move(values_),
                               std::tuple<T>{parser_.valid()
                                                 ? std::forward<T>(new_value)
                                                 : std::nullopt});
            return {std::move(merged_values), parser_};
        }

        template <typename U, typename... Us, typename Fun = none>
        void try_convert_and_invoke(std::optional<U>& value, Fun&& fun) {
            if (parser_.valid()) {
                return;
            }

            auto tuple_output = try_same<Us...>();
            if (!parser_.valid()) {
                return;
            }

            if constexpr (!std::is_same_v<U, decltype(tuple_output)>) {
                value = to_object<U>(std::move(tuple_output));
            } else {
                value = std::move(tuple_output);
            }

            parser_.try_invoke(*value, std::forward<Fun>(fun));
        }

        template <typename U, typename... Us>
        [[nodiscard]] no_void_validator_tup_t<U, Us...> try_same() {
            parser_.clear_error();
            auto value =
                parser_.reader_.converter_.template convert<U, Us...>();
            if (!parser_.reader_.converter_.valid()) {
                parser_.handle_error_invalid_conversion();
            }
            return value;
        }

        ////////////////
        // members
        ////////////////

        std::tuple<Ts...> values_;
        parser& parser_;
    };

    // tries to convert a line and returns a composite which is
    // able to try additional conversions in case of failure
    template <typename... Ts, typename Fun = none>
    [[nodiscard]] composite<std::optional<no_void_validator_tup_t<Ts...>>>
    try_next(Fun&& fun = none{}) {
        assert_throw_on_error_not_defined<throw_on_error>();
        using Ret = no_void_validator_tup_t<Ts...>;
        return try_invoke_and_make_composite<
            std::optional<Ret>>(get_next<Ts...>(), std::forward<Fun>(fun));
    }

    // identical to try_next but returns composite with object instead of a
    // tuple
    template <typename T, typename... Ts, typename Fun = none>
    [[nodiscard]] composite<std::optional<T>> try_object(Fun&& fun = none{}) {
        assert_throw_on_error_not_defined<throw_on_error>();
        return try_invoke_and_make_composite<
            std::optional<T>>(get_object<T, Ts...>(), std::forward<Fun>(fun));
    }

private:
    // tries to invoke the given function (see below), if the function
    // returns a value which can be used as a conditional, and it returns
    // false, the function sets an error, and allows the invoke of the
    // next possible conversion as if the validation of the current one
    // failed
    template <typename Arg, typename Fun = none>
    void try_invoke(Arg&& arg, Fun&& fun) {
        constexpr bool is_none = std::is_same_v<std::decay_t<Fun>, none>;
        if constexpr (!is_none) {
            using Ret = decltype(try_invoke_impl(arg, std::forward<Fun>(fun)));
            constexpr bool returns_void = std::is_same_v<Ret, void>;
            if constexpr (!returns_void) {
                if (!try_invoke_impl(std::forward<Arg>(arg),
                                     std::forward<Fun>(fun))) {
                    handle_error_failed_check();
                }
            } else {
                try_invoke_impl(arg, std::forward<Fun>(fun));
            }
        }
    }

    // tries to invoke the function if not none
    // it first tries to invoke the function without arguments,
    // than with one argument if the function accepts the whole tuple
    // as an argument, and finally tries to invoke it with the tuple
    // laid out as a parameter pack
    template <typename Arg, typename Fun = none>
    auto try_invoke_impl(Arg&& arg, Fun&& fun) {
        constexpr bool is_none = std::is_same_v<std::decay_t<Fun>, none>;
        if constexpr (!is_none) {
            if constexpr (std::is_invocable_v<Fun>) {
                return fun();
            } else if constexpr (std::is_invocable_v<Fun, Arg>) {
                return std::invoke(std::forward<Fun>(fun),
                                   std::forward<Arg>(arg));
            } else {
                return std::apply(std::forward<Fun>(fun),
                                  std::forward<Arg>(arg));
            }
        }
    }

    template <typename T, typename Fun = none>
    [[nodiscard]] composite<T> try_invoke_and_make_composite(T&& value,
                                                             Fun&& fun) {
        if (valid()) {
            try_invoke(*value, std::forward<Fun>(fun));
        }
        return {valid() ? std::forward<T>(value) : std::nullopt, *this};
    }

    ////////////////
    // header
    ////////////////

    void assert_ignore_header_not_defined() const {
        static_assert(!ignore_header,
                      "cannot use this method when 'ignore_header' is defined");
    }

    [[nodiscard]] bool strict_split(header_splitter& splitter,
                                    std::string& header) {
        if constexpr (throw_on_error) {
            try {
                splitter.split(header.data(), reader_.delim_);
            } catch (const ss::exception& e) {
                decorate_rethrow_invalid_header_split(e);
            }
        } else {
            splitter.split(header.data(), reader_.delim_);
            if (!splitter.valid()) {
                handle_error_invalid_header_split(splitter);
                return false;
            }
        }

        return true;
    }

    void split_header_data() {
        header_splitter splitter;
        std::string raw_header_copy = raw_header_;

        if (!strict_split(splitter, raw_header_copy)) {
            return;
        }

        for (const auto& [begin, end] : splitter.get_split_data()) {
            std::string field{begin, end};
            if (field.empty()) {
                handle_error_duplicate_header_field(field);
                header_.clear();
                return;
            }
            if (std::find(header_.begin(), header_.end(), field) !=
                header_.end()) {
                handle_error_duplicate_header_field(field);
                header_.clear();
                return;
            }
            header_.push_back(std::move(field));
        }
    }

    [[nodiscard]] std::optional<size_t> header_index(const std::string& field) {
        auto it = std::find(header_.begin(), header_.end(), field);

        if (it == header_.end()) {
            return std::nullopt;
        }

        return std::distance(header_.begin(), it);
    }

    ////////////////
    // error
    ////////////////

    void clear_error() {
        if constexpr (string_error) {
            error_.clear();
        } else {
            error_ = false;
        }
    }

    void handle_error_failed_check() {
        constexpr static auto error_msg = ": failed check";

        if constexpr (string_error) {
            error_.clear();
            error_.append(file_name_).append(error_msg);
        } else if constexpr (throw_on_error) {
            throw ss::exception{file_name_ + error_msg};
        } else {
            error_ = true;
        }
    }

    void handle_error_null_buffer() {
        constexpr static auto error_msg = ": received null data buffer";

        if constexpr (string_error) {
            error_.clear();
            error_.append(file_name_).append(error_msg);
        } else if constexpr (throw_on_error) {
            throw ss::exception{file_name_ + error_msg};
        } else {
            error_ = true;
        }
    }

    void handle_error_file_not_open() {
        constexpr static auto error_msg = ": could not be opened";

        if constexpr (string_error) {
            error_.clear();
            error_.append(file_name_).append(error_msg);
        } else if constexpr (throw_on_error) {
            throw ss::exception{file_name_ + error_msg};
        } else {
            error_ = true;
        }
    }

    void handle_error_eof_reached() {
        constexpr static auto error_msg = ": read on end of file";

        if constexpr (string_error) {
            error_.clear();
            error_.append(file_name_).append(error_msg);
        } else if constexpr (throw_on_error) {
            throw ss::exception{file_name_ + error_msg};
        } else {
            error_ = true;
        }
    }

    void handle_error_invalid_conversion() {
        if constexpr (string_error) {
            error_.clear();
            error_.append(file_name_)
                .append(" ")
                .append(std::to_string(reader_.line_number_))
                .append(": ")
                .append(reader_.converter_.error_msg());
        } else if constexpr (!throw_on_error) {
            error_ = true;
        }
    }

    void handle_error_invalid_field(const std::string& field) {
        constexpr static auto error_msg =
            ": header does not contain given field: ";

        if constexpr (string_error) {
            error_.clear();
            error_.append(file_name_).append(error_msg).append(field);
        } else if constexpr (throw_on_error) {
            throw ss::exception{file_name_ + error_msg + field};
        } else {
            error_ = true;
        }
    }

    void handle_error_field_used_multiple_times(const std::string& field) {
        constexpr static auto error_msg = ": given field used multiple times: ";

        if constexpr (string_error) {
            error_.clear();
            error_.append(file_name_).append(error_msg).append(field);
        } else if constexpr (throw_on_error) {
            throw ss::exception{file_name_ + error_msg + field};
        } else {
            error_ = true;
        }
    }

    void handle_error_invalid_use_fields_argument() {
        constexpr static auto error_msg =
            "received invalid argument for 'use_fields'";

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg);
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg};
        } else {
            error_ = true;
        }
    }

    void handle_error_invalid_header_field() {
        constexpr static auto error_msg = ": header contains empty field";

        if constexpr (string_error) {
            error_.clear();
            error_.append(file_name_).append(error_msg);
        } else if constexpr (throw_on_error) {
            throw ss::exception{file_name_ + error_msg};
        } else {
            error_ = true;
        }
    }

    void handle_error_duplicate_header_field(const std::string& field) {
        constexpr static auto error_msg = ": header contains duplicate: ";

        if constexpr (string_error) {
            error_.clear();
            error_.append(file_name_).append(error_msg).append(field);
        } else if constexpr (throw_on_error) {
            throw ss::exception{file_name_ + error_msg + field};
        } else {
            error_ = true;
        }
    }

    void handle_error_invalid_header_split(const header_splitter& splitter) {
        constexpr static auto error_msg = ": failed header parsing: ";

        if constexpr (string_error) {
            error_.clear();
            error_.append(file_name_)
                .append(error_msg)
                .append(splitter.error_msg());
        } else {
            error_ = true;
        }
    }

    void decorate_rethrow_invalid_header_split(const ss::exception& e) const {
        static_assert(throw_on_error,
                      "throw_on_error needs to be enabled to use this method");
        throw ss::exception{std::string{file_name_}
                                .append(": failed header parsing: ")
                                .append(e.what())};
    }

    void decorate_rethrow(const ss::exception& e) const {
        static_assert(throw_on_error,
                      "throw_on_error needs to be enabled to use this method");
        throw ss::exception{std::string{file_name_}
                                .append(" ")
                                .append(std::to_string(line()))
                                .append(": ")
                                .append(e.what())};
    }

    ////////////////
    // line reading
    ////////////////

    void read_line() {
        eof_ = !reader_.read_next();
    }

    struct reader {
        reader(const std::string& file_name_, std::string delim)
            : delim_{std::move(delim)},
              file_{std::fopen(file_name_.c_str(), "rb")} {
        }

        reader(const char* const buffer, size_t csv_data_size,
               std::string delim)
            : delim_{std::move(delim)}, csv_data_buffer_{buffer},
              csv_data_size_{csv_data_size} {
        }

        reader(reader&& other) noexcept
            : buffer_{other.buffer_},
              next_line_buffer_{other.next_line_buffer_},
              helper_buffer_{other.helper_buffer_},
              converter_{std::move(other.converter_)},
              next_line_converter_{std::move(other.next_line_converter_)},
              buffer_size_{other.buffer_size_},
              next_line_buffer_size_{other.next_line_buffer_size_},
              helper_buffer_size{other.helper_buffer_size},
              delim_{std::move(other.delim_)}, file_{other.file_},
              csv_data_buffer_{other.csv_data_buffer_},
              csv_data_size_{other.csv_data_size_},
              curr_char_{other.curr_char_}, crlf_{other.crlf_},
              line_number_{other.line_number_}, chars_read_{other.chars_read_},
              next_line_size_{other.next_line_size_} {
            other.buffer_ = nullptr;
            other.next_line_buffer_ = nullptr;
            other.helper_buffer_ = nullptr;
            other.file_ = nullptr;
        }

        reader& operator=(reader&& other) noexcept {
            if (this != &other) {
                buffer_ = other.buffer_;
                next_line_buffer_ = other.next_line_buffer_;
                helper_buffer_ = other.helper_buffer_;
                converter_ = std::move(other.converter_);
                next_line_converter_ = std::move(other.next_line_converter_);
                buffer_size_ = other.buffer_size_;
                next_line_buffer_size_ = other.next_line_buffer_size_;
                helper_buffer_size = other.helper_buffer_size;
                delim_ = std::move(other.delim_);
                file_ = other.file_;
                csv_data_buffer_ = other.csv_data_buffer_;
                csv_data_size_ = other.csv_data_size_;
                curr_char_ = other.curr_char_;
                crlf_ = other.crlf_;
                line_number_ = other.line_number_;
                chars_read_ = other.chars_read_;
                next_line_size_ = other.next_line_size_;

                other.buffer_ = nullptr;
                other.next_line_buffer_ = nullptr;
                other.helper_buffer_ = nullptr;
                other.file_ = nullptr;
                other.csv_data_buffer_ = nullptr;
            }

            return *this;
        }

        ~reader() {
            std::free(buffer_);
            std::free(next_line_buffer_);
            std::free(helper_buffer_);

            if (file_) {
                std::ignore = std::fclose(file_);
            }
        }

        reader() = delete;
        reader(const reader& other) = delete;
        reader& operator=(const reader& other) = delete;

        // read next line each time in order to set eof_
        [[nodiscard]] bool read_next() {
            next_line_converter_.clear_error();
            size_t size = 0;
            while (size == 0) {
                ++line_number_;
                if (next_line_buffer_size_ > 0) {
                    next_line_buffer_[0] = '\0';
                }

                chars_read_ = curr_char_;
                auto [ssize, eof] =
                    get_line(next_line_buffer_, next_line_buffer_size_, file_,
                             csv_data_buffer_, csv_data_size_, curr_char_);

                if (eof) {
                    return false;
                }

                size = remove_eol(next_line_buffer_, ssize);

                if constexpr (!ignore_empty) {
                    break;
                }
            }

            next_line_size_ = size;
            return true;
        }

        void parse() {
            size_t limit = 0;

            if constexpr (escaped_multiline_enabled) {
                while (escaped_eol(next_line_size_)) {
                    if (multiline_limit_reached(limit)) {
                        return;
                    }

                    if (!append_next_line_to_buffer(next_line_buffer_,
                                                    next_line_size_,
                                                    next_line_buffer_size_)) {
                        next_line_converter_.handle_error_unterminated_escape();
                        return;
                    }
                }
            }

            next_line_converter_.split(next_line_buffer_, delim_);

            if constexpr (quoted_multiline_enabled) {
                while (unterminated_quote()) {
                    next_line_size_ -= next_line_converter_.size_shifted();

                    if (multiline_limit_reached(limit)) {
                        return;
                    }

                    if (!append_next_line_to_buffer(next_line_buffer_,
                                                    next_line_size_,
                                                    next_line_buffer_size_)) {
                        next_line_converter_.handle_error_unterminated_quote();
                        return;
                    }

                    if constexpr (escaped_multiline_enabled) {
                        while (escaped_eol(next_line_size_)) {
                            if (multiline_limit_reached(limit)) {
                                return;
                            }

                            if (!append_next_line_to_buffer(
                                    next_line_buffer_, next_line_size_,
                                    next_line_buffer_size_)) {
                                next_line_converter_
                                    .handle_error_unterminated_escape();
                                return;
                            }
                        }
                    }

                    next_line_converter_.resplit(next_line_buffer_,
                                                 next_line_size_, delim_);
                }
            }
        }

        void update() {
            std::swap(buffer_, next_line_buffer_);
            std::swap(buffer_size_, next_line_buffer_size_);
            std::swap(converter_, next_line_converter_);
        }

        [[nodiscard]] bool multiline_limit_reached(size_t& limit) {
            if constexpr (multiline::size > 0) {
                if (limit++ >= multiline::size) {
                    next_line_converter_.handle_error_multiline_limit_reached();
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] bool escaped_eol(size_t size) {
            const char* curr = nullptr;
            for (curr = next_line_buffer_ + size - 1;
                 curr >= next_line_buffer_ &&
                 setup<Options...>::escape::match(*curr);
                 --curr) {
            }
            return (next_line_buffer_ - curr + size) % 2 == 0;
        }

        [[nodiscard]] bool unterminated_quote() {
            return next_line_converter_.unterminated_quote();
        }

        void undo_remove_eol(char* buffer, size_t& line_size,
                             size_t buffer_size) {
            if (crlf_ && buffer_size >= line_size + 2) {
                std::copy_n("\r\n", 2, buffer + line_size);
                line_size += 2;
            } else if (buffer_size > line_size) {
                std::copy_n("\n", 1, buffer + line_size);
                line_size += 1;
            }
        }

        [[nodiscard]] size_t remove_eol(char*& buffer, size_t ssize) {
            if (buffer[ssize - 1] != '\n') {
                crlf_ = false;
                return ssize;
            }

            size_t size = ssize - 1;
            if (ssize >= 2 && buffer[ssize - 2] == '\r') {
                crlf_ = true;
                size--;
            } else {
                crlf_ = false;
            }

            buffer[size] = '\0';
            return size;
        }

        void realloc_concat(char*& first, size_t& first_size,
                            size_t& buffer_size, const char* const second,
                            size_t second_size) {
            buffer_size = first_size + second_size + 3;
            auto* new_first = static_cast<char*>(
                strict_realloc(static_cast<void*>(first), buffer_size));

            first = new_first;
            std::copy_n(second, second_size + 1, first + first_size);
            first_size += second_size;
        }

        [[nodiscard]] bool append_next_line_to_buffer(char*& buffer,
                                                      size_t& line_size,
                                                      size_t buffer_size) {
            undo_remove_eol(buffer, line_size, buffer_size);

            chars_read_ = curr_char_;
            auto [next_ssize, eof] =
                get_line(helper_buffer_, helper_buffer_size, file_,
                         csv_data_buffer_, csv_data_size_, curr_char_);

            if (eof) {
                return false;
            }

            ++line_number_;
            size_t next_size = remove_eol(helper_buffer_, next_ssize);
            realloc_concat(buffer, line_size, next_line_buffer_size_,
                           helper_buffer_, next_size);
            return true;
        }

        [[nodiscard]] std::string get_buffer() {
            return std::string{next_line_buffer_, next_line_size_};
        }

        ////////////////
        // members
        ////////////////
        char* buffer_{nullptr};
        char* next_line_buffer_{nullptr};
        char* helper_buffer_{nullptr};

        converter<Options...> converter_;
        converter<Options...> next_line_converter_;

        size_t buffer_size_{0};
        size_t next_line_buffer_size_{0};
        size_t helper_buffer_size{0};

        std::string delim_;
        FILE* file_{nullptr};

        const char* csv_data_buffer_{nullptr};
        size_t csv_data_size_{0};
        size_t curr_char_{0};

        bool crlf_{false};
        size_t line_number_{0};
        size_t chars_read_{0};

        size_t next_line_size_{0};
    };

    ////////////////
    // members
    ////////////////

    std::string file_name_;
    error_type error_{};
    reader reader_;
    std::vector<std::string> header_;
    std::string raw_header_;
    bool eof_{false};
};

} /* namespace ss */
