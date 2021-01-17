#pragma once
#include "type_traits.hpp"
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

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

public:
    static bool match(char c) {
        return match_impl<Cs...>(c);
    }
    constexpr static bool enabled = true;
};

template <>
class matcher<'\0'> {
public:
    constexpr static bool enabled = false;
    static bool match(char c) = delete;
};

template <char... Cs>
struct quote : matcher<Cs...> {};

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
    using type =
        typename ternary<is_instance_of_matcher<T, Matcher>::value, T,
                         typename get_matcher<Matcher, Ts...>::type>::type;
};

template <template <char...> class Matcher>
struct get_matcher<Matcher> {
    using type = Matcher<'\0'>;
};

template <template <char...> class Matcher, typename... Ts>
using get_matcher_t = typename get_matcher<Matcher, Ts...>::type;

// TODO add static asserts
template <typename... Ts>
struct setup {
    using quote = get_matcher_t<quote, Ts...>;
    using trim = get_matcher_t<trim, Ts...>;
    using escape = get_matcher_t<escape, Ts...>;
};

template <typename... Ts>
struct setup<setup<Ts...>> : setup<Ts...> {};

enum class state { begin, reading, quoting, finished };
using range = std::pair<const char*, const char*>;

using string_range = std::pair<const char*, const char*>;
using split_input = std::vector<string_range>;

template <typename... Ts>
class splitter {
    using quote = typename setup<Ts...>::quote;
    using trim = typename setup<Ts...>::trim;
    using escape = typename setup<Ts...>::escape;

    bool match(const char* end_i, char delim) {
        return *end_i == delim;
    };

    bool match(const char* end_i, const std::string& delim) {
        return strncmp(end_i, delim.c_str(), delim.size()) == 0;
    };

    size_t delimiter_size(char) {
        return 1;
    }

    size_t delimiter_size(const std::string& delim) {
        return delim.size();
    }

    void trim_if_enabled(char*& curr) {
        if constexpr (trim::enabled) {
            while (trim::match(*curr)) {
                ++curr;
            }
        }
    }

    void shift_if_escaped(char*& curr_i) {
        if constexpr (escape::enabled) {
            if (escape::match(*curr_i)) {
                *curr = end[1];
                ++end;
            }
        }
    }

    void shift() {
        if constexpr (escape::enabled || quote::enabled) {
            *curr = *end;
        }
        ++end;
        ++curr;
    }

    void shift(size_t n) {
        if constexpr (escape::enabled || quote::enabled) {
            memcpy(curr, end, n);
        }
        end += n;
        curr += n;
    }

    template <typename Delim>
    std::tuple<size_t, bool> match_delimiter(char* begin, const Delim& delim) {
        char* end_i = begin;

        trim_if_enabled(end_i);

        // just spacing
        if (*end_i == '\0') {
            return {0, false};
        }

        // not a delimiter
        if (!match(end_i, delim)) {
            shift_if_escaped(end_i);
            return {1 + end_i - begin, false};
        }

        end_i += delimiter_size(delim);
        trim_if_enabled(end_i);

        // delimiter
        return {end_i - begin, true};
    }

    void push_and_start_next(size_t n) {
        output_.emplace_back(begin, curr);
        begin = end + n;
        state_ = state::begin;
    }

public:
    bool valid() {
        return error_.empty();
    }

    split_input& split(char* new_line, const std::string& d = ",") {
        line = new_line;
        output_.clear();
        switch (d.size()) {
        case 0:
            // set error
            return output_;
        case 1:
            return split_impl(d[0]);
        default:
            return split_impl(d);
        }
    }

    template <typename Delim>
    std::vector<range>& split_impl(const Delim& delim) {
        state_ = state::begin;
        begin = line;

        trim_if_enabled(begin);

        while (state_ != state::finished) {
            curr = end = begin;
            switch (state_) {
            case (state::begin):
                state_begin();
                break;
            case (state::reading):
                state_reading(delim);
                break;
            case (state::quoting):
                state_quoting(delim);
                break;
            default:
                break;
            };
        }

        return output_;
    }

    void state_begin() {
        if constexpr (quote::enabled) {
            if (quote::match(*begin)) {
                ++begin;
                state_ = state::quoting;
                return;
            }
        }
        state_ = state::reading;
    }

    template <typename Delim>
    void state_reading(const Delim& delim) {
        while (true) {
            auto [width, valid] = match_delimiter(end, delim);

            // not a delimiter
            if (!valid) {
                if (width == 0) {
                    // eol
                    output_.emplace_back(begin, curr);
                    state_ = state::finished;
                    break;
                } else {
                    shift(width);
                    continue;
                }
            }

            // found delimiter
            push_and_start_next(width);
            break;
        }
    }

    template <typename Delim>
    void state_quoting(const Delim& delim) {
        if constexpr (quote::enabled) {
            while (true) {
                if (quote::match(*end)) {
                    // double quote
                    // eg: ...,"hel""lo,... -> hel"lo
                    if (quote::match(end[1])) {
                        ++end;
                        shift();
                        continue;
                    }

                    auto [width, valid] = match_delimiter(end + 1, delim);

                    // not a delimiter
                    if (!valid) {
                        if (width == 0) {
                            // eol
                            // eg: ...,"hello"   \0 -> hello
                            // eg no trim: ...,"hello"\0 -> hello
                            output_.emplace_back(begin, curr);
                        } else {
                            // missmatched quote
                            // eg: ...,"hel"lo,... -> error
                        }
                        state_ = state::finished;
                        break;
                    }

                    // delimiter
                    push_and_start_next(width + 1);
                    break;
                }

                if constexpr (escape::enabled) {
                    if (escape::match(*end)) {
                        ++end;
                        shift();
                        continue;
                    }
                }

                // unterminated error
                // eg: ..."hell\0 -> quote not terminated
                if (*end == '\0') {
                    *curr = '\0';
                    state_ = state::finished;
                    break;
                }
                shift();
            }
        } else {
            // set error impossible scenario
            state_ = state::finished;
        }
    }

private:
    std::vector<range> output_;
    std::string error_ = "";
    state state_;
    char* curr;
    char* end;
    char* begin;
    char* line;
};

} /* ss */
