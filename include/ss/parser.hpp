#pragma once

#include "common.hpp"
#include "converter.hpp"
#include "extract.hpp"
#include "restrictions.hpp"
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

namespace ss {

template <typename... Matchers>
class parser {
    constexpr static auto string_error = setup<Matchers...>::string_error;

    using multiline = typename setup<Matchers...>::multiline;
    using error_type = ss::ternary_t<string_error, std::string, bool>;

    constexpr static bool escaped_multiline_enabled =
        multiline::enabled && setup<Matchers...>::escape::enabled;

    constexpr static bool quoted_multiline_enabled =
        multiline::enabled && setup<Matchers...>::quote::enabled;

public:
    parser(const std::string& file_name,
           const std::string& delim = ss::default_delimiter)
        : file_name_{file_name}, reader_{file_name_, delim} {
        if (reader_.file_) {
            read_line();
        } else {
            set_error_file_not_open();
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

    template <typename T, typename... Ts>
    no_void_validator_tup_t<T, Ts...> get_next() {
        reader_.update();
        clear_error();
        if (eof_) {
            set_error_eof_reached();
            return {};
        }

        auto value = reader_.converter_.template convert<T, Ts...>();

        if (!reader_.converter_.valid()) {
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
            auto value =
                parser_.reader_.converter_.template convert<U, Us...>();
            if (!parser_.reader_.converter_.valid()) {
                parser_.set_error_invalid_conversion();
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
    // error
    ////////////////

    void clear_error() {
        if constexpr (string_error) {
            error_.clear();
        } else {
            error_ = false;
        }
    }

    void set_error_failed_check() {
        if constexpr (string_error) {
            error_.append(file_name_).append(" failed check.");
        } else {
            error_ = true;
        }
    }

    void set_error_file_not_open() {
        if constexpr (string_error) {
            error_.append(file_name_).append(" could not be opened.");
        } else {
            error_ = true;
        }
    }

    void set_error_eof_reached() {
        if constexpr (string_error) {
            error_.append(file_name_).append(" reached end of file.");
        } else {
            error_ = true;
        }
    }

    void set_error_invalid_conversion() {
        if constexpr (string_error) {
            error_.append(file_name_)
                .append(" ")
                .append(std::to_string(line_number_))
                .append(": ")
                .append(reader_.converter_.error_msg())
                .append(": \"")
                .append(reader_.buffer_)
                .append("\"");
        } else {
            error_ = true;
        }
    }

    ////////////////
    // line reading
    ////////////////

    void read_line() {
        eof_ = !reader_.read_next();
        ++line_number_;
    }

    struct reader {
        reader(const std::string& file_name_, const std::string& delim)
            : delim_{delim}, file_{fopen(file_name_.c_str(), "rb")} {
        }

        reader(reader&& other)
            : buffer_{other.buffer_},
              next_line_buffer_{other.next_line_buffer_},
              helper_buffer_{other.helper_buffer_}, converter_{std::move(
                                                        other.converter_)},
              next_line_converter_{std::move(other.next_line_converter_)},
              size_{other.size_}, next_line_size_{other.size_},
              helper_size_{other.helper_size_}, delim_{std::move(other.delim_)},
              file_{other.file_}, crlf_{other.crlf_} {
            other.buffer_ = nullptr;
            other.next_line_buffer_ = nullptr;
            other.helper_buffer_ = nullptr;
            other.file_ = nullptr;
        }

        reader& operator=(reader&& other) {
            if (this != &other) {
                buffer_ = other.buffer_;
                next_line_buffer_ = other.next_line_buffer_;
                helper_buffer_ = other.helper_buffer_;
                converter_ = std::move(other.converter_);
                next_line_converter_ = std::move(other.next_line_converter_);
                size_ = other.size_;
                next_line_size_ = other.next_line_size_;
                helper_size_ = other.helper_size_;
                delim_ = std::move(other.delim_);
                file_ = other.file_;
                crlf_ = other.crlf_;

                other.buffer_ = nullptr;
                other.next_line_buffer_ = nullptr;
                other.helper_buffer_ = nullptr;
                other.file_ = nullptr;
            }

            return *this;
        }

        ~reader() {
            free(buffer_);
            free(next_line_buffer_);
            free(helper_buffer_);

            if (file_) {
                fclose(file_);
            }
        }

        reader() = delete;
        reader(const reader& other) = delete;
        reader& operator=(const reader& other) = delete;

        bool read_next() {
            memset(next_line_buffer_, '\0', next_line_size_);
            ssize_t ssize =
                get_line(&next_line_buffer_, &next_line_size_, file_);

            if (ssize == -1) {
                return false;
            }

            size_t size = remove_eol(next_line_buffer_, ssize);
            size_t limit = 0;

            if constexpr (escaped_multiline_enabled) {
                while (escaped_eol(size)) {
                    if (multiline_limit_reached(limit)) {
                        return true;
                    }
                    if (!append_next_line_to_buffer(next_line_buffer_, size)) {
                        return false;
                    }
                }
            }

            next_line_converter_.split(next_line_buffer_, delim_);

            if constexpr (quoted_multiline_enabled) {
                while (unterminated_quote()) {
                    if (multiline_limit_reached(limit)) {
                        return true;
                    }
                    if (!append_next_line_to_buffer(next_line_buffer_, size)) {
                        return false;
                    }

                    if constexpr (escaped_multiline_enabled) {
                        while (escaped_eol(size)) {
                            if (multiline_limit_reached(limit)) {
                                return true;
                            }
                            if (!append_next_line_to_buffer(next_line_buffer_,
                                                            size)) {
                                return false;
                            }
                        }
                    }

                    next_line_converter_.resplit(next_line_buffer_, size);
                }
            }

            return true;
        }

        void update() {
            std::swap(buffer_, next_line_buffer_);
            std::swap(size_, next_line_size_);
            std::swap(converter_, next_line_converter_);
        }

        bool multiline_limit_reached(size_t& limit) {
            if constexpr (multiline::size > 0) {
                if (limit++ >= multiline::size) {
                    next_line_converter_.set_error_multiline_limit_reached();
                    return true;
                }
            }
            return false;
        }

        bool escaped_eol(size_t size) {
            const char* curr;
            for (curr = next_line_buffer_ + size - 1;
                 curr >= next_line_buffer_ &&
                 setup<Matchers...>::escape::match(*curr);
                 --curr) {
            }
            return (next_line_buffer_ - curr + size) % 2 == 0;
        }

        bool unterminated_quote() {
            if (next_line_converter_.unterminated_quote()) {
                return true;
            }
            return false;
        }

        void undo_remove_eol(char* buffer, size_t& string_end) {
            if (next_line_converter_.unterminated_quote()) {
                string_end -= next_line_converter_.size_shifted();
            }
            if (crlf_) {
                std::copy_n("\r\n\0", 3, buffer + string_end);
                string_end += 2;
            } else {
                std::copy_n("\n\0", 2, buffer + string_end);
                string_end += 1;
            }
        }

        size_t remove_eol(char*& buffer, size_t size) {
            size_t new_size = size - 1;
            if (size >= 2 && buffer[size - 2] == '\r') {
                crlf_ = true;
                new_size--;
            } else {
                crlf_ = false;
            }

            buffer[new_size] = '\0';
            return new_size;
        }

        void realloc_concat(char*& first, size_t& first_size,
                            const char* const second, size_t second_size) {
            next_line_size_ = first_size + second_size + 2;
            first = static_cast<char*>(
                realloc(static_cast<void*>(first), next_line_size_));
            std::copy_n(second, second_size + 1, first + first_size);
            first_size += second_size;
        }

        bool append_next_line_to_buffer(char*& buffer, size_t& size) {
            undo_remove_eol(buffer, size);

            ssize_t next_ssize =
                get_line(&helper_buffer_, &helper_size_, file_);
            if (next_ssize == -1) {
                return false;
            }

            size_t next_size = remove_eol(helper_buffer_, next_ssize);
            realloc_concat(buffer, size, helper_buffer_, next_size);
            return true;
        }

        ////////////////
        // members
        ////////////////
        char* buffer_{nullptr};
        char* next_line_buffer_{nullptr};
        char* helper_buffer_{nullptr};

        converter<Matchers...> converter_;
        converter<Matchers...> next_line_converter_;

        size_t size_{0};
        size_t next_line_size_{0};
        size_t helper_size_{0};

        std::string delim_;
        FILE* file_{nullptr};

        bool crlf_;
    };

    ////////////////
    // members
    ////////////////

    std::string file_name_;
    error_type error_{};
    reader reader_;
    size_t line_number_{0};
    bool eof_{false};
};

} /* ss */
