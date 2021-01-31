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

using string_range = std::pair<const char*, const char*>;
using split_input = std::vector<string_range>;

// the error can be set inside a string, or a bool
enum class error_mode { error_string, error_bool };

template <typename... Ts>
class splitter {
private:
    enum class state { begin, reading, quoting, finished };
    constexpr static auto default_delimiter = ",";

    using quote = typename setup<Ts...>::quote;
    using trim = typename setup<Ts...>::trim;
    using escape = typename setup<Ts...>::escape;

    constexpr static auto is_const_line = !quote::enabled && !escape::enabled;

public:
    using line_ptr_type =
        typename ternary<is_const_line, const char*, char*>::type;

    bool valid() const {
        return (error_mode_ == error_mode::error_string) ? string_error_.empty()
                                                         : bool_error_ == false;
    }

    bool unterminated_quote() const {
        return unterminated_quote_;
    }

    const std::string& error_msg() const {
        return string_error_;
    }

    void set_error_mode(error_mode mode) {
        error_mode_ = mode;
    }

    const split_input& split(line_ptr_type new_line,
                       const std::string& delimiter = default_delimiter) {
        output_.clear();
        return resplit(new_line, -1, delimiter);
    }

    void adjust_ranges(const char* old_line) {
        for (auto& [begin, end] : output_) {
            begin = begin - old_line + line_;
            end = end - old_line + line_;
        }
    }

    const split_input& resplit(line_ptr_type new_line, ssize_t new_size,
                         const std::string& delimiter = default_delimiter) {
        line_ = new_line;

        // resplitting, continue from last slice
        if (!output_.empty() && unterminated_quote()) {
            const auto& last = std::prev(output_.end());
            const auto [old_line, old_begin] = *last;
            size_t begin = old_begin - old_line - 1;
            output_.pop_back();
            adjust_ranges(old_line);

            // safety measure
            if (new_size != -1 && static_cast<size_t>(new_size) < begin) {
                set_error_invalid_resplit();
                return output_;
            }

            begin_ = line_ + begin;
        }

        return split_impl_select_delim(delimiter);
    }

private:
    ////////////////
    // error
    ////////////////

    void clear_error() {
        string_error_.clear();
        bool_error_ = false;
        unterminated_quote_ = false;
    }

    void set_error_empty_delimiter() {
        if (error_mode_ == error_mode::error_string) {
            string_error_.clear();
            string_error_.append("empty delimiter");
        } else {
            bool_error_ = true;
        }
    }

    void set_error_unterminated_quote() {
        unterminated_quote_ = true;
        if (error_mode_ == error_mode::error_string) {
            string_error_.clear();
            string_error_.append("unterminated quote");
        } else {
            bool_error_ = true;
        }
    }

    void set_error_invalid_resplit() {
        if (error_mode_ == error_mode::error_string) {
            string_error_.clear();
            string_error_.append("invalid_resplit");
        } else {
            bool_error_ = true;
        }
    }

    ////////////////
    // matching
    ////////////////

    bool match(const char* const curr, char delim) {
        return *curr == delim;
    };

    bool match(const char* const curr, const std::string& delim) {
        return strncmp(curr, delim.c_str(), delim.size()) == 0;
    };

    size_t delimiter_size(char) {
        return 1;
    }

    size_t delimiter_size(const std::string& delim) {
        return delim.size();
    }

    void trim_if_enabled(line_ptr_type& curr) {
        if constexpr (trim::enabled) {
            while (trim::match(*curr)) {
                ++curr;
            }
        }
    }

    void shift_if_escaped(line_ptr_type& curr) {
        if constexpr (escape::enabled) {
            if (escape::match(*curr)) {
                *curr_ = end_[1];
                ++end_;
            }
        }
    }

    template <typename Delim>
    std::tuple<size_t, bool> match_delimiter(line_ptr_type begin,
                                             const Delim& delim) {
        line_ptr_type end = begin;

        trim_if_enabled(end);

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
        trim_if_enabled(end);

        // delimiter
        return {end - begin, true};
    }

    ////////////////
    // matching
    ////////////////

    void shift() {
        if constexpr (!is_const_line) {
            *curr_ = *end_;
        }
        ++end_;
        ++curr_;
    }

    void shift(size_t n) {
        if constexpr (!is_const_line) {
            memcpy(curr_, end_, n);
        }
        end_ += n;
        curr_ += n;
    }

    void push_and_start_next(size_t n) {
        output_.emplace_back(begin_, curr_);
        begin_ = end_ + n;
        state_ = state::begin;
    }

    split_input& split_impl_select_delim(
        const std::string& delimiter = default_delimiter) {
        clear_error();
        switch (delimiter.size()) {
        case 0:
            set_error_empty_delimiter();
            return output_;
        case 1:
            return split_impl(delimiter[0]);
        default:
            return split_impl(delimiter);
        }
    }

    template <typename Delim>
    split_input& split_impl(const Delim& delim) {
        state_ = state::begin;

        if (output_.empty()) {
            begin_ = line_;
        }

        trim_if_enabled(begin_);

        while (state_ != state::finished) {
            curr_ = end_ = begin_;
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

    ////////////////
    // states
    ////////////////

    void state_begin() {
        if constexpr (quote::enabled) {
            if (quote::match(*begin_)) {
                ++begin_;
                state_ = state::quoting;
                return;
            }
        }
        state_ = state::reading;
    }

    template <typename Delim>
    void state_reading(const Delim& delim) {
        while (true) {
            auto [width, valid] = match_delimiter(end_, delim);

            // not a delimiter
            if (!valid) {
                if (width == 0) {
                    // eol
                    output_.emplace_back(begin_, curr_);
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
                if (quote::match(*end_)) {
                    // double quote
                    // eg: ...,"hel""lo,... -> hel"lo
                    if (quote::match(end_[1])) {
                        ++end_;
                        shift();
                        continue;
                    }

                    auto [width, valid] = match_delimiter(end_ + 1, delim);

                    // not a delimiter
                    if (!valid) {
                        if (width == 0) {
                            // eol
                            // eg: ...,"hello"   \0 -> hello
                            // eg no trim: ...,"hello"\0 -> hello
                            output_.emplace_back(begin_, curr_);
                        } else {
                            // missmatched quote
                            // eg: ...,"hel"lo,... -> error
                            // or not
                        }
                        state_ = state::finished;
                        break;
                    }

                    // delimiter
                    push_and_start_next(width + 1);
                    break;
                }

                if constexpr (escape::enabled) {
                    if (escape::match(*end_)) {
                        ++end_;
                        shift();
                        continue;
                    }
                }

                // unterminated error
                // eg: ..."hell\0 -> quote not terminated
                if (*end_ == '\0') {
                    set_error_unterminated_quote();
                    output_.emplace_back(line_, begin_);
                    state_ = state::finished;
                    break;
                }
                shift();
            }
        }
    }

    ////////////////
    // members
    ////////////////

    std::vector<string_range> output_;
    std::string string_error_;
    bool bool_error_{false};
    bool unterminated_quote_{false};
    enum error_mode error_mode_ { error_mode::error_bool };
    line_ptr_type begin_;
    line_ptr_type curr_;
    line_ptr_type end_;
    line_ptr_type line_;
    state state_;
};

} /* ss */
