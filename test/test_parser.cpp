#include "test_helpers.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ss/parser.hpp>
#include <sstream>

std::string time_now_rand() {
    std::stringstream ss;
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    ss << std::put_time(&tm, "%d%m%Y%H%M%S");
    srand(time(nullptr));
    return ss.str() + std::to_string(rand());
}

inline int i = 0;
struct unique_file_name {
    const std::string name;

    unique_file_name()
        : name{"random_" + std::to_string(i++) + time_now_rand() +
               "_file.csv"} {
    }

    ~unique_file_name() {
        std::filesystem::remove(name);
    }
};

void replace_all(std::string& s, const std::string& from,
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
    int i;
    double d;
    std::string s;

    std::string to_string() const {
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

#include <iostream>

TEST_CASE("parser test various cases") {
    unique_file_name f;
    std::vector<X> data = {{1, 2, "x"}, {3, 4, "y"},  {5, 6, "z"},
                           {7, 8, "u"}, {9, 10, "v"}, {11, 12, "w"}};
    make_and_write(f.name, data);
    {
        ss::parser<ss::string_error> p{f.name, ","};
        std::vector<X> i;

        ss::parser<ss::string_error> p2{f.name, ","};
        std::vector<X> i2;

        while (!p.eof()) {
            auto a = p.get_next<int, double, std::string>();
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
                     [](const X& x) { return x.i != excluded; });
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
        unique_file_name empty_f;
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

using test_tuple = std::tuple<double, char, double>;
struct test_struct {
    int i;
    double d;
    char c;
    auto tied() {
        return std::tie(i, d, c);
    }
};

void expect_test_struct(const test_struct&) {
}

// various scenarios
TEST_CASE("parser test composite conversion") {
    unique_file_name f;
    {
        std::ofstream out{f.name};
        for (auto& i :
             {"10,a,11.1", "10,20,11.1", "junk", "10,11.1", "1,11.1,a", "junk",
              "10,junk", "11,junk", "10,11.1,c", "10,20", "10,22.2,f"}) {
            out << i << std::endl;
        }
    }

    ss::parser<ss::string_error> p{f.name, ","};
    auto fail = [] { FAIL(""); };
    auto expect_error = [](auto error) { CHECK(!error.empty()); };

    REQUIRE(p.valid());
    REQUIRE_FALSE(p.eof());

    {
        constexpr static auto expectedData = std::tuple{10, 'a', 11.1};

        auto [d1, d2, d3, d4] =
            p.try_next<int, int, double>(fail)
                .or_else<test_struct>(fail)
                .or_else<int, char, double>(
                    [](auto&& data) { CHECK_EQ(data, expectedData); })
                .on_error(fail)
                .or_else<test_tuple>(fail)
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
            p.try_next<int, int, double>([](auto& i1, auto i2, double d) {
                 CHECK_EQ(std::tie(i1, i2, d), expectedData);
             })
                .on_error(fail)
                .or_object<test_struct, int, double, char>(fail)
                .on_error(fail)
                .or_else<test_tuple>(fail)
                .on_error(fail)
                .or_else<int, char, double>(fail)
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
            p.try_object<test_struct, int, double, char>(fail)
                .on_error(expect_error)
                .or_else<int, char, char>(fail)
                .or_else<test_struct>(fail)
                .or_else<test_tuple>(fail)
                .or_else<int, char, double>(fail)
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
            p.try_next<int, double>([](auto& i, auto& d) {
                 REQUIRE_EQ(std::tie(i, d), std::tuple{10, 11.1});
             })
                .or_else<int, double>([](auto&, auto&) { FAIL(""); })
                .values();

        REQUIRE(p.valid());
        REQUIRE(d1);
        REQUIRE_FALSE(d2);
    }

    {
        REQUIRE(!p.eof());

        auto [d1, d2] = p.try_next<int, double>([](auto&, auto&) { FAIL(""); })
                            .or_else<test_struct>(expect_test_struct)
                            .values();

        REQUIRE(p.valid());
        REQUIRE_FALSE(d1);
        REQUIRE(d2);
        CHECK_EQ(d2->tied(), std::tuple{1, 11.1, 'a'});
    }

    {
        REQUIRE(!p.eof());

        auto [d1, d2, d3, d4, d5] =
            p.try_next<int, int, double>(fail)
                .or_object<test_struct, int, double, char>()
                .or_else<test_struct>(expect_test_struct)
                .or_else<test_tuple>(fail)
                .or_else<std::tuple<int, double>>(fail)
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

        auto [d1, d2] = p.try_next<int, std::optional<int>>()
                            .on_error(fail)
                            .or_else<std::tuple<int, std::string>>(fail)
                            .on_error(fail)
                            .values();

        REQUIRE(p.valid());
        REQUIRE(d1);
        REQUIRE_FALSE(d2);
        CHECK_EQ(*d1, std::tuple{10, std::nullopt});
    }

    {
        REQUIRE_FALSE(p.eof());

        auto [d1, d2] = p.try_next<int, std::variant<int, std::string>>()
                            .on_error(fail)
                            .or_else<std::tuple<int, std::string>>(fail)
                            .on_error(fail)
                            .values();

        REQUIRE(p.valid());
        REQUIRE(d1);
        REQUIRE_FALSE(d2);
        CHECK_EQ(*d1, std::tuple{11, std::variant<int, std::string>{"junk"}});
    }

    {
        REQUIRE(!p.eof());

        auto [d1, d2] = p.try_object<test_struct, int, double, char>()
                            .or_else<int>(fail)
                            .values();
        REQUIRE(p.valid());
        REQUIRE(d1);
        REQUIRE_FALSE(d2);
        CHECK_EQ(d1->tied(), std::tuple{10, 11.1, 'c'});
    }

    {
        REQUIRE_FALSE(p.eof());

        auto [d1, d2, d3, d4] =
            p.try_next<int, int>([] { return false; })
                .or_else<int, double>([](auto&) { return false; })
                .or_else<int, int>()
                .or_else<int, int>(fail)
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
            p.try_object<test_struct, int, double, char>([] { return false; })
                .or_else<int, double>([](auto&) { return false; })
                .or_object<test_struct, int, double, char>()
                .or_else<int, int>(fail)
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

size_t move_called = 0;

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
        move_called++;
        other.data = nullptr;
    }

    my_string& operator=(my_string&& other) {
        move_called++;
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

TEST_CASE("parser test the moving of parsed values") {
    size_t move_called_one_col;

    {
        unique_file_name f;
        {
            std::ofstream out{f.name};
            out << "x" << std::endl;
        }

        ss::parser p{f.name, ","};
        auto x = p.get_next<my_string>();
        CHECK_LT(move_called, 3);
        move_called_one_col = move_called;
        move_called = 0;
    }

    unique_file_name f;
    {
        std::ofstream out{f.name};
        out << "a,b,c" << std::endl;
    }

    {

        ss::parser p{f.name, ","};
        auto x = p.get_next<my_string, my_string, my_string>();
        CHECK_LE(move_called, 3 * move_called_one_col);
        move_called = 0;
    }

    {
        ss::parser p{f.name, ","};
        auto x = p.get_object<xyz, my_string, my_string, my_string>();
        CHECK_LE(move_called, 6 * move_called_one_col);
        move_called = 0;
    }

    {
        ss::parser p{f.name, ","};
        auto x = p.get_next<xyz>();
        CHECK_LE(move_called, 6 * move_called_one_col);
        move_called = 0;
    }
}

TEST_CASE("parser test the moving of parsed composite values") {
    // to compile is enough
    return;
    ss::parser p{"", ""};
    p.try_next<my_string, my_string, my_string>()
        .or_else<my_string, my_string, my_string, my_string>([](auto&&) {})
        .or_else<my_string>([](auto&) {})
        .or_else<xyz>([](auto&&) {})
        .or_object<xyz, my_string, my_string, my_string>([](auto&&) {})
        .or_else<std::tuple<my_string, my_string, my_string>>(
            [](auto&, auto&, auto&) {});
}

TEST_CASE("parser test error mode") {
    unique_file_name f;
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

std::string no_quote(const std::string& s) {
    if (!s.empty() && s[0] == '"') {
        return {std::next(begin(s)), std::prev(end(s))};
    }
    return s;
}

TEST_CASE("parser test csv on multiple lines with quotes") {
    unique_file_name f;
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

    ss::parser<ss::multiline, ss::quote<'"'>> p{f.name, ","};
    std::vector<X> i;

    while (!p.eof()) {
        auto a = p.get_next<int, double, std::string>();
        i.emplace_back(ss::to_object<X>(a));
    }

    for (auto& [_, __, s] : i) {
        update_if_crlf(s);
    }
    CHECK_EQ(i, data);

    ss::parser<ss::quote<'"'>> p_no_multiline{f.name, ","};
    while (!p.eof()) {
        auto a = p_no_multiline.get_next<int, double, std::string>();
        CHECK(!p.valid());
    }
}

std::string no_escape(std::string& s) {
    s.erase(std::remove(begin(s), end(s), '\\'), end(s));
    return s;
}

TEST_CASE("parser test csv on multiple lines with escapes") {
    unique_file_name f;
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
    unique_file_name f;
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
    unique_file_name f;
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

template <typename T, typename Tuple>
struct has_type;

template <typename T, typename... Us>
struct has_type<T, std::tuple<Us...>>
    : std::disjunction<std::is_same<T, Us>...> {};

void checkSize(size_t size1, size_t size2) {
    CHECK_EQ(size1, size2);
}

template <typename... Ts>
void testFields(const std::string file_name, const std::vector<X>& data,
                const std::vector<std::string>& fields) {
    using CaseType = std::tuple<Ts...>;

    ss::parser p{file_name, ","};
    p.use_fields(fields);
    std::vector<CaseType> i;

    for (const auto& a : p.iterate<CaseType>()) {
        i.push_back(a);
    }

    checkSize(i.size(), data.size());
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
    unique_file_name f;
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
        p.use_fields(Int, "Unknown");
        CHECK_FALSE(p.valid());
    }

    {
        ss::parser<ss::ignore_header, ss::string_error> p{f.name, ","};
        p.use_fields(Int, Int);
        CHECK_FALSE(p.valid());
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

    testFields<str>(o, d, {Str});
    testFields<int>(o, d, {Int});
    testFields<double>(o, d, {Dbl});
    testFields<str, int>(o, d, {Str, Int});
    testFields<str, double>(o, d, {Str, Dbl});
    testFields<int, str>(o, d, {Int, Str});
    testFields<int, double>(o, d, {Int, Dbl});
    testFields<double, str>(o, d, {Dbl, Str});
    testFields<double, int>(o, d, {Dbl, Int});
    testFields<str, int, double>(o, d, {Str, Int, Dbl});
    testFields<str, double, int>(o, d, {Str, Dbl, Int});
    testFields<int, str, double>(o, d, {Int, Str, Dbl});
    testFields<int, double, str>(o, d, {Int, Dbl, Str});
    testFields<double, str, int>(o, d, {Dbl, Str, Int});
    testFields<double, int, str>(o, d, {Dbl, Int, Str});
}
