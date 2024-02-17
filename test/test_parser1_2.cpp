#include "test_parser1.hpp"

struct my_string {
    char* data{nullptr};

    my_string() = default;

    ~my_string() {
        delete[] data;
    }

    // make sure no object is copied
    my_string(const my_string&) = delete;
    my_string& operator=(const my_string&) = delete;

    my_string(my_string&& other) : data{other.data} {
        other.data = nullptr;
    }

    my_string& operator=(my_string&& other) {
        data = other.data;
        return *this;
    }
};

template <>
inline bool ss::extract(const char* begin, const char* end, my_string& s) {
    size_t size = end - begin;
    s.data = new char[size + 1];
    strncpy(s.data, begin, size);
    s.data[size] = '\0';
    return true;
}

struct xyz {
    my_string x;
    my_string y;
    my_string z;
    auto tied() {
        return std::tie(x, y, z);
    }
};

template <bool buffer_mode, typename... Ts>
void test_moving_of_parsed_composite_values() {
    // to compile is enough
    return;
    auto [p, _] = make_parser<buffer_mode, Ts...>("", "");
    p.template try_next<my_string, my_string, my_string>()
        .template or_else<my_string, my_string, my_string, my_string>(
            [](auto&&) {})
        .template or_else<my_string>([](auto&) {})
        .template or_else<xyz>([](auto&&) {})
        .template or_object<xyz, my_string, my_string, my_string>([](auto&&) {})
        .template or_else<std::tuple<my_string, my_string, my_string>>(
            [](auto&, auto&, auto&) {});
}

TEST_CASE("parser test the moving of parsed composite values") {
    test_moving_of_parsed_composite_values<false>();
    test_moving_of_parsed_composite_values<false, ss::string_error>();
    test_moving_of_parsed_composite_values<true>();
    test_moving_of_parsed_composite_values<true, ss::string_error>();
}

TEST_CASE("parser test error mode") {
    unique_file_name f{"test_parser"};
    {
        std::ofstream out{f.name};
        out << "junk" << std::endl;
        out << "junk" << std::endl;
    }

    {
        auto [p, _] = make_parser<false, ss::string_error>(f.name, ",");

        REQUIRE_FALSE(p.eof());
        p.get_next<int>();
        CHECK_FALSE(p.valid());
        CHECK_FALSE(p.error_msg().empty());
    }

    {
        auto [p, _] = make_parser<true, ss::string_error>(f.name, ",");

        REQUIRE_FALSE(p.eof());
        p.get_next<int>();
        CHECK_FALSE(p.valid());
        CHECK_FALSE(p.error_msg().empty());
    }
}

TEST_CASE("parser throw on error mode") {
    unique_file_name f{"test_parser"};
    {
        std::ofstream out{f.name};
        out << "junk" << std::endl;
        out << "junk" << std::endl;
    }

    {
        auto [p, _] = make_parser<false, ss::throw_on_error>(f.name, ",");

        REQUIRE_FALSE(p.eof());
        try {
            p.get_next<int>();
            FAIL("Expected exception...");
        } catch (const std::exception& e) {
            CHECK_FALSE(std::string{e.what()}.empty());
        }
    }

    {
        auto [p, _] = make_parser<true, ss::throw_on_error>(f.name, ",");

        REQUIRE_FALSE(p.eof());
        try {
            p.get_next<int>();
            FAIL("Expected exception...");
        } catch (const std::exception& e) {
            CHECK_FALSE(std::string{e.what()}.empty());
        }
    }
}

static inline std::string no_quote(const std::string& s) {
    if (!s.empty() && s[0] == '"') {
        return {std::next(begin(s)), std::prev(end(s))};
    }
    return s;
}

template <bool buffer_mode, typename... Ts>
void test_quote_multiline() {
    unique_file_name f{"test_parser"};
    std::vector<X> data = {{1, 2, "\"x\r\nx\nx\""},
                           {3, 4, "\"y\ny\r\ny\""},
                           {5, 6, "\"z\nz\""},
                           {7, 8, "\"u\"\"\""},
                           {9, 10, "v"},
                           {11, 12, "\"w\n\""}};
    for (auto& [_, __, s] : data) {
        update_if_crlf(s);
    }

    make_and_write(f.name, data);
    for (auto& [_, __, s] : data) {
        s = no_quote(s);
        if (s[0] == 'u') {
            s = "u\"";
        }
    }

    auto [p, _] =
        make_parser<buffer_mode, ss::multiline, ss::quote<'"'>, Ts...>(f.name,
                                                                       ",");

    std::vector<X> i;

    while (!p.eof()) {
        auto a = p.template get_next<int, double, std::string>();
        i.emplace_back(ss::to_object<X>(a));
    }

    for (auto& [_, __, s] : i) {
        update_if_crlf(s);
    }
    CHECK_EQ(i, data);

    auto [p_no_multiline, __] =
        make_parser<buffer_mode, ss::quote<'"'>, Ts...>(f.name, ",");
    while (!p.eof()) {
        auto command = [&p_no_multiline = p_no_multiline] {
            p_no_multiline.template get_next<int, double, std::string>();
        };
        expect_error_on_command(p_no_multiline, command);
    }
}

TEST_CASE("parser test csv on multiple lines with quotes") {
    test_quote_multiline<false>();
    test_quote_multiline<false, ss::string_error>();
    test_quote_multiline<false, ss::throw_on_error>();
    test_quote_multiline<true>();
    test_quote_multiline<true, ss::string_error>();
    test_quote_multiline<true, ss::throw_on_error>();
}

static inline std::string no_escape(std::string& s) {
    s.erase(std::remove(begin(s), end(s), '\\'), end(s));
    return s;
}

template <bool buffer_mode, typename... Ts>
void test_escape_multiline() {
    unique_file_name f{"test_parser"};
    std::vector<X> data = {{1, 2, "x\\\nx\\\r\nx"},
                           {5, 6, "z\\\nz\\\nz"},
                           {7, 8, "u"},
                           {3, 4, "y\\\ny\\\ny"},
                           {9, 10, "v\\\\"},
                           {11, 12, "w\\\n"}};
    for (auto& [_, __, s] : data) {
        update_if_crlf(s);
    }

    make_and_write(f.name, data);
    for (auto& [_, __, s] : data) {
        s = no_escape(s);
        if (s == "v") {
            s = "v\\";
        }
    }

    auto [p, _] =
        make_parser<buffer_mode, ss::multiline, ss::escape<'\\'>, Ts...>(f.name,
                                                                         ",");
    std::vector<X> i;

    while (!p.eof()) {
        auto a = p.template get_next<int, double, std::string>();
        i.emplace_back(ss::to_object<X>(a));
    }

    for (auto& [_, __, s] : i) {
        update_if_crlf(s);
    }
    CHECK_EQ(i, data);

    auto [p_no_multiline, __] =
        make_parser<buffer_mode, ss::escape<'\\'>, Ts...>(f.name, ",");
    while (!p.eof()) {
        auto command = [&p_no_multiline = p_no_multiline] {
            auto a =
                p_no_multiline.template get_next<int, double, std::string>();
        };
        expect_error_on_command(p_no_multiline, command);
    }
}

TEST_CASE("parser test csv on multiple lines with escapes") {
    test_escape_multiline<false>();
    test_escape_multiline<false, ss::string_error>();
    test_escape_multiline<false, ss::throw_on_error>();
    test_escape_multiline<true>();
    test_escape_multiline<true, ss::string_error>();
    test_escape_multiline<true, ss::throw_on_error>();
}

template <bool buffer_mode, typename... Ts>
void test_quote_escape_multiline() {
    unique_file_name f{"test_parser"};
    {
        std::ofstream out{f.name};
        out << "1,2,\"just\\\n\nstrings\"" << std::endl;
#ifndef _WIN32
        out << "3,4,\"just\r\nsome\\\r\n\n\\\nstrings\"" << std::endl;
        out << "5,6,\"just\\\n\\\r\n\r\n\nstrings" << std::endl;
#else
        out << "3,4,\"just\nsome\\\n\n\\\nstrings\"" << std::endl;
        out << "5,6,\"just\\\n\\\n\n\nstrings" << std::endl;
#endif
        out << "7,8,\"just strings\"" << std::endl;
        out << "9,10,just strings" << std::endl;
    }
    size_t bad_lines = 1;
    auto num_errors = 0;

    auto [p, _] = make_parser<buffer_mode, ss::multiline, ss::escape<'\\'>,
                              ss::quote<'"'>, Ts...>(f.name);
    std::vector<X> i;

    while (!p.eof()) {
        try {
            auto a = p.template get_next<int, double, std::string>();
            if (p.valid()) {
                i.emplace_back(ss::to_object<X>(a));
            } else {
                ++num_errors;
            }
        } catch (const std::exception& e) {
            ++num_errors;
        }
    }

    CHECK(bad_lines == num_errors);

    std::vector<X> data = {{1, 2, "just\n\nstrings"},
#ifndef _WIN32
                           {3, 4, "just\r\nsome\r\n\n\nstrings"},
#else
                           {3, 4, "just\nsome\n\n\nstrings"},
#endif
                           {9, 10, "just strings"}};

    for (auto& [_, __, s] : i) {
        update_if_crlf(s);
    }
    CHECK_EQ(i, data);
}

TEST_CASE("parser test csv on multiple lines with quotes and escapes") {
    test_quote_escape_multiline<false>();
    test_quote_escape_multiline<false, ss::string_error>();
    test_quote_escape_multiline<false, ss::throw_on_error>();
    test_quote_escape_multiline<true>();
    test_quote_escape_multiline<true, ss::string_error>();
    test_quote_escape_multiline<true, ss::throw_on_error>();
}
