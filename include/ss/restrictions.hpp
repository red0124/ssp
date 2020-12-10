#pragma once

namespace ss {

////////////////
// all except
////////////////

template <typename T, auto... Values>
struct ax {
    private:
        template <auto X, auto... Xs>
        bool ss_valid_impl(const T& x) const {
                if constexpr (sizeof...(Xs) != 0) {
                        return x != X && ss_valid_impl<Xs...>(x);
                }
                return x != X;
        }

    public:
        bool ss_valid(const T& value) const {
                return ss_valid_impl<Values...>(value);
        }

        const char* error() const {
                return "value excluded";
        }
};

////////////////
// none except
////////////////

template <typename T, auto... Values>
struct nx {
    private:
        template <auto X, auto... Xs>
        bool ss_valid_impl(const T& x) const {
                if constexpr (sizeof...(Xs) != 0) {
                        return x == X || ss_valid_impl<Xs...>(x);
                }
                return x == X;
        }

    public:
        bool ss_valid(const T& value) const {
                return ss_valid_impl<Values...>(value);
        }

        const char* error() const {
                return "value excluded";
        }
};

////////////////
// in range
////////////////

template <typename T, auto Min, auto Max>
struct ir {
        bool ss_valid(const T& value) const {
                return value >= Min && value <= Max;
        }

        const char* error() const {
                return "out of range";
        }
};

////////////////
// out of range
////////////////

template <typename T, auto Min, auto Max>
struct oor {
        bool ss_valid(const T& value) const {
                return value < Min || value > Max;
        }

        const char* error() const {
                return "in restricted range";
        }
};

////////////////
// non empty
////////////////

template <typename T>
struct ne {
        bool ss_valid(const T& value) const {
                return !value.empty();
        }

        const char* error() const {
                return "empty field";
        }
};

} /* ss */
