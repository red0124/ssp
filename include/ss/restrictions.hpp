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
// greater than or equal to
// greater than
// less than
// less than or equal to
////////////////

template <typename T, auto N>
struct gt {
    bool ss_valid(const T& value) const {
        return value > N;
    }
};

template <typename T, auto N>
struct gte {
    bool ss_valid(const T& value) const {
        return value >= N;
    }
};

template <typename T, auto N>
struct lt {
    bool ss_valid(const T& value) const {
        return value < N;
    }
};

template <typename T, auto N>
struct lte {
    bool ss_valid(const T& value) const {
        return value <= N;
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
};

////////////////
// out of range
////////////////

template <typename T, auto Min, auto Max>
struct oor {
    bool ss_valid(const T& value) const {
        return value < Min || value > Max;
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

} /* namespace ss */
