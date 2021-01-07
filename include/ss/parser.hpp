#pragma once

#include "converter.hpp"
#include "extract.hpp"
#include "restrictions.hpp"
#include <cstring>
#include <optional>
#include <cstdlib>
#include <string>
#include <vector>

namespace ss {

struct none {};
template <typename...>
class composite;

class parser {
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
        fclose(file_);
    }

    bool valid() const {
        return (error_mode_ == error_mode::error_string) ? string_error_.empty()
                                                         : bool_error_ == false;
    }

    void set_error_mode(error_mode mode) {
        error_mode_ = mode;
        converter_.set_error_mode(mode);
    }

    const std::string& error_msg() const {
        return string_error_;
    }

    bool eof() const {
        return eof_;
    }

    bool ignore_next() {
        return buff_.read(file_);
    }

    template <typename T, typename... Ts>
    T get_object() {
        return to_object<T>(get_next<Ts...>());
    }

    template <typename T, typename... Ts>
    no_void_validator_tup_t<T, Ts...> get_next() {
        buff_.update();
        clear_error();
        if (eof_) {
            set_error_eof_reached();
            return {};
        }

        split_input_ = converter_.split(buff_.get(), delim_);
        auto value = converter_.convert<T, Ts...>(split_input_);

        if (!converter_.valid()) {
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
                parser_.converter_.convert<U, Us...>(parser_.split_input_);
            if (!parser_.converter_.valid()) {
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
    };

    // identical to try_next but returns composite with object instead of a
    // tuple
    template <typename T, typename... Ts, typename Fun = none>
    composite<std::optional<T>> try_object(Fun&& fun = none{}) {
        return try_invoke_and_make_composite<
            std::optional<T>>(get_object<T, Ts...>(), std::forward<Fun>(fun));
    };

private:
    template <typename...>
    friend class composite;

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

    class buffer {
        char* buffer_{nullptr};
        char* new_buffer_{nullptr};
        size_t size_{0};

    public:
        ~buffer() {
            free(buffer_);
            free(new_buffer_);
        }

        bool read(FILE* file) {
            ssize_t size = getline(&new_buffer_, &size_, file);
            size_t string_end = size - 1;

            if (size == -1) {
                return false;
            }

            if (size >= 2 && new_buffer_[size - 2] == '\r') {
                string_end--;
            }

            new_buffer_[string_end] = '\0';
            return true;
        }

        const char* get() const {
            return buffer_;
        }

        void update() {
            std::swap(buffer_, new_buffer_);
        }
    };

    void read_line() {
        eof_ = !buff_.read(file_);
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
                .append(converter_.error_msg())
                .append(": \"")
                .append(buff_.get())
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
    converter converter_;
    converter::split_input split_input_;
    FILE* file_{nullptr};
    buffer buff_;
    size_t line_number_{0};
    bool eof_{false};
};

} /* ss */
