#pragma once
#include <exception>
#include <string>

namespace ss {

////////////////
// exception
////////////////

class exception : public std::exception {
    std::string msg_;

public:
    exception(std::string msg) : msg_{std::move(msg)} {
    }

    [[nodiscard]] char const* what() const noexcept override {
        return msg_.c_str();
    }
};

} /* namespace ss */
