#pragma once

// TODO remove or rename
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define U_IF(OP) if (unlikely(OP))
#define L_IF(OP) if (likely(OP))

#include "common.hpp"
#include "converter.hpp"
#include "exception.hpp"
#include "extract.hpp"
#include "restrictions.hpp"
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

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

public:
    parser(const std::string& file_name,
           const std::string& delim = ss::default_delimiter)
        : file_name_{file_name}, reader_{file_name_, delim} {
        if (reader_.file_) {
            // read_line();
            if constexpr (ignore_header) {
                ignore_next();
            } else {
                // TODO handle
                // raw_header_ = reader_.get_buffer();
            }
        } else {
            handle_error_file_not_open();
            eof_ = true;
        }
    }

    parser(parser&& other) = default;
    parser& operator=(parser&& other) = default;

    parser() = delete;
    parser(const parser& other) = delete;
    parser& operator=(const parser& other) = delete;

    bool valid() const {
        if constexpr (string_error) {
            return error_.empty();
        } else if constexpr (throw_on_error) {
            return true;
        } else {
            return !error_;
        }
    }

    const std::string& error_msg() const {
        assert_string_error_defined<string_error>();
        return error_;
    }

    bool eof() const {
        return eof_;
    }

    bool ignore_next() {
        return reader_.read_next();
    }

    template <typename T, typename... Ts>
    T get_object() {
        return to_object<T>(get_next<Ts...>());
    }

    size_t line() const {
        return reader_.line_number_;
    }

    template <typename T, typename... Ts>
    no_void_validator_tup_t<T, Ts...> get_next() {
        clear_error();
        // TODO update
        reader_.clear_error();

        if (eof_) {
            handle_error_eof_reached();
            return {};
        }

        if constexpr (throw_on_error) {
            try {
                read_line();
                return reader_.converter_.template convert<T, Ts...>(
                    reader_.split_data_);
            } catch (const ss::exception& e) {
                decorate_rethrow(e);
            }
        }

        read_line();

        // TODO update
        if (!reader_.valid()) {
            handle_error_invalid_read();
            return {};
        }

        auto value =
            reader_.converter_.template convert<T, Ts...>(reader_.split_data_);

        if (!reader_.converter_.valid()) {
            handle_error_invalid_conversion();
        }

        return value;
    }

    bool field_exists(const std::string& field) {
        if (header_.empty()) {
            split_header_data();
        }

        return header_index(field).has_value();
    }

    template <typename... Ts>
    void use_fields(const Ts&... fields_args) {
        if constexpr (ignore_header) {
            handle_error_header_ignored();
            return;
        }

        if (header_.empty()) {
            split_header_data();
        }

        if (!valid()) {
            return;
        }

        auto fields = std::vector<std::string>{fields_args...};

        if (fields.empty()) {
            handle_error_empty_mapping();
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

        if (line() == 1) {
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
            iterator(iterator&& other) = default;

            value& operator*() {
                return value_;
            }

            value* operator->() {
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

            iterator& operator++(int) {
                return ++*this;
            }

            friend bool operator==(const iterator& lhs, const iterator& rhs) {
                return (lhs.parser_ == nullptr && rhs.parser_ == nullptr) ||
                       (lhs.parser_ == rhs.parser_ &&
                        &lhs.value_ == &rhs.value_);
            }

            friend bool operator!=(const iterator& lhs, const iterator& rhs) {
                return !(lhs == rhs);
            }

        private:
            parser<Options...>* parser_;
            value value_;
        };

        iterable(parser<Options...>* parser) : parser_{parser} {
        }

        iterator begin() {
            return ++iterator{parser_};
        }

        iterator end() {
            return iterator{};
        }

    private:
        parser<Options...>* parser_;
    };

    template <typename... Ts>
    auto iterate() {
        return iterable<false, Ts...>{this};
    }

    template <typename... Ts>
    auto iterate_object() {
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
            try_convert_and_invoke<Value, Us...>(value, fun);
            return composite_with(std::move(value));
        }

        // same as or_else, but saves the result into a 'U' object
        // instead of a tuple
        template <typename U, typename... Us, typename Fun = none>
        composite<Ts..., std::optional<U>> or_object(Fun&& fun = none{}) {
            std::optional<U> value;
            try_convert_and_invoke<U, Us...>(value, fun);
            return composite_with(std::move(value));
        }

        std::tuple<Ts...> values() {
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
        composite<Ts..., T> composite_with(T&& new_value) {
            auto merged_values =
                std::tuple_cat(std::move(values_),
                               std::tuple<T>{parser_.valid()
                                                 ? std::forward<T>(new_value)
                                                 : std::nullopt});
            return {std::move(merged_values), parser_};
        }

        template <typename U, typename... Us, typename Fun = none>
        void try_convert_and_invoke(std::optional<U>& value, Fun&& fun) {
            if (!parser_.valid()) {
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
        }

        template <typename U, typename... Us>
        no_void_validator_tup_t<U, Us...> try_same() {
            parser_.clear_error();
            auto value = parser_.reader_.converter_.template convert<U, Us...>(
                parser_.reader_.split_data_);
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
    composite<std::optional<no_void_validator_tup_t<Ts...>>> try_next(
        Fun&& fun = none{}) {
        assert_throw_on_error_not_defined<throw_on_error>();
        using Ret = no_void_validator_tup_t<Ts...>;
        return try_invoke_and_make_composite<
            std::optional<Ret>>(get_next<Ts...>(), std::forward<Fun>(fun));
    }

    // identical to try_next but returns composite with object instead of a
    // tuple
    template <typename T, typename... Ts, typename Fun = none>
    composite<std::optional<T>> try_object(Fun&& fun = none{}) {
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
                if (!try_invoke_impl(arg, std::forward<Fun>(fun))) {
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
    composite<T> try_invoke_and_make_composite(T&& value, Fun&& fun) {
        if (valid()) {
            try_invoke(*value, std::forward<Fun>(fun));
        }
        return {valid() ? std::move(value) : std::nullopt, *this};
    }

    ////////////////
    // header
    ////////////////

    void split_header_data() {
        ss::splitter<Options...> splitter;
        std::string raw_header_copy = raw_header_;
        splitter.split(raw_header_copy.data(), reader_.delim_);
        for (const auto& [begin, end] : splitter.split_data_) {
            std::string field{begin, end};
            if (std::find(header_.begin(), header_.end(), field) !=
                header_.end()) {
                handle_error_invalid_header(field);
                header_.clear();
                return;
            }
            header_.push_back(std::move(field));
        }
    }

    std::optional<size_t> header_index(const std::string& field) {
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
        constexpr static auto error_msg = " failed check";

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
        constexpr static auto error_msg = " could not be opened";

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
        constexpr static auto error_msg = " read on end of file";

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

    void handle_error_invalid_read() {
        if constexpr (string_error) {
            error_.clear();
            error_.append(file_name_)
                .append(" ")
                .append(std::to_string(reader_.line_number_))
                .append(": ")
                .append(reader_.error_msg());
        } else if constexpr (!throw_on_error) {
            error_ = true;
        }
    }

    void handle_error_header_ignored() {
        constexpr static auto error_msg =
            ": the header row is ignored within the setup it cannot be used";

        if constexpr (string_error) {
            error_.clear();
            error_.append(file_name_).append(error_msg);
        } else if constexpr (throw_on_error) {
            throw ss::exception{file_name_ + error_msg};
        } else {
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

    void handle_error_empty_mapping() {
        constexpr static auto error_msg = "received empty mapping";

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg);
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg};
        } else {
            error_ = true;
        }
    }

    void handle_error_invalid_header(const std::string& field) {
        constexpr static auto error_msg = "header contains duplicates: ";

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg).append(error_msg);
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg + field};
        } else {
            error_ = true;
        }
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

    ////////////////
    // new reader
    ////////////////
    struct reader {
        reader(const reader& other) = delete;
        reader& operator=(const reader& other) = delete;

        // TODO update
        reader(reader&& other)
            : delim_{std::move(other.delim_)}, file_{other.file_},
              converter_{std::move(other.converter_)}, buff_{other.buff_},
              line_number_{other.line_number_}, buff_size_{other.buff_size_},
              buff_filled_{other.buff_filled_},
              buff_processed_{other.buff_processed_},
              delim_char_{other.delim_char_}, begin_{other.begin_},
              curr_{other.curr_}, end_{other.end_},
              last_read_{other.last_read_}, split_data_{
                                                std::move(other.split_data_)} {
            other.file_ = nullptr;
            other.buff_ = nullptr;
        }

        reader& operator=(reader&& other) {
            if (this != &other) {
                delim_ = std::move(other.delim_);
                file_ = other.file_;
                converter_ = std::move(other.converter_);
                buff_ = other.buff_;
                line_number_ = other.line_number_;
                buff_filled_ = other.buff_filled_;
                buff_size_ = other.buff_size_;
                buff_processed_ = other.buff_processed_;
                delim_char_ = other.delim_char_;
                begin_ = other.begin_;
                curr_ = other.curr_;
                end_ = other.end_;
                last_read_ = other.last_read_;
                split_data_ = std::move(other.split_data_);

                other.file_ = nullptr;
                other.buff_ = nullptr;
            }
            return *this;
        }

        reader(const std::string& file_name, const std::string& delim)
            : delim_{delim} {

            // TODO update
            delim_char_ = delim_.at(0);

            // TODO check file
            file_ = std::fopen(file_name.c_str(), "rb");

            if (!file_) {
                return;
            }

            // TODO check buff_
            buff_ = static_cast<char*>(std::malloc(buff_size_));

            // TODO check buff_filled
            buff_filled_ = std::fread(buff_, 1, buff_size_, file_);

            if (buff_filled_ != buff_size_) {
                last_read_ = true;
            }

            // TODO handle differently
            if (buff_filled_ == 0 || (buff_filled_ == 1 && buff_[0] == '\n') ||
                (buff_filled_ == 2 && buff_[0] == '\r' && buff_[1] == '\n')) {
                fclose(file_);
                file_ = nullptr;
            }

            begin_ = buff_;
            curr_ = buff_;
            shifted_curr_ = buff_;
            end_ = buff_ + buff_filled_;
        }

        ~reader() {
            free(buff_);

            if (file_) {
                fclose(file_);
            }
        }

        void shift_read_next() {
            buff_filled_ -= buff_processed_;

            // shift back data that is not processed
            memcpy(buff_, buff_ + buff_processed_, buff_filled_);

            // read data to fill the rest of the buffer
            buff_filled_ +=
                fread(buff_ + buff_filled_, 1, buff_processed_, file_);
        }

        void handle_buffer_end_reached() {
            buff_size_ *= 8;

            // TODO handle NULL
            buff_ = static_cast<char*>(std::realloc(buff_, buff_size_));

            // fill the rest of the buffer
            buff_filled_ += fread(buff_ + buff_filled_, 1,
                                  buff_size_ - buff_filled_, file_);

            if (buff_filled_ != buff_size_) {
                last_read_ = true;
            }
        };

        // TODO move
        using quote = typename setup<Options...>::quote;
        using trim_left = typename setup<Options...>::trim_left;
        using trim_right = typename setup<Options...>::trim_right;
        using escape = typename setup<Options...>::escape;
        using multiline = typename setup<Options...>::multiline;

        constexpr static auto is_const_line =
            !quote::enabled && !escape::enabled;
        using line_ptr_type =
            std::conditional_t<is_const_line, const char*, char*>;

        using error_type = std::conditional_t<string_error, std::string, bool>;

        bool valid() const {
            if constexpr (string_error) {
                return error_.empty();
            } else if constexpr (throw_on_error) {
                return true;
            } else {
                return !error_;
            }
        }

        const std::string& error_msg() const {
            assert_string_error_defined<string_error>();
            return error_;
        }

        void clear_error() {
            if constexpr (string_error) {
                error_.clear();
            } else if constexpr (!throw_on_error) {
                error_ = false;
            }
            unterminated_quote_ = false;
        }

        void handle_error_mismatched_quote() {
            constexpr static auto error_msg = "mismatched quote";

            if constexpr (string_error) {
                error_.clear();
                error_.append(error_msg);
            } else if constexpr (throw_on_error) {
                throw ss::exception{error_msg};
            } else {
                error_ = true;
            }
        }

        bool match(const char* const curr, char delim) {
            return *curr == delim;
        };

        // todo test for huge delimiters
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

        // matches delimiter taking spacing into account
        // returns {spacing + delimiter size, is delimiter}
        template <typename Delim>
        std::tuple<size_t, bool> match_delimiter(line_ptr_type begin,
                                                 const Delim& delim) {
            line_ptr_type curr = begin;

            trim_right_if_enabled(curr);

            // just spacing
            // TODO handle \r\n
            if (*curr == '\n' || *curr == '\r') {
                return {0, false};
            }

            // not a delimiter
            if (!match(curr, delim)) {
                shift_if_escaped(curr);
                return {1 + curr - begin, false};
            }

            curr += delimiter_size(delim);
            trim_left_if_enabled(curr);

            // delimiter
            return {curr - begin, true};
        }

        void shift_if_escaped(line_ptr_type& curr) {
            if constexpr (escape::enabled) {
                if (escape::match(*curr)) {
                    // TODO handle differently
                    if (curr[1] == '\0') {
                        if constexpr (!multiline::enabled) {
                            // TODO handle
                            throw "unterminated escape";
                            //  handle_error_unterminated_escape();
                        }
                        return;
                    }
                    shift_and_jump_escape();
                }
            }
        }

        void shift_and_set_shifted_current() {
            if constexpr (!is_const_line) {
                if (escaped_ > 0) {
                    // shift by number of escapes
                    std::copy_n(shifted_curr_ + escaped_,
                                curr_ - shifted_curr_ - escaped_,
                                shifted_curr_);
                    shifted_curr_ = curr_ - escaped_;
                    return;
                }
            }
            shifted_curr_ = curr_;
        }

        void shift_and_jump_escape() {
            if constexpr (!is_const_line && escape::enabled) {
                if (curr_[1] == '\r' && curr_[2] == '\n') {
                    shift_and_set_shifted_current();
                    ++escaped_;
                    ++curr_;
                    check_buff_end();
                    shift_and_set_shifted_current();
                    ++curr_;
                    check_buff_end();
                    return;
                }
            }

            shift_and_set_shifted_current();
            if constexpr (!is_const_line) {
                ++escaped_;
            }
            ++curr_;
        }

        void shift_push_and_start_next(size_t n) {
            shift_and_push();
            begin_ = curr_ + n;
        }

        void shift_and_push() {
            shift_and_set_shifted_current();
            split_data_.emplace_back(begin_, shifted_curr_);
        }

        // TODO check attribute
        __attribute__((always_inline)) void check_buff_end() {
            if (curr_ == end_) {
                auto old_buff = buff_;

                if (last_read_) {
                    // TODO handle
                    throw "no new line at eof";
                }

                handle_buffer_end_reached();
                end_ = buff_ + buff_filled_;

                for (auto& [begin, end] : split_data_) {
                    begin = begin - old_buff + buff_;
                    end = end - old_buff + buff_;
                }

                begin_ = begin_ - old_buff + buff_;
                curr_ = curr_ - old_buff + buff_;
                shifted_curr_ = shifted_curr_ - old_buff + buff_;
            }
        }

        void parse_next_line() {
            while (true) {
                if constexpr (quote::enabled || escape::enabled) {
                    escaped_ = 0;
                }

                // quoted string
                if constexpr (quote::enabled) {
                    if (quote::match(*curr_)) {
                        begin_ = shifted_curr_ = ++curr_;
                        check_buff_end();

                        while (true) {
                            // quote not closed
                            if (!quote::match(*curr_)) {
                                // end of line
                                if constexpr (!multiline::enabled) {
                                    // TODO test \r\n
                                    if (*curr_ == '\n' || *curr_ == '\r') {
                                        throw "unterminated quote";
                                    }
                                }

                                if constexpr (escape::enabled) {
                                    if (escape::match(*curr_)) {
                                        // TODO update
                                        if constexpr (!multiline::enabled) {
                                            if (curr_[1] == '\n' ||
                                                curr_[1] == '\r') {
                                                // eol, unterminated escape
                                                // eg: ... "hel\\n
                                                break;
                                            }
                                            throw "unterminated escape";
                                        }
                                        // not eol

                                        shift_and_jump_escape();
                                        check_buff_end();
                                        continue;
                                    }
                                }

                                ++curr_;

                                check_buff_end();
                                continue;
                            }

                            auto [width, is_delim] =
                                match_delimiter(curr_ + 1, delim_char_);

                            // delimiter
                            if (is_delim) {
                                shift_push_and_start_next(width + 1);
                                curr_ += width + 1;
                                check_buff_end();
                                break;
                            }

                            // double quote
                            // eg: ...,"hel""lo",... -> hel"lo
                            if (quote::match(*(curr_ + 1))) {
                                shift_and_jump_escape();
                                ++curr_;
                                check_buff_end();
                                continue;
                            }

                            if (width == 0) {
                                // eol
                                // eg: ...,"hello"   \n -> hello
                                // eg no trim: ...,"hello"\n -> hello
                                shift_and_push();
                                ++curr_;
                                // TODO handle differently
                                if (curr_[0] == '\r') {
                                    ++curr_;
                                }
                                check_buff_end();
                                return;
                            }

                            // mismatched quote
                            // eg: ...,"hel"lo,... -> error

                            // go to next line, TODO update
                            while (*curr_ != '\n') {
                                ++curr_;
                            }

                            if (throw_on_error) {
                                ++curr_;
                            }

                            handle_error_mismatched_quote();
                            return;
                        }
                    }
                }

                // not quoted
                begin_ = shifted_curr_ = curr_;
                while (true) {
                    // std::cout << "* " << *curr_ << std::endl;

                    auto [width, is_delim] =
                        match_delimiter(curr_, delim_char_);

                    if (!is_delim) {
                        // not a delimiter

                        if (width == 0) {
                            // eol
                            shift_and_push();
                            // ++curr_;
                            // TODO handle differently
                            if (curr_[0] == '\r') {
                                ++curr_;
                            }
                            return;
                        } else {
                            curr_ += width;
                            check_buff_end();
                            continue;
                        }
                    } else {
                        // found delimiter
                        shift_push_and_start_next(width);
                        curr_ += width;
                        check_buff_end();
                        break;
                    }
                }
            }
        }

        // read next line each time in order to set eof_
        bool read_next() {
            // TODO update division value
            if (buff_processed_ > buff_filled_ / 2) {
                if (!last_read_) {
                    shift_read_next();

                    if (buff_filled_ != buff_size_) {
                        last_read_ = true;
                    }

                    curr_ = buff_;
                    end_ = buff_ + buff_filled_;
                }
            }

            split_data_.clear();
            begin_ = curr_;

            parse_next_line();

            ++curr_;
            buff_processed_ = curr_ - buff_;

            // TODO check where to put this
            ++line_number_;

            if (last_read_ && curr_ >= end_) {
                return false;
            }

            return true;
        }

        std::string delim_{};
        FILE* file_{nullptr};

        converter<Options...> converter_;
        char* buff_{nullptr};
        size_t line_number_{0};

        // TODO set initial buffer size
        size_t buff_size_{1};
        size_t buff_filled_{0};
        size_t buff_processed_{0};

        // TODO update
        char delim_char_;

        // TODO move to splitter
        char* begin_{nullptr};
        char* shifted_curr_{nullptr};
        char* curr_{nullptr};
        char* end_{nullptr};

        bool last_read_{false};

        split_data split_data_;

        // TODO add to constructors
        size_t escaped_{0};
        bool unterminated_quote_{true};
        bool unterminated_escape_{true};
        error_type error_;
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

} /* ss */
