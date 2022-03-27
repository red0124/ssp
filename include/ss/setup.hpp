#pragma once
#include "type_traits.hpp"
#include <array>

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
                  "the same matcher is cannot"
                  "be defined multiple times");
    using type = ternary_t<is_matcher<T>::value, T,
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
    using type = ternary_t<is_instance_of_multiline<T>::value, T,
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

class string_error;

////////////////
// ignore_header
////////////////

class ignore_header;

////////////////
// setup implementation
////////////////

template <typename... Ts>
struct setup {
private:
    template <typename T>
    struct is_matcher
        : std::disjunction<is_instance_of_matcher_t<T, quote>,
                           is_instance_of_matcher_t<T, escape>,
                           is_instance_of_matcher_t<T, trim>,
                           is_instance_of_matcher_t<T, trim_left>,
                           is_instance_of_matcher_t<T, trim_right>> {};

    template <typename T>
    struct is_string_error : std::is_same<T, string_error> {};

    template <typename T>
    struct is_ignore_header : std::is_same<T, ignore_header> {};

    constexpr static auto count_matcher = count_v<is_matcher, Ts...>;
    constexpr static auto count_multiline =
        count_v<is_instance_of_multiline, Ts...>;
    constexpr static auto count_string_error = count_v<is_string_error, Ts...>;
    constexpr static auto count_ignore_header =
        count_v<is_ignore_header, Ts...>;

    constexpr static auto number_of_valid_setup_types =
        count_matcher + count_multiline + count_string_error +
        count_ignore_header;

    using trim_left_only = get_matcher_t<trim_left, Ts...>;
    using trim_right_only = get_matcher_t<trim_right, Ts...>;
    using trim_all = get_matcher_t<trim, Ts...>;

public:
    using quote = get_matcher_t<quote, Ts...>;
    using escape = get_matcher_t<escape, Ts...>;

    using trim_left = ternary_t<trim_all::enabled, trim_all, trim_left_only>;
    using trim_right = ternary_t<trim_all::enabled, trim_all, trim_right_only>;

    using multiline = get_multiline_t<Ts...>;
    constexpr static bool string_error = (count_string_error == 1);
    constexpr static bool ignore_header = (count_ignore_header == 1);

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
        "to enable multiline either quote or escape need to be enabled");

    static_assert(!(trim_all::enabled && trim_left_only::enabled) &&
                      !(trim_all::enabled && trim_right_only::enabled),
                  "ambiguous trim setup");

    static_assert(count_multiline <= 1, "mutliline defined multiple times");
    static_assert(count_string_error <= 1,
                  "string_error defined multiple times");

    static_assert(number_of_valid_setup_types == sizeof...(Ts),
                  "one or multiple invalid setup parameters defined");
};

template <typename... Ts>
struct setup<setup<Ts...>> : setup<Ts...> {};

} /* ss */
