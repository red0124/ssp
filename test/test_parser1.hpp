#pragma once

#include "test_helpers.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ss/parser.hpp>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace {
[[maybe_unused]] void replace_all(std::string& s, const std::string& from,
                                  const std::string& to) {
    if (from.empty()) return;
    size_t start_pos = 0;
    while ((start_pos = s.find(from, start_pos)) != std::string::npos) {
        s.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

template <typename... Ts>
void expect_error_on_command(ss::parser<Ts...>& p,
                             const std::function<void()> command) {
    if (ss::setup<Ts...>::throw_on_error) {
        try {
            command();
        } catch (const std::exception& e) {
            CHECK_FALSE(std::string{e.what()}.empty());
        }
    } else {
        command();
        CHECK(!p.valid());
        if constexpr (ss::setup<Ts...>::string_error) {
            CHECK_FALSE(p.error_msg().empty());
        }
    }
}

[[maybe_unused]] void update_if_crlf(std::string& s) {
#ifdef _WIN32
    replace_all(s, "\r\n", "\n");
#else
    (void)(s);
#endif
}

struct X {
    constexpr static auto delim = ",";
    constexpr static auto empty = "_EMPTY_";
    int i;
    double d;
    std::string s;

    std::string to_string() const {
        if (s == empty) {
            return "";
        }

        return std::to_string(i)
            .append(delim)
            .append(std::to_string(d))
            .append(delim)
            .append(s);
    }
    auto tied() const {
        return std::tie(i, d, s);
    }
};

template <typename T>
std::enable_if_t<ss::has_m_tied_t<T>, bool> operator==(const T& lhs,
                                                       const T& rhs) {
    return lhs.tied() == rhs.tied();
}

template <typename T>
static void make_and_write(const std::string& file_name,
                           const std::vector<T>& data,
                           const std::vector<std::string>& header = {},
                           bool new_line_eof = true) {
    std::ofstream out{file_name};

#ifdef _WIN32
    std::vector<const char*> new_lines = {"\n"};
#else
    std::vector<const char*> new_lines = {"\n", "\r\n"};
#endif

    for (const auto& i : header) {
        if (&i != &header.front()) {
            out << T::delim;
        }
        out << i;
    }

    if (!header.empty()) {
        out << new_lines.front();
    }

    for (size_t i = 0; i < data.size(); ++i) {
        out << data[i].to_string();
        if (new_line_eof || i + 1 < data.size()) {
            out << new_lines[i % new_lines.size()];
        }
    }
}

} /* namespace */
