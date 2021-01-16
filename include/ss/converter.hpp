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
struct no_validator_tup {
    using type = typename apply_trait<no_validator, std::tuple<Ts...>>::type;
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
struct no_void_validator_tup<std::tuple<Ts...>> {
    using type = no_validator_tup_t<no_void_tup_t<Ts...>>;
};

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

// the error can be set inside a string, or a bool
enum class error_mode { error_string, error_bool };

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
template <char... Cs>
struct matcher {
private:
    template <char X, char... Xs>
    static bool match_impl(char c) {
        if constexpr (sizeof...(Xs) != 0) {
            return (c == X) || match_impl<Xs...>(c);
        }
        return (c == X);
    }

public:
    static bool match(char c) {
        return match_impl<Cs...>(c);
    }
    constexpr static bool enabled = true;
};

template <>
class matcher<'\0'> {
public:
    constexpr static bool enabled = false;
    static bool match(char c) = delete;
};

////////////////
// is instance of
////////////////

template <typename T, template <char...> class Template>
struct is_instance_of_char {
    constexpr static bool value = false;
};

template <char... Ts, template <char...> class Template>
struct is_instance_of_char<Template<Ts...>, Template> {
    constexpr static bool value = true;
};

///////////////////////////////////////////////////

template <char... Cs>
struct quote : matcher<Cs...> {};

template <char... Cs>
struct trim : matcher<Cs...> {};

template <char... Cs>
struct escape : matcher<Cs...> {};

/////////////////////////////////////////////////
// -> type traits
template <bool B, typename T, typename U>
struct if_then_else;

template <typename T, typename U>
struct if_then_else<true, T, U> {
    using type = T;
};

template <typename T, typename U>
struct if_then_else<false, T, U> {
    using type = U;
};

//////////////////////////////////////////////
template <template <char...> class Matcher, typename... Ts>
struct get_matcher;

template <template <char...> class Matcher, typename T, typename... Ts>
struct get_matcher<Matcher, T, Ts...> {
    using type =
        typename if_then_else<is_instance_of_char<T, Matcher>::value, T,
                              typename get_matcher<Matcher, Ts...>::type>::type;
};

template <template <char...> class Matcher>
struct get_matcher<Matcher> {
    using type = Matcher<'\0'>;
};

///////////////////////////////////////////////
// TODO add restriction
template <typename... Ts>
struct setup {
    using quote = typename get_matcher<quote, Ts...>::type;
    using trim = typename get_matcher<trim, Ts...>::type;
    using escape = typename get_matcher<escape, Ts...>::type;
};

template <typename... Ts>
struct setup<setup<Ts...>> : setup<Ts...> {};

/////////////////////////////////////////////////////////////////////////////

enum class State { finished, begin, reading, quoting };
using range = std::pair<const char*, const char*>;

using string_range = std::pair<const char*, const char*>;
using split_input = std::vector<string_range>;

template <typename... Ts>
class splitter {
    using Setup = setup<Ts...>;
    using quote = typename Setup::quote;
    using trim = typename Setup::trim;
    using escape = typename Setup::escape;

    bool match(const char* end_i, char delim) {
        return *end_i == delim;
    };

    bool match(const char* end_i, const std::string& delim) {
        return strncmp(end_i, delim.c_str(), delim.size()) == 0;
    };

    size_t delimiter_size(char) {
        return 1;
    }
    size_t delimiter_size(const std::string& delim) {
        return delim.size();
    }

    void trim_if_enabled(char*& curr) {
        if constexpr (trim::enabled) {
            while (trim::match(*curr)) {
                ++curr;
            }
        }
    }

    void shift_if_escaped(char*& curr_i) {
        if constexpr (escape::enabled) {
            if (escape::match(*curr_i)) {
                *curr = end[1];
                ++end;
            }
        }
    }

    void shift() {
        *curr = *end;
        ++end;
        ++curr;
    }

    void shift(size_t n) {
        memcpy(curr, end, n);
        end += n;
        curr += n;
    }

    template <typename Delim>
    std::tuple<size_t, bool> match_delimiter(char* begin, const Delim& delim) {
        char* end_i = begin;

        trim_if_enabled(end_i);

        // just spacing
        if (*end_i == '\0') {
            return {0, false};
        }

        // not a delimiter
        if (!match(end_i, delim)) {
            shift_if_escaped(end_i);
            return {1 + end_i - begin, false};
        }

        end_i += delimiter_size(delim);
        trim_if_enabled(end_i);

        // delimiter
        return {end_i - begin, true};
    }

public:
    bool valid() {
        return error_.empty();
    }

    split_input& split(char* new_line, const std::string& d = ",") {
        line = new_line;
        output_.clear();
        switch (d.size()) {
        case 0:
            // set error
            return output_;
        case 1:
            return split_impl(d[0]);
        default:
            return split_impl(d);
        }
    }

    template <typename Delim>
    std::vector<range>& split_impl(const Delim& delim) {
        state = State::begin;
        begin = line;

        trim_if_enabled(begin);

        while (state != State::finished) {
            curr = end = begin;
            switch (state) {
            case (State::begin):
                state_begin();
                break;
            case (State::reading):
                state_reading(delim);
                break;
            case (State::quoting):
                state_quoting(delim);
                break;
            default:
                break;
            };
        }

        return output_;
    }

    void state_begin() {
        if constexpr (quote::enabled) {
            if (quote::match(*begin)) {
                ++begin;
                state = State::quoting;
                return;
            }
        }
        state = State::reading;
    }

    template <typename Delim>
    void state_reading(const Delim& delim) {
        while (true) {
            auto [width, valid] = match_delimiter(end, delim);

            // not a delimiter
            if (!valid) {
                if (width == 0) {
                    // eol
                    output_.emplace_back(begin, curr);
                    state = State::finished;
                    break;
                } else {
                    shift(width);
                    continue;
                }
            }

            // found delimiter
            push_and_start_next(width);
            break;
        }
    }

    template <typename Delim>
    void state_quoting(const Delim& delim) {
        if constexpr (quote::enabled) {
            while (true) {
                if (quote::match(*end)) {
                    // double quote
                    // eg: ...,"hel""lo,... -> hel"lo
                    if (quote::match(end[1])) {
                        ++end;
                        shift();
                        continue;
                    }

                    auto [width, valid] = match_delimiter(end + 1, delim);

                    // not a delimiter
                    if (!valid) {
                        if (width == 0) {
                            // eol
                            // eg: ...,"hello"   \0 -> hello
                            // eg no trim: ...,"hello"\0 -> hello
                            output_.emplace_back(begin, curr);
                        } else {
                            // missmatched quote
                            // eg: ...,"hel"lo,... -> error
                        }
                        state = State::finished;
                        break;
                    }

                    // delimiter
                    push_and_start_next(width + 1);
                    break;
                }

                if constexpr (escape::enabled) {
                    if (escape::match(*end)) {
                        ++end;
                        shift();
                        continue;
                    }
                }

                // unterminated error
                // eg: ..."hell\0 -> quote not terminated
                if (*end == '\0') {
                    *curr = '\0';
                    state = State::finished;
                    break;
                }
                shift();
            }
        } else {
            // set error impossible scenario
            state = State::finished;
        }
    }

    void push_and_start_next(size_t n) {
        output_.emplace_back(begin, curr);
        begin = end + n;
        state = State::begin;
    }

private:
    std::vector<range> output_;
    std::string error_ = "";
    State state;
    char* curr;
    char* end;
    char* begin;
    char* line;
};

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

////////////////
// converter
////////////////

template <typename... Matchers>
class converter {
    constexpr static auto default_delimiter = ",";

public:
    // parses line with given delimiter, returns a 'T' object created with
    // extracted values of type 'Ts'
    template <typename T, typename... Ts>
    T convert_object(char* line, const std::string& delim = default_delimiter) {
        return to_object<T>(convert<Ts...>(line, delim));
    }

    // parses line with given delimiter, returns tuple of objects with
    // extracted values of type 'Ts'
    template <typename... Ts>
    no_void_validator_tup_t<Ts...> convert(
        char* line, const std::string& delim = default_delimiter) {
        input_ = split(line, delim);
        return convert<Ts...>(input_);
    }

    // parses already split line, returns 'T' object with extracted values
    template <typename T, typename... Ts>
    T convert_object(const split_input& elems) {
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
    no_void_validator_tup_t<T, Ts...> convert(const split_input& elems) {
        if constexpr (sizeof...(Ts) == 0 &&
                      is_instance_of<T, std::tuple>::value) {
            return convert_impl(elems, (T*){});

        } else if constexpr (tied_class_v<T, Ts...>) {
            using arg_ref_tuple =
                typename std::result_of_t<decltype (&T::tied)(T)>;

            using arg_tuple =
                typename apply_trait<std::decay, arg_ref_tuple>::type;

            return to_object<T>(convert_impl(elems, (arg_tuple*){}));
        } else {
            return convert_impl<T, Ts...>(elems);
        }
    }

    // same as above, but uses cached split line
    template <typename T, typename... Ts>
    no_void_validator_tup_t<T, Ts...> convert() {
        return convert<T, Ts...>(input_);
    }

    bool valid() const {
        return (error_mode_ == error_mode::error_string) ? string_error_.empty()
                                                         : bool_error_ == false;
    }

    const std::string& error_msg() const {
        return string_error_;
    }

    void set_error_mode(error_mode mode) {
        error_mode_ = mode;
    }

    // 'splits' string by given delimiter, returns vector of pairs which
    // contain the beginnings and the ends of each column of the string
    const split_input& split(char* line,
                             const std::string& delim = default_delimiter) {
        input_.clear();
        if (line[0] == '\0') {
            return input_;
        }

        input_ = splitter_.split(line, delim);
        return input_;
    }

private:
    ////////////////
    // error
    ////////////////

    void clear_error() {
        string_error_.clear();
        bool_error_ = false;
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

    void set_error_invalid_quotation() {
        if (error_mode_ == error_mode::error_string) {
            string_error_.clear();
            string_error_.append("invalid quotation");
        } else {
            bool_error_ = true;
        }
    }

    void set_error_unterminated_quote() {
        if (error_mode_ == error_mode::error_string) {
            string_error_.clear();
            string_error_.append("unterminated quote");
        } else {
            bool_error_ = true;
        }
    }

    void set_error_invalid_conversion(const string_range msg, size_t pos) {
        if (error_mode_ == error_mode::error_string) {
            string_error_.clear();
            string_error_.append("invalid conversion for parameter ")
                .append(error_sufix(msg, pos));
        } else {
            bool_error_ = true;
        }
    }

    void set_error_validate(const char* const error, const string_range msg,
                            size_t pos) {
        if (error_mode_ == error_mode::error_string) {
            string_error_.clear();
            string_error_.append(error).append(" ").append(
                error_sufix(msg, pos));
        } else {
            bool_error_ = true;
        }
    }

    void set_error_number_of_colums(size_t expected_pos, size_t pos) {
        if (error_mode_ == error_mode::error_string) {
            string_error_.clear();
            string_error_.append("invalid number of columns, expected: ")
                .append(std::to_string(expected_pos))
                .append(", got: ")
                .append(std::to_string(pos));
        } else {
            bool_error_ = true;
        }
    }

    ////////////////
    // convert implementation
    ////////////////

    template <typename... Ts>
    no_void_validator_tup_t<Ts...> convert_impl(const split_input& elems) {
        clear_error();
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
                          const split_input& elems) {
        using elem_t = std::tuple_element_t<ArgN, std::tuple<Ts...>>;

        constexpr bool not_void = !std::is_void_v<elem_t>;
        constexpr bool one_element = count_not<std::is_void, Ts...>::size == 1;

        if constexpr (not_void) {
            if constexpr (one_element) {
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
    std::string string_error_;
    bool bool_error_;
    enum error_mode error_mode_ { error_mode::error_bool };
    splitter<Matchers...> splitter_;
};

} /* ss */
