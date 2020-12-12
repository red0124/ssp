#pragma once

#include "converter.hpp"
#include "extract.hpp"
#include "restrictions.hpp"
#include <cstring>
#include <optional>
#include <stdlib.h>
#include <string>
#include <vector>

namespace ss {

struct None {};
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
                return (error_mode_ == error_mode::String)
                           ? string_error_.empty()
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
        T get_struct() {
                return to_struct<T>(get_next<Ts...>());
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
                composite(std::tuple<Ts...> values, parser& parser)
                    : values_{values}, parser_{parser} {
                }

                // tries to convert the same line as different output if the 
                // previous conversion was not successful, returns composite
                // containing itself and the new output as optional
                // if a parameter is passed which can be invoked with
                // the new output, it will be invoked if the returned value
                // of the conversion is valid
                template <typename... Us, typename Fun = None>
                composite<Ts..., std::optional<no_void_validator_tup_t<Us...>>>
                or_else(Fun&& fun = None{}) {
                        std::optional<no_void_validator_tup_t<Us...>> value;

                        if (!parser_.valid()) {
                                auto new_value = try_same<Us...>();
                                if (parser_.valid()) {
                                        value = new_value;
                                        if constexpr (!std::is_same_v<Fun,
                                                                      None>) {
                                                fun(*value);
                                        }
                                }
                        }

                        auto values =
                            std::tuple_cat(std::move(values_),
                                           std::tuple{std::move(value)});
                        return {std::move(values), parser_};
                }

                auto values() { return values_; }

            private:
                template <typename U, typename... Us>
                no_void_validator_tup_t<U, Us...> try_same() {
                        parser_.clear_error();
                        auto value = parser_.converter_.convert<U, Us...>(
                            parser_.split_input_);
                        if (!parser_.converter_.valid()) {
                                parser_.set_error_invalid_conversion();
                        }
                        return value;
                }

                std::tuple<Ts...> values_;
                parser& parser_;
        };

        // tries to convert a line, but returns a composite which is
        // able to chain additional conversions in case of failure
        template <typename... Ts, typename Fun = None>
        composite<std::optional<no_void_validator_tup_t<Ts...>>> try_next(
            Fun&& fun = None{}) {
                std::optional<no_void_validator_tup_t<Ts...>> value;
                auto new_value = get_next<Ts...>();
                if (valid()) {
                        value = new_value;
                        if constexpr (!std::is_same_v<Fun, None>) {
                                fun(*value);
                        }
                }
                return {value, *this};
        };

    private:
        template <typename...>
        friend class composite;

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

        void set_error_file_not_open() {
                if (error_mode_ == error_mode::String) {
                        string_error_.append(file_name_)
                            .append(" could not be not open.");
                } else {
                        bool_error_ = true;
                }
        }

        void set_error_eof_reached() {
                if (error_mode_ == error_mode::String) {
                        string_error_.append(file_name_)
                            .append(" reached end of file.");
                } else {
                        bool_error_ = true;
                }
        }

        void set_error_invalid_conversion() {
                if (error_mode_ == error_mode::String) {
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
        bool bool_error_;
        error_mode error_mode_{error_mode::String};
        converter converter_;
        converter::split_input split_input_;
        FILE* file_{nullptr};
        buffer buff_;
        size_t line_number_{0};
        bool eof_{false};
};

} /* ss */
