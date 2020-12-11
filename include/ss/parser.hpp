#pragma once

#include "converter.hpp"
#include "extract.hpp"
#include "restrictions.hpp"
#include <cstring>
#include <stdlib.h>
#include <string>
#include <vector>

namespace ss {

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
                return error_.empty();
        }

        bool eof() const {
                return eof_;
        }

        bool ignore_next() {
                return buff_.read(file_);
        }

        const std::string& error() const {
                return error_;
        }

        template <typename T, typename... Ts>
        T get_struct() {
                return to_struct<T>(get_next<Ts...>());
        }

        template <typename T, typename... Ts>
        no_void_validator_tup_t<T, Ts...> get_next() {
                error_.clear();
                if (eof_) {
                        set_error_eof_reached();
                        return {};
                }

                auto value = converter_.convert<T, Ts...>(buff_.get(), delim_);

                if (!converter_.valid()) {
                        set_error_invalid_conversion();
                }

                read_line();
                return value;
        }

    private:
        ////////////////
        // line reading
        ////////////////

        class buffer {
                char* buffer_{nullptr};
                size_t size_{0};

            public:
                ~buffer() {
                        free(buffer_);
                }

                bool read(FILE* file) {
                        ssize_t size = getline(&buffer_, &size_, file);
                        size_t string_end = size - 1;

                        if (size == -1) {
                                return false;
                        }

                        if (size >= 2 && buffer_[size - 2] == '\r') {
                                string_end--;
                        }

                        buffer_[string_end] = '\0';
                        return true;
                }

                const char* get() const {
                        return buffer_;
                }
        };

        void read_line() {
                eof_ = !buff_.read(file_);
                ++line_number_;
        }

        ////////////////
        // error
        ////////////////

        void set_error_file_not_open() {
                error_.append(file_name_).append(" could not be not open.");
        }

        void set_error_eof_reached() {
                error_.append(file_name_).append(" reached end of file.");
        }

        void set_error_invalid_conversion() {
                error_.append(file_name_)
                    .append(" ")
                    .append(std::to_string(line_number_))
                    .append(": ")
                    .append(converter_.error())
                    .append(": \"")
                    .append(buff_.get());
                error_.append("\"");
        }

        ////////////////
        // members
        ////////////////

        const std::string file_name_;
        const std::string delim_;
        std::string error_;
        ss::converter converter_;
        FILE* file_{nullptr};
        buffer buff_;
        size_t line_number_{0};
        bool eof_{false};
};

} /* ss */
