#pragma once
#include "common.hpp"
#include "setup.hpp"
#include "type_traits.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

// TODO remove
#include <iostream>

namespace ss {

template <typename... Ts>
class splitter {
private:
    using quote = typename setup<Ts...>::quote;
    using trim_left = typename setup<Ts...>::trim_left;
    using trim_right = typename setup<Ts...>::trim_right;
    using escape = typename setup<Ts...>::escape;

    constexpr static auto string_error = setup<Ts...>::string_error;
    constexpr static auto is_const_line = !quote::enabled && !escape::enabled;

    using error_type = ss::ternary_t<string_error, std::string, bool>;

public:
    using line_ptr_type = ternary_t<is_const_line, const char*, char*>;

    bool valid() const {
        if constexpr (string_error) {
            return error_.empty();
        } else {
            return !error_;
        }
    }

    const std::string& error_msg() const {
        assert_string_error_defined<string_error>();
        return error_;
    }

    bool unterminated_quote() const {
        return unterminated_quote_;
    }

    const split_data& split(line_ptr_type new_line,
                            const std::string& delimiter = default_delimiter) {
        split_data_.clear();
        return resplit(new_line, -1, delimiter);
    }

private:
    ////////////////
    // resplit
    ////////////////

    void adjust_ranges(const char* old_line) {
        for (auto& [begin, end] : split_data_) {
            begin = begin - old_line + line_;
            end = end - old_line + line_;
        }
    }

    const split_data& resplit(
        line_ptr_type new_line, ssize_t new_size,
        const std::string& delimiter = default_delimiter) {
        line_ = new_line;

        // resplitting, continue from last slice
        if constexpr (quote::enabled) {
            if (!split_data_.empty() && unterminated_quote()) {
                const auto& last = std::prev(split_data_.end());
                const auto [old_line, old_begin] = *last;
                size_t begin = old_begin - old_line - 1;
                split_data_.pop_back();
                adjust_ranges(old_line);

                // safety measure
                if (new_size != -1 && static_cast<size_t>(new_size) < begin) {
                    set_error_invalid_resplit();
                    return split_data_;
                }

                std::cout << "======================" << std::endl;
                std::cout << "resplitting" << std::endl;
                resplitting_ = true;
                begin_ = line_ + begin;
                size_t end = end_ - old_line - escaped_;
                end_ = line_ + end;
                curr_ = end_;
            }
        }

        return split_impl_select_delim(delimiter);
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
        unterminated_quote_ = false;
    }

    void set_error_empty_delimiter() {
        if constexpr (string_error) {
            error_.clear();
            error_.append("empt  delimiter");
        } else {
            error_ = true;
        }
    }

    void set_error_mismatched_quote(size_t n) {
        if constexpr (string_error) {
            error_.clear();
            error_.append("mismatched quote at position: " + std::to_string(n));
        } else {
            error_ = true;
        }
    }

    void set_error_unterminated_quote() {
        unterminated_quote_ = true;
        if constexpr (string_error) {
            error_.clear();
            error_.append("unterminated quote");
        } else {
            error_ = true;
        }
    }

    void set_error_invalid_resplit() {
        unterminated_quote_ = false;
        if constexpr (string_error) {
            error_.clear();
            error_.append("invalid resplit, new line must be longer"
                          "than the end of the last slice");
        } else {
            error_ = true;
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
    std::tuple<size_t, bool> match_delimiter(line_ptr_type begin,
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

    void shift_and_push() {
        shift_and_set_current();
        split_data_.emplace_back(begin_, curr_);
    }

    void shift_if_escaped(line_ptr_type& curr) {
        if constexpr (escape::enabled) {
            if (escape::match(*curr)) {
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

    ////////////////
    // split impl
    ////////////////

    const split_data& split_impl_select_delim(
        const std::string& delimiter = default_delimiter) {
        clear_error();
        switch (delimiter.size()) {
        case 0:
            set_error_empty_delimiter();
            return split_data_;
        case 1:
            return split_impl(delimiter[0]);
        default:
            return split_impl(delimiter);
        }
    }

    template <typename Delim>
    const split_data& split_impl(const Delim& delim) {

        if (split_data_.empty()) {
            begin_ = line_;
        }

        trim_left_if_enabled(begin_);

        for (done_ = false; !done_; read(delim))
            ;

        return split_data_;
    }

    ////////////////
    // reading
    ////////////////

    template <typename Delim>
    void read(const Delim& delim) {
        escaped_ = 0;
        if constexpr (quote::enabled) {
            if (resplitting_) {
                resplitting_ = false;
                ++begin_;
                read_quoted(delim);
                return;
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
            std::cout << "start loop: " << std::endl;
            while (true) {
                std::cout << "- " << *end_ << std::endl;
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
                        shift_and_set_current();
                        set_error_unterminated_quote();
                        split_data_.emplace_back(line_, begin_);
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

public:
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

} /* ss */
