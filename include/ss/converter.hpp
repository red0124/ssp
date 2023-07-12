#pragma once
#include "exception.hpp"
#include "extract.hpp"
#include "function_traits.hpp"
#include "restrictions.hpp"
#include "splitter.hpp"
#include "type_traits.hpp"
#include <string>
#include <type_traits>
#include <vector>

namespace ss {
INIT_HAS_METHOD(tied)
INIT_HAS_METHOD(ss_valid)
INIT_HAS_METHOD(error)

////////////////
// replace validator
////////////////

// replace 'validator' types with elements they operate on
// eg. no_validator_tup_t<int, ss::nx<char, 'A', 'B'>> <=> std::tuple<int, char>
// where ss::nx<char, 'A', 'B'> is a validator '(n)one e(x)cept' which
// checks if the returned character is either 'A' or 'B', returns error if not
// additionally if one element is left in the pack, it will be unwrapped from
// the tuple eg. no_void_validator_tup_t<int> <=> int instead of std::tuple<int>
template <typename T, typename U = void>
struct no_validator;

template <typename T>
struct no_validator<T, typename std::enable_if_t<has_m_ss_valid_t<T>>> {
    using type = typename member_wrapper<decltype(&T::ss_valid)>::arg_type;
};

template <typename T, typename U>
struct no_validator {
    using type = T;
};

template <typename T>
using no_validator_t = typename no_validator<T>::type;

template <typename... Ts>
struct no_validator_tup : apply_trait<no_validator, std::tuple<Ts...>> {};

template <typename... Ts>
struct no_validator_tup<std::tuple<Ts...>> : no_validator_tup<Ts...> {};

template <typename T>
struct no_validator_tup<std::tuple<T>> : no_validator<T> {};

template <typename... Ts>
using no_validator_tup_t = typename no_validator_tup<Ts...>::type;

////////////////
// no void tuple
////////////////

template <typename... Ts>
struct no_void_tup : filter_not<std::is_void, no_validator_tup_t<Ts...>> {};

template <typename... Ts>
using no_void_tup_t = filter_not_t<std::is_void, Ts...>;

////////////////
// no void or validator
////////////////

// replace 'validators' and remove void from tuple
template <typename... Ts>
struct no_void_validator_tup : no_validator_tup<no_void_tup_t<Ts...>> {};

template <typename... Ts>
struct no_void_validator_tup<std::tuple<Ts...>>
    : no_validator_tup<no_void_tup_t<Ts...>> {};

template <typename... Ts>
using no_void_validator_tup_t = typename no_void_validator_tup<Ts...>::type;

////////////////
// tied class
////////////////

// check if the parameter pack is only one element which is a class and has
// the 'tied' method which is to be used for type deduction when converting
template <typename T, typename... Ts>
struct tied_class {
    constexpr static bool value =
        (sizeof...(Ts) == 0 && std::is_class_v<T> && has_m_tied<T>::value);
};

template <typename... Ts>
constexpr bool tied_class_v = tied_class<Ts...>::value;

////////////////
// converter
////////////////

template <typename... Options>
class converter {
    using line_ptr_type = typename splitter<Options...>::line_ptr_type;

    constexpr static auto string_error = setup<Options...>::string_error;
    constexpr static auto throw_on_error = setup<Options...>::throw_on_error;
    constexpr static auto default_delimiter = ",";

    using error_type = std::conditional_t<string_error, std::string, bool>;

public:
    // parses line with given delimiter, returns a 'T' object created with
    // extracted values of type 'Ts'
    template <typename T, typename... Ts>
    T convert_object(line_ptr_type line,
                     const std::string& delim = default_delimiter) {
        return to_object<T>(convert<Ts...>(line, delim));
    }

    // parses line with given delimiter, returns tuple of objects with
    // extracted values of type 'Ts'
    template <typename... Ts>
    no_void_validator_tup_t<Ts...> convert(
        line_ptr_type line, const std::string& delim = default_delimiter) {
        split(line, delim);
        if (splitter_.valid()) {
            return convert<Ts...>(splitter_.split_data_);
        } else {
            set_error_bad_split();
            return {};
        }
    }

    // parses already split line, returns 'T' object with extracted values
    template <typename T, typename... Ts>
    T convert_object(const split_data& elems) {
        return to_object<T>(convert<Ts...>(elems));
    }

    // same as above, but uses cached split line
    template <typename T, typename... Ts>
    T convert_object() {
        return to_object<T>(convert<Ts...>());
    }

    // parses already split line, returns either a tuple of objects with
    // parsed values (returns raw element (no tuple) if Ts is empty), or if
    // one argument is given which is a class which has a tied
    // method which returns a tuple, returns that type
    template <typename T, typename... Ts>
    no_void_validator_tup_t<T, Ts...> convert(const split_data& elems) {
        if constexpr (sizeof...(Ts) == 0 && is_instance_of_v<std::tuple, T>) {
            return convert_impl(elems, static_cast<T*>(nullptr));
        } else if constexpr (tied_class_v<T, Ts...>) {
            using arg_ref_tuple = std::result_of_t<decltype (&T::tied)(T)>;
            using arg_tuple = apply_trait_t<std::decay, arg_ref_tuple>;

            return to_object<T>(
                convert_impl(elems, static_cast<arg_tuple*>(nullptr)));
        } else {
            return convert_impl<T, Ts...>(elems);
        }
    }

    // same as above, but uses cached split line
    template <typename T, typename... Ts>
    no_void_validator_tup_t<T, Ts...> convert() {
        return convert<T, Ts...>(splitter_.split_data_);
    }

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
        return splitter_.unterminated_quote();
    }

    // 'splits' string by given delimiter, returns vector of pairs which
    // contain the beginnings and the ends of each column of the string
    const split_data& split(line_ptr_type line,
                            const std::string& delim = default_delimiter) {
        splitter_.split_data_.clear();
        if (line[0] == '\0') {
            return splitter_.split_data_;
        }

        return splitter_.split(line, delim);
    }

private:
    ////////////////
    // resplit
    ////////////////

    const split_data& resplit(line_ptr_type new_line, ssize_t new_size,
                              const std::string& delim = default_delimiter) {
        return splitter_.resplit(new_line, new_size, delim);
    }

    size_t size_shifted() {
        return splitter_.size_shifted();
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

    std::string error_sufix(const string_range msg, size_t pos) const {
        std::string error;
        error.reserve(32);
        error.append("at column ")
            .append(std::to_string(pos + 1))
            .append(": \'")
            .append(msg.first, msg.second)
            .append("\'");
        return error;
    }

    void set_error_bad_split() {
        if constexpr (string_error) {
            error_.clear();
            error_.append(splitter_.error_msg());
        } else if constexpr (!throw_on_error) {
            error_ = true;
        }
    }

    void set_error_unterminated_escape() {
        if constexpr (string_error) {
            error_.clear();
            splitter_.set_error_unterminated_escape();
            error_.append(splitter_.error_msg());
        } else if constexpr (throw_on_error) {
            splitter_.set_error_unterminated_escape();
        } else {
            error_ = true;
        }
    }

    void set_error_unterminated_quote() {
        if constexpr (string_error) {
            error_.clear();
            splitter_.set_error_unterminated_quote();
            error_.append(splitter_.error_msg());
        } else if constexpr (throw_on_error) {
            splitter_.set_error_unterminated_quote();
        } else {
            error_ = true;
        }
    }

    void set_error_multiline_limit_reached() {
        constexpr static auto error_msg = "multiline limit reached";

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg);
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg};
        } else {
            error_ = true;
        }
    }

    void set_error_invalid_conversion(const string_range msg, size_t pos) {
        constexpr static auto error_msg = "invalid conversion for parameter ";

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg).append(error_sufix(msg, pos));
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg + error_sufix(msg, pos)};
        } else {
            error_ = true;
        }
    }

    void set_error_validate(const char* const error, const string_range msg,
                            size_t pos) {
        if constexpr (string_error) {
            error_.clear();
            error_.append(error).append(" ").append(error_sufix(msg, pos));
        } else if constexpr (throw_on_error) {
            throw ss::exception{error + (" " + error_sufix(msg, pos))};
        } else {
            error_ = true;
        }
    }

    void set_error_number_of_columns(size_t expected_pos, size_t pos) {
        constexpr static auto error_msg1 =
            "invalid number of columns, expected: ";
        constexpr static auto error_msg2 = ", got: ";

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg1)
                .append(std::to_string(expected_pos))
                .append(error_msg2)
                .append(std::to_string(pos));
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg1 + std::to_string(expected_pos) +
                                error_msg2 + std::to_string(pos)};
        } else {
            error_ = true;
        }
    }

    void set_error_incompatible_mapping(size_t argument_size,
                                        size_t mapping_size) {
        constexpr static auto error_msg1 =
            "number of arguments does not match mapping, expected: ";
        constexpr static auto error_msg2 = ", got: ";

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg1)
                .append(std::to_string(mapping_size))
                .append(error_msg2)
                .append(std::to_string(argument_size));
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg1 + std::to_string(mapping_size) +
                                error_msg2 + std::to_string(argument_size)};
        } else {
            error_ = true;
        }
    }

    void set_error_invalid_mapping() {
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

    void set_error_mapping_out_of_range(size_t maximum_index,
                                        size_t number_of_columnts) {
        constexpr static auto error_msg1 = "maximum index: ";
        constexpr static auto error_msg2 = ", greater than number of columns: ";

        if constexpr (string_error) {
            error_.clear();
            error_.append(error_msg1)
                .append(std::to_string(maximum_index))
                .append(error_msg2)
                .append(std::to_string(number_of_columnts));
        } else if constexpr (throw_on_error) {
            throw ss::exception{error_msg1 + std::to_string(maximum_index) +
                                error_msg2 +
                                std::to_string(number_of_columnts)};
        } else {
            error_ = true;
        }
    }

    ////////////////
    // convert implementation
    ////////////////

    template <typename... Ts>
    no_void_validator_tup_t<Ts...> convert_impl(const split_data& elems) {
        clear_error();

        if (!splitter_.valid()) {
            set_error_bad_split();
            return {};
        }

        if (!columns_mapped()) {
            if (sizeof...(Ts) != elems.size()) {
                set_error_number_of_columns(sizeof...(Ts), elems.size());
                return {};
            }
        } else {
            if (sizeof...(Ts) != column_mappings_.size()) {
                set_error_incompatible_mapping(sizeof...(Ts),
                                               column_mappings_.size());
                return {};
            }

            if (elems.size() != number_of_columns_) {
                set_error_number_of_columns(number_of_columns_, elems.size());
                return {};
            }
        }

        return extract_tuple<Ts...>(elems);
    }

    // do not know how to specialize by return type :(
    template <typename... Ts>
    no_void_validator_tup_t<std::tuple<Ts...>> convert_impl(
        const split_data& elems, const std::tuple<Ts...>*) {
        return convert_impl<Ts...>(elems);
    }

    ////////////////
    // column mapping
    ////////////////

    bool columns_mapped() const {
        return column_mappings_.size() != 0;
    }

    size_t column_position(size_t tuple_position) const {
        if (!columns_mapped()) {
            return tuple_position;
        }
        return column_mappings_[tuple_position];
    }

    void set_column_mapping(std::vector<size_t> positions,
                            size_t number_of_columns) {
        if (positions.empty()) {
            set_error_invalid_mapping();
            return;
        }

        auto max_index = *std::max_element(positions.begin(), positions.end());
        if (max_index >= number_of_columns) {
            set_error_mapping_out_of_range(max_index, number_of_columns);
            return;
        }

        column_mappings_ = positions;
        number_of_columns_ = number_of_columns;
    }

    void clear_column_positions() {
        column_mappings_.clear();
        number_of_columns_ = 0;
    }

    ////////////////
    // conversion
    ////////////////

    template <typename T>
    void extract_one(no_validator_t<T>& dst, const string_range msg,
                     size_t pos) {
        if (!valid()) {
            return;
        }

        if constexpr (std::is_same_v<T, std::string>) {
            extract(msg.first, msg.second, dst);
            return;
        }

        if (!extract(msg.first, msg.second, dst)) {
            set_error_invalid_conversion(msg, pos);
            return;
        }

        if constexpr (has_m_ss_valid_t<T>) {
            if (T validator; !validator.ss_valid(dst)) {
                if constexpr (has_m_error_t<T>) {
                    set_error_validate(validator.error(), msg, pos);
                } else {
                    set_error_validate("validation error", msg, pos);
                }
                return;
            }
        }
    }

    template <size_t ArgN, size_t TupN, typename... Ts>
    void extract_multiple(no_void_validator_tup_t<Ts...>& tup,
                          const split_data& elems) {
        using elem_t = std::tuple_element_t<ArgN, std::tuple<Ts...>>;

        constexpr bool not_void = !std::is_void_v<elem_t>;
        constexpr bool one_element = count_not_v<std::is_void, Ts...> == 1;

        if constexpr (not_void) {
            if constexpr (one_element) {
                extract_one<elem_t>(tup, elems[column_position(ArgN)], ArgN);
            } else {
                auto& el = std::get<TupN>(tup);
                extract_one<elem_t>(el, elems[column_position(ArgN)], ArgN);
            }
        }

        if constexpr (sizeof...(Ts) > ArgN + 1) {
            constexpr size_t NewTupN = (not_void) ? TupN + 1 : TupN;
            extract_multiple<ArgN + 1, NewTupN, Ts...>(tup, elems);
        }
    }

    template <typename... Ts>
    no_void_validator_tup_t<Ts...> extract_tuple(const split_data& elems) {
        static_assert(!all_of_v<std::is_void, Ts...>,
                      "at least one parameter must be non void");
        no_void_validator_tup_t<Ts...> ret{};
        extract_multiple<0, 0, Ts...>(ret, elems);
        return ret;
    }

    ////////////////
    // members
    ////////////////

    error_type error_{};
    splitter<Options...> splitter_;

    template <typename...>
    friend class parser;

    std::vector<size_t> column_mappings_;
    size_t number_of_columns_;
};

} /* ss */
