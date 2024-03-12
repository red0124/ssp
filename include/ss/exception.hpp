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
    exception(const std::string& msg): msg_{msg} {
    }

    virtual char const* what() const noexcept {
        return msg_.c_str();
    }
};

} /* namespace ss */
