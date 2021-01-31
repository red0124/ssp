#pragma once

#include "converter.hpp"
#include "extract.hpp"
#include "restrictions.hpp"
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

// TODO remove
#include <iostream>

namespace ss {

template <typename... Matchers>
class parser {
    struct none {};

public:
    parser(const std::string& file_name, const std::string& delimiter)
        : file_name_{file_name}, delim_{delimiter},
          file_{fopen(file_name_.c_str(), "rb")} {
        if (file_) {
            read_line();
        } else {
            set_error_file_not_open();
            eof_ = true;
        }
    }

    ~parser() {
        if (file_) {
            fclose(file_);
        }
    }

    bool valid() const {
        return (error_mode_ == error_mode::error_string) ? string_error_.empty()
                                                         : bool_error_ == false;
    }

    void set_error_mode(error_mode mode) {
        error_mode_ = mode;
        reader_.set_error_mode(mode);
    }

    const std::string& error_msg() const {
        return string_error_;
    }

    bool eof() const {
        return eof_;
    }

    bool ignore_next() {
        return reader_.read(file_);
    }

    template <typename T, typename... Ts>
    T get_object() {
        return to_object<T>(get_next<Ts...>());
    }

    template <typename T, typename... Ts>
    no_void_validator_tup_t<T, Ts...> get_next() {
        reader_.update();
        clear_error();
        if (eof_) {
            set_error_eof_reached();
            return {};
        }

        auto value = reader_.get_converter().template convert<T, Ts...>();

        if (!reader_.get_converter().valid()) {
            set_error_invalid_conversion();
        }

        read_line();
        return value;
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
            if (!parser_.valid()) {
                if constexpr (std::is_invocable_v<Fun>) {
                    fun();
                } else {
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
            auto value =
                parser_.reader_.get_converter().template convert<U, Us...>();
            if (!parser_.reader_.get_converter().valid()) {
                parser_.set_error_invalid_conversion();
            }
            return value;
        }

        std::tuple<Ts...> values_;
        parser& parser_;
    };

    // tries to convert a line and returns a composite which is
    // able to try additional conversions in case of failure
    template <typename... Ts, typename Fun = none>
    composite<std::optional<no_void_validator_tup_t<Ts...>>> try_next(
        Fun&& fun = none{}) {
        using Ret = no_void_validator_tup_t<Ts...>;
        return try_invoke_and_make_composite<
            std::optional<Ret>>(get_next<Ts...>(), std::forward<Fun>(fun));
    }

    // identical to try_next but returns composite with object instead of a
    // tuple
    template <typename T, typename... Ts, typename Fun = none>
    composite<std::optional<T>> try_object(Fun&& fun = none{}) {
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
                    set_error_failed_check();
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
    // line reading
    ////////////////

    class reader {
        char* buffer_{nullptr};
        char* next_line_buffer_{nullptr};
        char* helper_buffer_{nullptr};

        converter<Matchers...> converter_;
        converter<Matchers...> next_line_converter_;

        size_t size_{0};
        size_t helper_size_{0};
        const std::string& delim_;

        bool crlf;

        bool escaped_eol(size_t size) {
            // escaped new line
            if constexpr (setup<Matchers...>::escape::enabled) {
                const char* curr;
                for (curr = next_line_buffer_ + size - 1;
                     curr >= next_line_buffer_ &&
                     setup<Matchers...>::escape::match(*curr);
                     --curr) {
                }
                return (next_line_buffer_ - curr + size) % 2 == 0;
            }

            return false;
        }

        bool unterminated_quote() {
            // unterimated quote
            if constexpr (ss::setup<Matchers...>::quote::enabled) {
                if (next_line_converter_.unterminated_quote()) {
                    return true;
                }
            }
            return false;
        }

        void undo_remove_eol(size_t& string_end) {
            if (crlf) {
                memcpy(next_line_buffer_ + string_end, "\r\n\0", 3);
                string_end += 2;
            } else {
                memcpy(next_line_buffer_ + string_end, "\n\0", 2);
                string_end += 1;
            }
        }

        size_t remove_eol(char*& buffer, size_t size) {
            size_t new_size = size - 1;
            if (size >= 2 && buffer[size - 2] == '\r') {
                crlf = true;
                new_size--;
            } else {
                crlf = false;
            }

            buffer[new_size] = '\0';
            return new_size;
        }

        void realloc_concat(char*& first, size_t& first_size,
                            const char* const second, size_t second_size) {
            first = static_cast<char*>(realloc(static_cast<void*>(first),
                                               first_size + second_size + 2));

            memcpy(first + first_size, second, second_size + 1);
            first_size += second_size;
        }

        bool append_line(FILE* file, char*& dst_buffer, size_t& dst_size) {
            undo_remove_eol(dst_size);

            ssize_t ssize = getline(&helper_buffer_, &helper_size_, file);
            if (ssize == -1) {
                return false;
            }

            size_t size = remove_eol(helper_buffer_, ssize);
            realloc_concat(dst_buffer, dst_size, helper_buffer_, size);
            return true;
        }

    public:
        reader(const std::string& delimiter) : delim_{delimiter} {
        }

        ~reader() {
            free(buffer_);
            free(next_line_buffer_);
            free(helper_buffer_);
        }

        bool read(FILE* file) {
            ssize_t ssize = getline(&next_line_buffer_, &size_, file);

            if (ssize == -1) {
                return false;
            }

            size_t size = remove_eol(next_line_buffer_, ssize);

            while (escaped_eol(size)) {
                if (!append_line(file, next_line_buffer_, size)) {
                    return false;
                }
            }

            next_line_converter_.split(next_line_buffer_, delim_);

            while (unterminated_quote()) {
                if (!append_line(file, next_line_buffer_, size)) {
                    return false;
                }
                next_line_converter_.resplit(next_line_buffer_, size);
            }

            return true;
        }

        void set_error_mode(error_mode mode) {
            converter_.set_error_mode(mode);
            next_line_converter_.set_error_mode(mode);
        }

        converter<Matchers...>& get_converter() {
            return converter_;
        }

        const char* get_buffer() const {
            return buffer_;
        }

        void update() {
            std::swap(buffer_, next_line_buffer_);
            std::swap(converter_, next_line_converter_);
        }
    };

    void read_line() {
        eof_ = !reader_.read(file_);
        ++line_number_;
    }

    ////////////////
    // error
    ////////////////

    void clear_error() {
        string_error_.clear();
        bool_error_ = false;
    }

    void set_error_failed_check() {
        if (error_mode_ == error_mode::error_string) {
            string_error_.append(file_name_).append(" failed check.");
        } else {
            bool_error_ = true;
        }
    }

    void set_error_file_not_open() {
        string_error_.append(file_name_).append(" could not be opened.");
        bool_error_ = true;
    }

    void set_error_eof_reached() {
        if (error_mode_ == error_mode::error_string) {
            string_error_.append(file_name_).append(" reached end of file.");
        } else {
            bool_error_ = true;
        }
    }

    void set_error_invalid_conversion() {
        if (error_mode_ == error_mode::error_string) {
            string_error_.append(file_name_)
                .append(" ")
                .append(std::to_string(line_number_))
                .append(": ")
                .append(reader_.get_converter().error_msg())
                .append(": \"")
                .append(reader_.get_buffer())
                .append("\"");
        } else {
            bool_error_ = true;
        }
    }

    ////////////////
    // members
    ////////////////

    const std::string file_name_;
    const std::string delim_;
    std::string string_error_;
    bool bool_error_{false};
    error_mode error_mode_{error_mode::error_bool};
    FILE* file_{nullptr};
    reader reader_{delim_};
    size_t line_number_{0};
    bool eof_{false};
};

} /* ss */
