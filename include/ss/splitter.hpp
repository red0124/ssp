#pragma once
#include "setup.hpp"
#include "type_traits.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace ss {

using string_range = std::pair<const char*, const char*>;
using split_input = std::vector<string_range>;

// the error can be set inside a string, or a bool
enum class error_mode { error_string, error_bool };

template <typename... Ts>
class splitter {
private:
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
        input_.clear();
        return resplit(new_line, -1, delimiter);
    }

    void adjust_ranges(const char* old_line) {
        for (auto& [begin, end] : input_) {
            begin = begin - old_line + line_;
            end = end - old_line + line_;
        }
    }

    const split_input& resplit(
        line_ptr_type new_line, ssize_t new_size,
        const std::string& delimiter = default_delimiter) {
        line_ = new_line;

        // resplitting, continue from last slice
        if (!input_.empty() && unterminated_quote()) {
            const auto& last = std::prev(input_.end());
            const auto [old_line, old_begin] = *last;
            size_t begin = old_begin - old_line - 1;
            input_.pop_back();
            adjust_ranges(old_line);

            // safety measure
            if (new_size != -1 && static_cast<size_t>(new_size) < begin) {
                set_error_invalid_resplit();
                return input_;
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

    void set_error_mismatched_quote(size_t n) {
        if (error_mode_ == error_mode::error_string) {
            string_error_.clear();
            string_error_.append("mismatched quote at position: " +
                                 std::to_string(n));
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
        unterminated_quote_ = false;
        if (error_mode_ == error_mode::error_string) {
            string_error_.clear();
            string_error_.append("invalid resplit, new line must be longer"
                                 "than the end of the last slice");
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
                shift_and_jump_escape();
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
    // shifting
    ////////////////

    void shift_and_set_current() {
        if (escaped_ > 0) {
            if constexpr (!is_const_line) {
                std::copy_n(curr_ + escaped_, end_ - curr_, curr_);
            }
        }
        curr_ = end_ - escaped_;
    }

    void shift_and_push() {
        shift_and_set_current();
        input_.emplace_back(begin_, curr_);
    }

    void shift_and_jump_escape() {
        shift_and_set_current();
        ++end_;
        ++escaped_;
    }

    void shift_push_and_start_next(size_t n) {
        shift_and_push();
        begin_ = end_ + n;
    }

    ////////////////
    // split impl
    ////////////////

    const split_input& split_impl_select_delim(
        const std::string& delimiter = default_delimiter) {
        clear_error();
        switch (delimiter.size()) {
        case 0:
            set_error_empty_delimiter();
            return input_;
        case 1:
            return split_impl(delimiter[0]);
        default:
            return split_impl(delimiter);
        }
    }

    template <typename Delim>
    const split_input& split_impl(const Delim& delim) {

        if (input_.empty()) {
            begin_ = line_;
        }

        trim_if_enabled(begin_);

        for (done_ = false; !done_; read(delim))
            ;

        return input_;
    }

    ////////////////
    // reading
    ////////////////

    template <typename Delim>
    void read(const Delim& delim) {
        escaped_ = 0;
        if constexpr (quote::enabled) {
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
                            shift_and_jump_escape();
                            ++end_;
                            continue;
                        }
                    }

                    // unterminated quote error
                    // eg: ..."hell\0 -> quote not terminated
                    if (*end_ == '\0') {
                        set_error_unterminated_quote();
                        input_.emplace_back(line_, begin_);
                        done_ = true;
                        break;
                    }
                    ++end_;
                    continue;
                }

                auto [width, valid] = match_delimiter(end_ + 1, delim);

                // delimiter
                if (valid) {
                    shift_push_and_start_next(width + 1);
                    break;
                }

                // double quote
                // eg: ...,"hel""lo",... -> hel"lo
                if (quote::match(end_[1])) {
                    shift_and_jump_escape();
                    ++end_;
                    continue;
                }

                // not a delimiter
                if (width == 0) {
                    // eol
                    // eg: ...,"hello"   \0 -> hello
                    // eg no trim: ...,"hello"\0 -> hello
                    shift_and_push();
                } else {
                    // mismatched quote
                    // eg: ...,"hel"lo,... -> error
                    set_error_mismatched_quote(end_ - line_);
                    input_.emplace_back(line_, begin_);
                }
                done_ = true;
                break;
            }
        }
    }

    ////////////////
    // members
    ////////////////

    std::string string_error_;
    bool bool_error_{false};
    bool unterminated_quote_{false};
    enum error_mode error_mode_ { error_mode::error_bool };
    line_ptr_type begin_;
    line_ptr_type curr_;
    line_ptr_type end_;
    line_ptr_type line_;
    bool done_;
    size_t escaped_{0};

public:
    split_input input_;
};

} /* ss */
