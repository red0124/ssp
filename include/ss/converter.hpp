#pragma once

#include "extract.hpp"
#include "function_traits.hpp"
#include "restrictions.hpp"
#include "type_traits.hpp"
#include <string>
#include <type_traits>
#include <vector>

namespace ss {
INIT_HAS_METHOD(tied);
INIT_HAS_METHOD(ss_valid);
INIT_HAS_METHOD(error);

////////////////
// replace validator
////////////////

// replace 'validator' types with elements they operate on
// eg. no_validator_tup_t<int, ss::nx<char, 'A', 'B'>> <=> std::tuple<int, char>
// where ss::nx<char, 'A', 'B'> is a validator '(n)one e(x)cept' which
// checks if the returned character is either 'A' or 'B', returns error if not
// additionaly if one element is left in the pack, it will be unwraped from
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
struct no_validator_tup {
        using type =
            typename apply_trait<no_validator, std::tuple<Ts...>>::type;
};

template <typename... Ts>
struct no_validator_tup<std::tuple<Ts...>> {
        using type = typename no_validator_tup<Ts...>::type;
};

template <typename T>
struct no_validator_tup<std::tuple<T>> {
        using type = no_validator_t<T>;
};

template <typename... Ts>
using no_validator_tup_t = typename no_validator_tup<Ts...>::type;

////////////////
// no void tuple
////////////////

template <typename... Ts>
struct no_void_tup {
        using type =
            typename filter_not<std::is_void, no_validator_tup_t<Ts...>>::type;
};

template <typename... Ts>
using no_void_tup_t = filter_not_t<std::is_void, Ts...>;

////////////////
// no void or validator
////////////////

// replace 'validators' and remove void from tuple
template <typename... Ts>
struct no_void_validator_tup {
        using type = no_validator_tup_t<no_void_tup_t<Ts...>>;
};

template <typename... Ts>
using no_void_validator_tup_t = typename no_void_validator_tup<Ts...>::type;

////////////////
// tied class
////////////////

// check if parameter pack is only one element which is a class and has
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

class converter {
        using string_range = std::pair<const char*, const char*>;
        using split_input = std::vector<string_range>;
        constexpr static auto default_delimiter = ',';

    public:
        // parses line with given delimiter, returns a 'T' object created with
        // extracted values of type 'Ts'
        template <typename T, typename... Ts>
        T convert_struct(const char* const line,
                         const std::string& delim = "") {
                return to_struct<T>(convert<Ts...>(line, delim));
        }

        // parses line with given delimiter, returns tuple of objects with
        // extracted values of type 'Ts'
        template <typename... Ts>
        no_void_validator_tup_t<Ts...> convert(const char* const line,
                                               const std::string& delim = "") {
                input_ = split(line, delim);
                return convert<Ts...>(input_);
        }

        // parses already split line, returns 'T' object with extracted values
        template <typename T, typename... Ts>
        T convert_struct(const split_input& elems) {
                return to_struct<T>(convert<Ts...>(elems));
        }

        // parses already split line, returns either a tuple of objects with
        // parsed values (returns raw element (no tuple) if Ts is empty), or if
        // one argument is given which is a class which has a tied
        // method which returns a tuple, returns that type
        template <typename T, typename... Ts>
        no_void_validator_tup_t<T, Ts...> convert(const split_input& elems) {
                if constexpr (tied_class_v<T, Ts...>) {
                        using arg_ref_tuple =
                            typename std::result_of_t<decltype (&T::tied)(T)>;

                        using arg_tuple =
                            typename apply_trait<std::decay,
                                                 arg_ref_tuple>::type;

                        return to_struct<T>(
                            convert_impl(elems, (arg_tuple*){}));
                } else {
                        return convert_impl<T, Ts...>(elems);
                }
        }

        bool valid() const {
                return error_.empty();
        }

        const std::string& error() const {
                return error_;
        }

        // 'splits' string by given delimiter, returns vector of pairs which
        // contain the beginings and the ends of each column of the string
        const split_input& split(const char* const line,
                                 const std::string& delim = "") {
                input_.clear();
                if (line[0] == '\0') {
                        return input_;
                }

                switch (delim.size()) {
                case 0:
                        return split_impl(line, ',');
                case 1:
                        return split_impl(line, delim[0]);
                default:
                        return split_impl(line, delim, delim.size());
                };
        }

    private:
        ////////////////
        // error
        ////////////////

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

        void set_error_invalid_conversion(const string_range msg, size_t pos) {
                error_.clear();
                error_.append("invalid conversion for parameter ")
                    .append(error_sufix(msg, pos));
        }

        void set_error_validate(const char* const error, const string_range msg,
                                size_t pos) {
                error_.clear();
                error_.append(error).append(" ").append(error_sufix(msg, pos));
        }

        void set_error_number_of_colums(size_t expected_pos, size_t pos) {
                error_.append("invalid number of columns, expected: ")
                    .append(std::to_string(expected_pos))
                    .append(", got: ")
                    .append(std::to_string(pos));
        }

        ////////////////
        // convert implementation
        ////////////////

        template <typename... Ts>
        no_void_validator_tup_t<Ts...> convert_impl(const split_input& elems) {
                error_.clear();
                no_void_validator_tup_t<Ts...> ret{};
                if (sizeof...(Ts) != elems.size()) {
                        set_error_number_of_colums(sizeof...(Ts), elems.size());
                        return ret;
                }
                return extract_tuple<Ts...>(elems);
        }

        // do not know how to specialize by return type :(
        template <typename... Ts>
        no_void_validator_tup_t<std::tuple<Ts...>> convert_impl(
            const split_input& elems, const std::tuple<Ts...>*) {
                return convert_impl<Ts...>(elems);
        }

        ////////////////
        // substring
        ////////////////

        template <typename Delim>
        const split_input& split_impl(const char* const line, Delim delim,
                                      size_t delim_size = 1) {
                auto range = substring(line, delim);
                input_.push_back(range);
                while (range.second[0] != '\0') {
                        range = substring(range.second + delim_size, delim);
                        input_.push_back(range);
                }
                return input_;
        }

        bool no_match(const char* end, char delim) const {
                return *end != delim;
        }

        bool no_match(const char* end, const std::string& delim) const {
                return strncmp(end, delim.c_str(), delim.size()) != 0;
        }

        template <typename Delim>
        string_range substring(const char* const begin, Delim delim) const {
                const char* end;
                for (end = begin; *end != '\0' && no_match(end, delim); ++end)
                        ;

                return string_range{begin, end};
        }

        ////////////////
        // conversion
        ////////////////

#ifdef SS_THROW_ON_INVALID
#define SS_RETURN_ON_INVALID // nop
#else
#define SS_RETURN_ON_INVALID                                                   \
        if (!valid()) {                                                        \
                return;                                                        \
        }
#endif

        template <typename T>
        void extract_one(no_validator_t<T>& dst, const string_range msg,
                         size_t pos) {
                SS_RETURN_ON_INVALID;

                if (!extract(msg.first, msg.second, dst)) {
                        set_error_invalid_conversion(msg, pos);
                        return;
                }

                if constexpr (has_m_ss_valid_t<T>) {
                        if (T validator; !validator.ss_valid(dst)) {
                                if constexpr (has_m_error_t<T>) {
                                        set_error_validate(validator.error(),
                                                           msg, pos);
                                } else {
                                        set_error_validate("validation error",
                                                           msg, pos);
                                }
                                return;
                        }
                }
        }

        template <size_t ArgN, size_t TupN, typename... Ts>
        void extract_multiple(no_void_validator_tup_t<Ts...>& tup,
                              const split_input& elems) {
                using elem_t = std::tuple_element_t<ArgN, std::tuple<Ts...>>;

                constexpr bool not_void = !std::is_void_v<elem_t>;
                constexpr bool not_tuple =
                    count_not<std::is_void, Ts...>::size == 1;

                if constexpr (not_void) {
                        if constexpr (not_tuple) {
                                extract_one<elem_t>(tup, elems[ArgN], ArgN);
                        } else {
                                auto& el = std::get<TupN>(tup);
                                extract_one<elem_t>(el, elems[ArgN], ArgN);
                        }
                }

                if constexpr (sizeof...(Ts) > ArgN + 1) {
                        constexpr size_t NewTupN = (not_void) ? TupN + 1 : TupN;
                        extract_multiple<ArgN + 1, NewTupN, Ts...>(tup, elems);
                }
        }

        template <typename... Ts>
        no_void_validator_tup_t<Ts...> extract_tuple(const split_input& elems) {
                static_assert(!all_of<std::is_void, Ts...>::value,
                              "at least one parameter must be non void");

                no_void_validator_tup_t<Ts...> ret;
                extract_multiple<0, 0, Ts...>(ret, elems);
                return ret;
        };

        ////////////////
        // members
        ////////////////

        std::vector<string_range> input_;
        std::string error_;
};

template <>
void converter::extract_one<std::string>(std::string& dst,
        const string_range msg, size_t) {
        SS_RETURN_ON_INVALID;
        extract(msg.first, msg.second, dst);
}

#undef SS_RETURN_ON_INVALID

} /* ss */
