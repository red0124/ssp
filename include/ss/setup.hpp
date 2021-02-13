#pragma once
#include "type_traits.hpp"
#include <array>

namespace ss {

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
constexpr bool matches_intersect() {
    for (const auto& first_match : FirstMatcher::matches) {
        for (const auto& second_match : SecondMatcher::matches) {
            if (first_match != '\0' && first_match == second_match) {
                return true;
            }
        }
    }
    return false;
}

template <>
class matcher<'\0'> {
public:
    constexpr static bool enabled = false;
    constexpr static std::array<char, 1> matches{'\0'};
    static bool match(char c) = delete;
};

template <char C>
struct quote : matcher<C> {};

template <char... Cs>
struct trim : matcher<Cs...> {};

template <char... Cs>
struct escape : matcher<Cs...> {};

template <typename T, template <char...> class Template>
struct is_instance_of_matcher {
    constexpr static bool value = false;
};

template <char... Ts, template <char...> class Template>
struct is_instance_of_matcher<Template<Ts...>, Template> {
    constexpr static bool value = true;
};

template <template <char...> class Matcher, typename... Ts>
struct get_matcher;

template <template <char...> class Matcher, typename T, typename... Ts>
struct get_matcher<Matcher, T, Ts...> {
    using type = ternary_t<is_instance_of_matcher<T, Matcher>::value, T,
                           typename get_matcher<Matcher, Ts...>::type>;
};

template <template <char...> class Matcher>
struct get_matcher<Matcher> {
    using type = Matcher<'\0'>;
};

template <template <char...> class Matcher, typename... Ts>
using get_matcher_t = typename get_matcher<Matcher, Ts...>::type;

class multiline;
class string_error;

template <typename... Ts>
struct setup {
private:
    template <typename T>
    struct is_multiline : std::is_same<T, multiline> {};

    constexpr static auto count_multiline = count<is_multiline, Ts...>::size;

    template <typename T>
    struct is_string_error : std::is_same<T, string_error> {};

    constexpr static auto count_string_error =
        count<is_string_error, Ts...>::size;

public:
    using quote = get_matcher_t<quote, Ts...>;
    using trim = get_matcher_t<trim, Ts...>;
    using escape = get_matcher_t<escape, Ts...>;
    constexpr static bool multiline = (count_multiline == 1);
    constexpr static bool string_error = (count_string_error == 1);

    static_assert(
        !multiline || (multiline && (quote::enabled || escape::enabled)),
        "to enable multiline either quote or escape need to be enabled");

#define ASSERT_MSG "cannot have the same match character in multiple matchers"
    static_assert(!matches_intersect<quote, trim>(), ASSERT_MSG);
    static_assert(!matches_intersect<trim, escape>(), ASSERT_MSG);
    static_assert(!matches_intersect<escape, quote>(), ASSERT_MSG);
#undef ASSERT_MSG
};

template <typename... Ts>
struct setup<setup<Ts...>> : setup<Ts...> {};

} /* ss */
