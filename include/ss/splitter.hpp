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

////////////////
// is instance of
////////////////

template <typename T, template <char...> class Template>
struct is_instance_of_char {
    constexpr static bool value = false;
};

template <char... Ts, template <char...> class Template>
struct is_instance_of_char<Template<Ts...>, Template> {
    constexpr static bool value = true;
};

///////////////////////////////////////////////////

template <char... Cs>
struct quote : matcher<Cs...> {};

template <char... Cs>
struct trim : matcher<Cs...> {};

template <char... Cs>
struct escape : matcher<Cs...> {};

/////////////////////////////////////////////////
// -> type traits
template <bool B, typename T, typename U>
struct if_then_else;

template <typename T, typename U>
struct if_then_else<true, T, U> {
    using type = T;
};

template <typename T, typename U>
struct if_then_else<false, T, U> {
    using type = U;
};

//////////////////////////////////////////////
template <template <char...> class Matcher, typename... Ts>
struct get_matcher;

template <template <char...> class Matcher, typename T, typename... Ts>
struct get_matcher<Matcher, T, Ts...> {
    using type =
        typename if_then_else<is_instance_of_char<T, Matcher>::value, T,
                              typename get_matcher<Matcher, Ts...>::type>::type;
};

template <template <char...> class Matcher>
struct get_matcher<Matcher> {
    using type = Matcher<'\0'>;
};

///////////////////////////////////////////////
// TODO add restriction
template <typename... Ts>
struct setup {
    using quote = typename get_matcher<quote, Ts...>::type;
    using trim = typename get_matcher<trim, Ts...>::type;
    using escape = typename get_matcher<escape, Ts...>::type;
};

template <typename... Ts>
struct setup<setup<Ts...>> : setup<Ts...> {};

/////////////////////////////////////////////////////////////////////////////

enum class State { finished, begin, reading, quoting };
using range = std::pair<const char*, const char*>;

using string_range = std::pair<const char*, const char*>;
using split_input = std::vector<string_range>;

template <typename... Ts>
class splitter {
    using Setup = setup<Ts...>;
    using quote = typename Setup::quote;
    using trim = typename Setup::trim;
    using escape = typename Setup::escape;

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
        state = State::begin;
        begin = line;

        trim_if_enabled(begin);

        while (state != State::finished) {
            curr = end = begin;
            switch (state) {
            case (State::begin):
                state_begin();
                break;
            case (State::reading):
                state_reading(delim);
                break;
            case (State::quoting):
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
                state = State::quoting;
                return;
            }
        }
        state = State::reading;
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
                    state = State::finished;
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
                        state = State::finished;
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
                    state = State::finished;
                    break;
                }
                shift();
            }
        } else {
            // set error impossible scenario
            state = State::finished;
        }
    }

    void push_and_start_next(size_t n) {
        output_.emplace_back(begin, curr);
        begin = end + n;
        state = State::begin;
    }

private:
    std::vector<range> output_;
    std::string error_ = "";
    State state;
    char* curr;
    char* end;
    char* begin;
    char* line;
};

} /* ss */
