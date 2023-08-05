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

// TODO remove
#include <iostream>

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

void update_if_crlf(std::string& s) {
#ifdef _WIN32
    replace_all(s, "\r\n", "\n");
#else
    (void)(s);
#endif
}

struct X {
    constexpr static auto delim = ",";
    constexpr static auto make_empty = "_EMPTY_";
    int i;
    double d;
    std::string s;

    std::string to_string() const {
        if (s == make_empty) {
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
                           const std::vector<std::string>& header = {}) {
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
        out << data[i].to_string() << new_lines[i % new_lines.size()];
    }
}
} /* namespace */

TEST_CASE("test file not found") {
    unique_file_name f{"test_parser"};

    {
        ss::parser p{f.name, ","};
        CHECK_FALSE(p.valid());
    }

    {
        ss::parser<ss::string_error> p{f.name, ","};
        CHECK_FALSE(p.valid());
    }

    try {
        ss::parser<ss::throw_on_error> p{f.name, ","};
        FAIL("Expected exception...");
    } catch (const std::exception& e) {
        CHECK_FALSE(std::string{e.what()}.empty());
    }
}

template <typename... Ts>
void test_various_cases() {
    unique_file_name f{"test_parser"};
    std::vector<X> data = {{1, 2, "x"}, {3, 4, "y"},  {5, 6, "z"},
                           {7, 8, "u"}, {9, 10, "v"}, {11, 12, "w"}};
    make_and_write(f.name, data);
    {
        ss::parser<Ts...> p{f.name, ","};
        ss::parser p0{std::move(p)};
        p = std::move(p0);
        std::vector<X> i;

        ss::parser<ss::string_error> p2{f.name, ","};
        std::vector<X> i2;

        auto move_rotate = [&] {
            auto p1 = std::move(p);
            p0 = std::move(p1);
            p = std::move(p0);
        };

        while (!p.eof()) {
            move_rotate();
            auto a = p.template get_next<int, double, std::string>();
            i.emplace_back(ss::to_object<X>(a));
        }

        for (const auto& a : p2.iterate<int, double, std::string>()) {
            i2.emplace_back(ss::to_object<X>(a));
        }

        CHECK_EQ(i, data);
        CHECK_EQ(i2, data);
    }

    {
        ss::parser p{f.name, ","};
        std::vector<X> i;

        ss::parser p2{f.name, ","};
        std::vector<X> i2;

        ss::parser p3{f.name, ","};
        std::vector<X> i3;

        std::vector<X> expected = {std::begin(data) + 1, std::end(data)};
        using tup = std::tuple<int, double, std::string>;

        p.ignore_next();
        while (!p.eof()) {
            auto a = p.get_next<tup>();
            i.emplace_back(ss::to_object<X>(a));
        }

        p2.ignore_next();
        for (const auto& a : p2.iterate<tup>()) {
            i2.emplace_back(ss::to_object<X>(a));
        }

        p3.ignore_next();
        for (auto it = p3.iterate<tup>().begin(); it != p3.iterate<tup>().end();
             ++it) {
            i3.emplace_back(ss::to_object<X>(*it));
        }

        CHECK_EQ(i, expected);
        CHECK_EQ(i2, expected);
        CHECK_EQ(i3, expected);
    }

    {
        ss::parser p{f.name, ","};
        std::vector<X> i;
        ss::parser p2{f.name, ","};
        std::vector<X> i2;

        while (!p.eof()) {
            i.push_back(p.get_object<X, int, double, std::string>());
        }

        for (auto&& a : p2.iterate_object<X, int, double, std::string>()) {
            i2.push_back(std::move(a));
        }

        CHECK_EQ(i, data);
        CHECK_EQ(i2, data);
    }

    {
        ss::parser p{f.name, ","};
        std::vector<X> i;

        for (auto&& a : p.iterate_object<X, int, double, std::string>()) {
            i.push_back(std::move(a));
        }

        CHECK_EQ(i, data);
    }

    {
        ss::parser p{f.name, ","};
        std::vector<X> i;

        ss::parser p2{f.name, ","};
        std::vector<X> i2;

        using tup = std::tuple<int, double, std::string>;
        while (!p.eof()) {
            i.push_back(p.get_object<X, tup>());
        }

        for (auto it = p2.iterate_object<X, tup>().begin();
             it != p2.iterate_object<X, tup>().end(); it++) {
            i2.push_back({it->i, it->d, it->s});
        }

        CHECK_EQ(i, data);
        CHECK_EQ(i2, data);
    }

    {
        ss::parser p{f.name, ","};
        std::vector<X> i;

        using tup = std::tuple<int, double, std::string>;
        for (auto&& a : p.iterate_object<X, tup>()) {
            i.push_back(std::move(a));
        }

        CHECK_EQ(i, data);
    }

    {
        ss::parser p{f.name, ","};
        std::vector<X> i;

        while (!p.eof()) {
            i.push_back(p.get_next<X>());
        }

        CHECK_EQ(i, data);
    }

    {
        ss::parser p{f.name, ","};
        std::vector<X> i;

        for (auto&& a : p.iterate<X>()) {
            i.push_back(std::move(a));
        }

        CHECK_EQ(i, data);
    }

    {
        constexpr int excluded = 3;
        ss::parser p{f.name, ","};
        std::vector<X> i;

        ss::parser p2{f.name, ","};
        std::vector<X> i2;

        while (!p.eof()) {
            auto a =
                p.get_object<X, ss::ax<int, excluded>, double, std::string>();
            if (p.valid()) {
                i.push_back(a);
            }
        }

        for (auto&& a : p2.iterate_object<X, ss::ax<int, excluded>, double,
                                          std::string>()) {
            if (p2.valid()) {
                i2.push_back(std::move(a));
            }
        }

        std::vector<X> expected;
        for (auto& x : data) {
            if (x.i != excluded) {
                expected.push_back(x);
            }
        }

        std::copy_if(data.begin(), data.end(), expected.begin(),
                     [&](const X& x) { return x.i != excluded; });
        CHECK_EQ(i, expected);
        CHECK_EQ(i2, expected);
    }

    {
        ss::parser p{f.name, ","};
        std::vector<X> i;

        ss::parser p2{f.name, ","};
        std::vector<X> i2;

        while (!p.eof()) {
            auto a = p.get_object<X, ss::nx<int, 3>, double, std::string>();
            if (p.valid()) {
                i.push_back(a);
            }
        }

        for (auto&& a :
             p2.iterate_object<X, ss::nx<int, 3>, double, std::string>()) {
            if (p2.valid()) {
                i2.push_back(std::move(a));
            }
        }

        std::vector<X> expected = {{3, 4, "y"}};
        CHECK_EQ(i, expected);
        CHECK_EQ(i2, expected);
    }

    {
        unique_file_name empty_f{"test_parser"};
        std::vector<X> empty_data = {};

        make_and_write(empty_f.name, empty_data);

        ss::parser p{empty_f.name, ","};
        std::vector<X> i;

        ss::parser p2{empty_f.name, ","};
        std::vector<X> i2;

        while (!p.eof()) {
            i.push_back(p.get_next<X>());
        }

        for (auto&& a : p2.iterate<X>()) {
            i2.push_back(std::move(a));
        }

        CHECK(i.empty());
        CHECK(i2.empty());
    }
}

TEST_CASE("parser test various cases") {
    test_various_cases();
    test_various_cases<ss::string_error>();
    test_various_cases<ss::throw_on_error>();
}

using test_tuple = std::tuple<double, char, double>;
struct test_struct {
    int i;
    double d;
    char c;
    auto tied() {
        return std::tie(i, d, c);
    }
};

static inline void expect_test_struct(const test_struct&) {
}

template <typename... Ts>
void test_composite_conversion() {
    unique_file_name f{"test_parser"};
    {
        std::ofstream out{f.name};
        for (auto& i :
             {"10,a,11.1", "10,20,11.1", "junk", "10,11.1", "1,11.1,a", "junk",
              "10,junk", "11,junk", "10,11.1,c", "10,20", "10,22.2,f"}) {
            out << i << std::endl;
        }
    }

    ss::parser<Ts...> p{f.name, ","};
    auto fail = [] { FAIL(""); };
    auto expect_error = [](auto error) { CHECK(!error.empty()); };
    auto ignore_error = [] {};

    REQUIRE(p.valid());
    REQUIRE_FALSE(p.eof());

    {
        constexpr static auto expectedData = std::tuple{10, 'a', 11.1};

        auto [d1, d2, d3, d4] =
            p.template try_next<int, int, double>(fail)
                .template or_else<test_struct>(fail)
                .template or_else<int, char, double>(
                    [&](auto&& data) { CHECK_EQ(data, expectedData); })
                .on_error(fail)
                .template or_else<test_tuple>(fail)
                .values();

        REQUIRE(p.valid());
        REQUIRE_FALSE(d1);
        REQUIRE_FALSE(d2);
        REQUIRE(d3);
        REQUIRE_FALSE(d4);
        CHECK_EQ(*d3, expectedData);
    }

    {
        REQUIRE(!p.eof());
        constexpr static auto expectedData = std::tuple{10, 20, 11.1};

        auto [d1, d2, d3, d4] =
            p.template try_next<int, int, double>(
                 [&](auto& i1, auto i2, double d) {
                     CHECK_EQ(std::tie(i1, i2, d), expectedData);
                 })
                .on_error(fail)
                .template or_object<test_struct, int, double, char>(fail)
                .on_error(fail)
                .template or_else<test_tuple>(fail)
                .on_error(fail)
                .template or_else<int, char, double>(fail)
                .values();

        REQUIRE(p.valid());
        REQUIRE(d1);
        REQUIRE_FALSE(d2);
        REQUIRE_FALSE(d3);
        REQUIRE_FALSE(d4);
        CHECK_EQ(*d1, expectedData);
    }

    {
        REQUIRE(!p.eof());

        auto [d1, d2, d3, d4, d5] =
            p.template try_object<test_struct, int, double, char>(fail)
                .on_error(expect_error)
                .template or_else<int, char, char>(fail)
                .template or_else<test_struct>(fail)
                .template or_else<test_tuple>(fail)
                .template or_else<int, char, double>(fail)
                .values();

        REQUIRE_FALSE(p.valid());
        REQUIRE_FALSE(d1);
        REQUIRE_FALSE(d2);
        REQUIRE_FALSE(d3);
        REQUIRE_FALSE(d4);
        REQUIRE_FALSE(d5);
    }

    {
        REQUIRE(!p.eof());

        auto [d1, d2] =
            p.template try_next<int, double>([](auto& i, auto& d) {
                 REQUIRE_EQ(std::tie(i, d), std::tuple{10, 11.1});
             })
                .template or_else<int, double>([](auto&, auto&) { FAIL(""); })
                .values();

        REQUIRE(p.valid());
        REQUIRE(d1);
        REQUIRE_FALSE(d2);
    }

    {
        REQUIRE(!p.eof());

        auto [d1, d2] =
            p.template try_next<int, double>([](auto&, auto&) { FAIL(""); })
                .template or_else<test_struct>(expect_test_struct)
                .values();

        REQUIRE(p.valid());
        REQUIRE_FALSE(d1);
        REQUIRE(d2);
        CHECK_EQ(d2->tied(), std::tuple{1, 11.1, 'a'});
    }

    {
        REQUIRE(!p.eof());

        auto [d1, d2, d3, d4, d5] =
            p.template try_next<int, int, double>(fail)
                .template or_object<test_struct, int, double, char>()
                .template or_else<test_struct>(expect_test_struct)
                .template or_else<test_tuple>(fail)
                .template or_else<std::tuple<int, double>>(fail)
                .on_error(ignore_error)
                .on_error(expect_error)
                .values();

        REQUIRE_FALSE(p.valid());
        REQUIRE_FALSE(d1);
        REQUIRE_FALSE(d2);
        REQUIRE_FALSE(d3);
        REQUIRE_FALSE(d4);
        REQUIRE_FALSE(d5);
    }

    {
        REQUIRE(!p.eof());

        auto [d1, d2] =
            p.template try_next<int, std::optional<int>>()
                .on_error(ignore_error)
                .on_error(fail)
                .template or_else<std::tuple<int, std::string>>(fail)
                .on_error(ignore_error)
                .on_error(fail)
                .on_error(ignore_error)
                .values();

        REQUIRE(p.valid());
        REQUIRE(d1);
        REQUIRE_FALSE(d2);
        CHECK_EQ(*d1, std::tuple{10, std::nullopt});
    }

    {
        REQUIRE_FALSE(p.eof());

        auto [d1, d2] =
            p.template try_next<int, std::variant<int, std::string>>()
                .on_error(fail)
                .template or_else<std::tuple<int, std::string>>(fail)
                .on_error(fail)
                .values();

        REQUIRE(p.valid());
        REQUIRE(d1);
        REQUIRE_FALSE(d2);
        CHECK_EQ(*d1, std::tuple{11, std::variant<int, std::string>{"junk"}});
    }

    {
        REQUIRE(!p.eof());

        auto [d1, d2] = p.template try_object<test_struct, int, double, char>()
                            .template or_else<int>(fail)
                            .values();
        REQUIRE(p.valid());
        REQUIRE(d1);
        REQUIRE_FALSE(d2);
        CHECK_EQ(d1->tied(), std::tuple{10, 11.1, 'c'});
    }

    {
        REQUIRE_FALSE(p.eof());

        auto [d1, d2, d3, d4] =
            p.template try_next<int, int>([] { return false; })
                .template or_else<int, double>([](auto&) { return false; })
                .template or_else<int, int>()
                .template or_else<int, int>(fail)
                .values();

        REQUIRE(p.valid());
        REQUIRE_FALSE(d1);
        REQUIRE_FALSE(d2);
        REQUIRE(d3);
        REQUIRE_FALSE(d4);
        CHECK_EQ(d3.value(), std::tuple{10, 20});
    }

    {
        REQUIRE(!p.eof());

        auto [d1, d2, d3, d4] =
            p.template try_object<test_struct, int, double, char>(
                 [] { return false; })
                .template or_else<int, double>([](auto&) { return false; })
                .template or_object<test_struct, int, double, char>()
                .template or_else<int, int>(fail)
                .values();

        REQUIRE(p.valid());
        REQUIRE_FALSE(d1);
        REQUIRE_FALSE(d2);
        REQUIRE(d3);
        REQUIRE_FALSE(d4);
        CHECK_EQ(d3->tied(), std::tuple{10, 22.2, 'f'});
    }

    CHECK(p.eof());
}

// various scenarios
TEST_CASE("parser test composite conversion") {
    test_composite_conversion<ss::string_error>();
}

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

template <typename... Ts>
void test_moving_of_parsed_composite_values() {
    // to compile is enough
    return;
    ss::parser<Ts...> p{"", ""};
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
    test_moving_of_parsed_composite_values();
    test_moving_of_parsed_composite_values<ss::string_error>();
}

TEST_CASE("parser test error mode") {
    unique_file_name f{"test_parser"};
    {
        std::ofstream out{f.name};
        out << "junk" << std::endl;
        out << "junk" << std::endl;
    }

    ss::parser<ss::string_error> p(f.name, ",");

    REQUIRE_FALSE(p.eof());
    p.get_next<int>();
    CHECK_FALSE(p.valid());
    CHECK_FALSE(p.error_msg().empty());
}

TEST_CASE("parser throw on error mode") {
    unique_file_name f{"test_parser"};
    {
        std::ofstream out{f.name};
        out << "junk" << std::endl;
        out << "junk" << std::endl;
    }

    ss::parser<ss::throw_on_error> p(f.name, ",");

    REQUIRE_FALSE(p.eof());
    try {
        p.get_next<int>();
        FAIL("Expected exception...");
    } catch (const std::exception& e) {
        CHECK_FALSE(std::string{e.what()}.empty());
    }
}

static inline std::string no_quote(const std::string& s) {
    if (!s.empty() && s[0] == '"') {
        return {std::next(begin(s)), std::prev(end(s))};
    }
    return s;
}

template <typename... Ts>
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

    ss::parser<ss::multiline, ss::quote<'"'>, Ts...> p{f.name, ","};
    std::vector<X> i;

    while (!p.eof()) {
        auto a = p.template get_next<int, double, std::string>();
        i.emplace_back(ss::to_object<X>(a));
    }

    for (auto& [_, __, s] : i) {
        update_if_crlf(s);
    }
    CHECK_EQ(i, data);

    ss::parser<ss::quote<'"'>, Ts...> p_no_multiline{f.name, ","};
    while (!p.eof()) {
        auto command = [&] {
            p_no_multiline.template get_next<int, double, std::string>();
        };
        expect_error_on_command(p_no_multiline, command);
    }
}

TEST_CASE("parser test csv on multiple lines with quotes") {
    test_quote_multiline();
    test_quote_multiline<ss::string_error>();
    test_quote_multiline<ss::throw_on_error>();
}

static inline std::string no_escape(std::string& s) {
    s.erase(std::remove(begin(s), end(s), '\\'), end(s));
    return s;
}

TEST_CASE("parser test csv on multiple lines with escapes") {
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

    ss::parser<ss::multiline, ss::escape<'\\'>> p{f.name, ","};
    std::vector<X> i;

    while (!p.eof()) {
        auto a = p.get_next<int, double, std::string>();
        i.emplace_back(ss::to_object<X>(a));
    }

    for (auto& [_, __, s] : i) {
        update_if_crlf(s);
    }
    CHECK_EQ(i, data);

    ss::parser<ss::escape<'\\'>> p_no_multiline{f.name, ","};
    while (!p.eof()) {
        auto a = p_no_multiline.get_next<int, double, std::string>();
        CHECK_FALSE(p.valid());
    }
}

TEST_CASE("parser test csv on multiple lines with quotes and escapes") {
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

    ss::parser<ss::multiline, ss::escape<'\\'>, ss::quote<'"'>> p{f.name};
    std::vector<X> i;

    while (!p.eof()) {
        auto a = p.get_next<int, double, std::string>();
        if (p.valid()) {
            i.emplace_back(ss::to_object<X>(a));
        }
    }

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

TEST_CASE("parser test multiline restricted") {
    unique_file_name f{"test_parser"};
    {
        std::ofstream out{f.name};
        out << "1,2,\"just\n\nstrings\"" << std::endl;
#ifndef _WIN32
        out << "3,4,\"ju\n\r\n\nnk\"" << std::endl;
        out << "5,6,just\\\n\\\r\nstrings" << std::endl;
#else
        out << "3,4,\"ju\n\n\nnk\"" << std::endl;
        out << "5,6,just\\\n\\\nstrings" << std::endl;
#endif
        out << "7,8,ju\\\n\\\n\\\nnk" << std::endl;
        out << "9,10,\"just\\\n\nstrings\"" << std::endl;
        out << "11,12,\"ju\\\n|\n\n\n\n\nk\"" << std::endl;
        out << "13,14,\"ju\\\n\\\n15,16\"\\\n\\\\\n\nnk\"" << std::endl;
        out << "17,18,\"ju\\\n\\\n\\\n\\\\\n\nnk\"" << std::endl;
        out << "19,20,just strings" << std::endl;
    }

    ss::parser<ss::multiline_restricted<2>, ss::quote<'"'>, ss::escape<'\\'>>
        p{f.name, ","};
    std::vector<X> i;

    while (!p.eof()) {
        auto a = p.get_next<int, double, std::string>();
        if (p.valid()) {
            i.emplace_back(ss::to_object<X>(a));
        }
    }

    std::vector<X> data = {{1, 2, "just\n\nstrings"},
#ifndef _WIN32
                           {5, 6, "just\n\r\nstrings"},
#else
                           {5, 6, "just\n\nstrings"},
#endif
                           {9, 10, "just\n\nstrings"},
                           {19, 20, "just strings"}};

    for (auto& [_, __, s] : i) {
        update_if_crlf(s);
    }

    CHECK_EQ(i, data);
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

template <typename... Ts>
void test_unterminated_line_impl(const std::vector<std::string>& lines,
                                 size_t bad_line) {
    unique_file_name f{"test_parser"};
    std::ofstream out{f.name};
    for (const auto& line : lines) {
        out << line << std::endl;
    }
    out.close();

    ss::parser<Ts...> p{f.name};
    size_t line = 0;
    while (!p.eof()) {
        auto command = [&] { p.template get_next<int, double, std::string>(); };

        if (line == bad_line) {
            expect_error_on_command(p, command);
            break;
        } else {
            CHECK(p.valid());
            ++line;
        }
    }
}

template <typename... Ts>
void test_unterminated_line(const std::vector<std::string>& lines,
                            size_t bad_line) {
    test_unterminated_line_impl<Ts...>(lines, bad_line);
    test_unterminated_line_impl<Ts..., ss::string_error>(lines, bad_line);
    test_unterminated_line_impl<Ts..., ss::throw_on_error>(lines, bad_line);
}

// TODO add more cases
TEST_CASE("parser test csv on multiple with errors") {
    using multiline = ss::multiline_restricted<3>;
    using escape = ss::escape<'\\'>;
    using quote = ss::quote<'"'>;

    // unterminated escape
    {
        const std::vector<std::string> lines{"1,2,just\\"};
        test_unterminated_line<multiline, escape>(lines, 0);
        test_unterminated_line<multiline, escape, quote>(lines, 0);
    }

    // unterminated quote
    {
        const std::vector<std::string> lines{"1,2,\"just"};
        test_unterminated_line<multiline, escape, quote>(lines, 0);
        test_unterminated_line<multiline, quote>(lines, 0);
    }

    // unterminated quote and escape
    {
        const std::vector<std::string> lines{"1,2,\"just\\"};
        test_unterminated_line<multiline, escape, quote>(lines, 0);
    }

    {
        const std::vector<std::string> lines{"1,2,\"just\\\n\\"};
        test_unterminated_line<multiline, escape, quote>(lines, 0);
    }

    {
        const std::vector<std::string> lines{"1,2,\"just\n\\"};
        test_unterminated_line<multiline, escape, quote>(lines, 0);
    }

    // multiline limmit reached escape
    {
        const std::vector<std::string> lines{"1,2,\\\n\\\n\\\n\\\njust"};
        test_unterminated_line<multiline, escape>(lines, 0);
        test_unterminated_line<multiline, escape, quote>(lines, 0);
    }

    // multiline limmit reached quote
    {
        const std::vector<std::string> lines{"1,2,\"\n\n\n\n\njust\""};
        test_unterminated_line<multiline, escape, quote>(lines, 0);
        test_unterminated_line<multiline, quote>(lines, 0);
    }

    // multiline limmit reached quote and escape
    {
        const std::vector<std::string> lines{"1,2,\"\\\n\n\\\n\\\n\\\njust"};
        test_unterminated_line<multiline, escape, quote>(lines, 0);
    }
}

template <typename T, typename Tuple>
struct has_type;

template <typename T, typename... Us>
struct has_type<T, std::tuple<Us...>>
    : std::disjunction<std::is_same<T, Us>...> {};

static inline void check_size(size_t size1, size_t size2) {
    CHECK_EQ(size1, size2);
}

template <typename... Ts>
static void test_fields(const std::string file_name, const std::vector<X>& data,
                        const std::vector<std::string>& fields) {
    using CaseType = std::tuple<Ts...>;

    ss::parser p{file_name, ","};
    CHECK_FALSE(p.field_exists("Unknown"));
    p.use_fields(fields);
    std::vector<CaseType> i;

    for (const auto& a : p.iterate<CaseType>()) {
        i.push_back(a);
    }

    check_size(i.size(), data.size());
    for (size_t j = 0; j < i.size(); ++j) {
        if constexpr (has_type<int, CaseType>::value) {
            CHECK_EQ(std::get<int>(i[j]), data[j].i);
        }
        if constexpr (has_type<double, CaseType>::value) {
            CHECK_EQ(std::get<double>(i[j]), data[j].d);
        }
        if constexpr (has_type<std::string, CaseType>::value) {
            CHECK_EQ(std::get<std::string>(i[j]), data[j].s);
        }
    }
}

TEST_CASE("parser test various cases with header") {
    unique_file_name f{"test_parser"};
    constexpr static auto Int = "Int";
    constexpr static auto Dbl = "Double";
    constexpr static auto Str = "String";
    using str = std::string;

    std::vector<std::string> header{Int, Dbl, Str};

    std::vector<X> data = {{1, 2, "x"}, {3, 4, "y"},  {5, 6, "z"},
                           {7, 8, "u"}, {9, 10, "v"}, {11, 12, "w"}};

    make_and_write(f.name, data, header);
    const auto& o = f.name;
    const auto& d = data;

    {
        ss::parser<ss::string_error> p{f.name, ","};
        std::vector<X> i;

        for (const auto& a : p.iterate<int, double, std::string>()) {
            i.emplace_back(ss::to_object<X>(a));
        }

        CHECK_NE(i, data);
    }

    {
        ss::parser<ss::string_error> p{f.name, ","};
        std::vector<X> i;

        p.ignore_next();
        for (const auto& a : p.iterate<int, double, std::string>()) {
            i.emplace_back(ss::to_object<X>(a));
        }

        CHECK_EQ(i, data);
    }

    {
        ss::parser<ss::ignore_header> p{f.name, ","};
        std::vector<X> i;

        for (const auto& a : p.iterate<int, double, std::string>()) {
            i.emplace_back(ss::to_object<X>(a));
        }

        CHECK_EQ(i, data);
    }

    {
        ss::parser<ss::ignore_header, ss::string_error> p{f.name, ","};
        p.use_fields(Int, Dbl, Str);
        CHECK_FALSE(p.valid());
    }

    {
        ss::parser<ss::ignore_header, ss::string_error> p{f.name, ","};
        CHECK_FALSE(p.field_exists("Unknown"));

        p.use_fields(Int, "Unknown");
        CHECK_FALSE(p.valid());
    }

    {
        ss::parser<ss::ignore_header, ss::string_error> p{f.name, ","};
        p.use_fields(Int, Int);
        CHECK_FALSE(p.valid());
    }

    {
        ss::parser<ss::string_error> p{f.name, ","};
        p.use_fields(Int, Dbl);

        {
            auto [int_, double_] = p.get_next<int, double>();
            CHECK_EQ(int_, data[0].i);
            CHECK_EQ(double_, data[0].d);
        }

        p.use_fields(Dbl, Int);

        {
            auto [double_, int_] = p.get_next<double, int>();
            CHECK_EQ(int_, data[1].i);
            CHECK_EQ(double_, data[1].d);
        }

        p.use_fields(Str);

        {
            auto string_ = p.get_next<std::string>();
            CHECK_EQ(string_, data[2].s);
        }

        p.use_fields(Str, Int, Dbl);

        {
            auto [string_, int_, double_] =
                p.get_next<std::string, int, double>();
            CHECK_EQ(double_, data[3].d);
            CHECK_EQ(int_, data[3].i);
            CHECK_EQ(string_, data[3].s);
        }
    }

    /*  python used to generate permutations
        import itertools

        header = {'str': 'Str',
                 'double': 'Dbl',
                 'int': 'Int'}

        keys = ['str', 'int', 'double']

        for r in range (1, 3):
            combinations = list(itertools.permutations(keys, r = r))

            for combination in combinations:
                template_params = []
                arg_params = []
                for type in combination:
                    template_params.append(type)
                    arg_params.append(header[type])
                call = 'testFields<' + ', '.join(template_params) + \
                    '>(o, d, {' + ', '.join(arg_params) + '});'
                print(call)
        */

    test_fields<str>(o, d, {Str});
    test_fields<int>(o, d, {Int});
    test_fields<double>(o, d, {Dbl});
    test_fields<str, int>(o, d, {Str, Int});
    test_fields<str, double>(o, d, {Str, Dbl});
    test_fields<int, str>(o, d, {Int, Str});
    test_fields<int, double>(o, d, {Int, Dbl});
    test_fields<double, str>(o, d, {Dbl, Str});
    test_fields<double, int>(o, d, {Dbl, Int});
    test_fields<str, int, double>(o, d, {Str, Int, Dbl});
    test_fields<str, double, int>(o, d, {Str, Dbl, Int});
    test_fields<int, str, double>(o, d, {Int, Str, Dbl});
    test_fields<int, double, str>(o, d, {Int, Dbl, Str});
    test_fields<double, str, int>(o, d, {Dbl, Str, Int});
    test_fields<double, int, str>(o, d, {Dbl, Int, Str});
}

template <typename... Ts>
void test_invalid_fields_impl(const std::vector<std::string>& lines,
                              const std::vector<std::string>& fields) {
    unique_file_name f{"test_parser"};
    std::ofstream out{f.name};
    for (const auto& line : lines) {
        out << line << std::endl;
    }
    out.close();

    {
        // No fields specified
        ss::parser<Ts...> p{f.name, ","};
        auto command = [&] { p.use_fields(); };
        expect_error_on_command(p, command);
    }

    {
        // Unknown field
        ss::parser<Ts...> p{f.name, ","};
        auto command = [&] { p.use_fields("Unknown"); };
        expect_error_on_command(p, command);
    }

    {
        // Field used multiple times
        ss::parser<Ts...> p{f.name, ","};
        auto command = [&] { p.use_fields(fields[0], fields[0]); };
        expect_error_on_command(p, command);
    }

    {
        // Mapping out of range
        ss::parser<Ts...> p{f.name, ","};
        auto command = [&] {
            p.use_fields(fields[0]);
            p.template get_next<std::string, std::string>();
        };
        expect_error_on_command(p, command);
    }
}

template <typename... Ts>
void test_invalid_fields(const std::vector<std::string>& lines,
                         const std::vector<std::string>& fields) {
    test_invalid_fields_impl(lines, fields);
    test_invalid_fields_impl<ss::string_error>(lines, fields);
    test_invalid_fields_impl<ss::throw_on_error>(lines, fields);
}

// TODO add more test cases
TEST_CASE("parser test invalid header fields usage") {
    test_invalid_fields({"Int,String,Double", "1,hi,2.34"},
                        {"Int", "String", "Double"});
}

static inline void test_ignore_empty(const std::vector<X>& data) {
    unique_file_name f{"test_parser"};
    make_and_write(f.name, data);

    std::vector<X> expected;
    for (const auto& d : data) {
        if (d.s != X::make_empty) {
            expected.push_back(d);
        }
    }

    {
        ss::parser<ss::string_error, ss::ignore_empty> p{f.name, ","};

        std::vector<X> i;
        for (const auto& a : p.iterate<X>()) {
            i.push_back(a);
        }

        CHECK_EQ(i, expected);
    }

    {
        ss::parser<ss::string_error> p{f.name, ","};
        std::vector<X> i;
        size_t n = 0;
        for (const auto& a : p.iterate<X>()) {
            if (data.at(n).s == X::make_empty) {
                CHECK_FALSE(p.valid());
            }
            i.push_back(a);
            ++n;
        }

        if (data != expected) {
            CHECK_NE(i, expected);
        }
    }
}

TEST_CASE("parser test various cases with empty lines") {
    test_ignore_empty({{1, 2, "x"}, {3, 4, "y"}, {9, 10, "v"}, {11, 12, "w"}});

    test_ignore_empty(
        {{1, 2, X::make_empty}, {3, 4, "y"}, {9, 10, "v"}, {11, 12, "w"}});

    test_ignore_empty(
        {{1, 2, "x"}, {3, 4, "y"}, {9, 10, "v"}, {11, 12, X::make_empty}});

    test_ignore_empty(
        {{1, 2, "x"}, {5, 6, X::make_empty}, {9, 10, "v"}, {11, 12, "w"}});

    test_ignore_empty({{1, 2, X::make_empty},
                       {5, 6, X::make_empty},
                       {9, 10, "v"},
                       {11, 12, "w"}});

    test_ignore_empty({{1, 2, X::make_empty},
                       {3, 4, "y"},
                       {9, 10, "v"},
                       {11, 12, X::make_empty}});

    test_ignore_empty({{1, 2, "x"},
                       {3, 4, "y"},
                       {9, 10, X::make_empty},
                       {11, 12, X::make_empty}});

    test_ignore_empty({{1, 2, X::make_empty},
                       {3, 4, "y"},
                       {9, 10, X::make_empty},
                       {11, 12, X::make_empty}});

    test_ignore_empty({{1, 2, X::make_empty},
                       {3, 4, X::make_empty},
                       {9, 10, X::make_empty},
                       {11, 12, X::make_empty}});

    test_ignore_empty({{1, 2, "x"},
                       {3, 4, X::make_empty},
                       {9, 10, X::make_empty},
                       {11, 12, X::make_empty}});

    test_ignore_empty({{1, 2, X::make_empty},
                       {3, 4, X::make_empty},
                       {9, 10, X::make_empty},
                       {11, 12, "w"}});

    test_ignore_empty({{11, 12, X::make_empty}});

    test_ignore_empty({});
}
