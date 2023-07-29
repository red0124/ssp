#include "test_helpers.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ss/parser.hpp>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

// TODO add single header tests
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
        // TODO uncomment
        // std::filesystem::remove(name);
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

#if 0
#include <iostream>

TEST_CASE("parser test various cases") {
    unique_file_name f;
    std::vector<X> data = {{1, 2, "x"}, {3, 4, "y"},  {5, 6, "z"},
                           {7, 8, "u"}, {9, 10, "v"}, {11, 12, "w"}};
    make_and_write(f.name, data);
    {
        ss::parser<ss::string_error> p{f.name, ","};
        ss::parser p0{std::move(p)};
        p = std::move(p0);
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
                    [&](auto&& data) { CHECK_EQ(data, expectedData); })
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
            p.try_next<int, int, double>([&](auto& i1, auto i2, double d) {
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

void check_size(size_t size1, size_t size2) {
    CHECK_EQ(size1, size2);
}

template <typename... Ts>
void test_fields(const std::string file_name, const std::vector<X>& data,
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

void test_ignore_empty(const std::vector<X>& data) {
    unique_file_name f;
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
#endif

////////////////
// parser tests v2
////////////////

#include <iostream>
#include <regex>
struct random_number_generator {
    size_t z1 = 12341;
    size_t z2 = 12342;
    size_t z3 = 12343;
    size_t z4 = 12344;

    size_t rand() {
        unsigned int b;
        b = ((z1 << 6) ^ z1) >> 13;
        z1 = ((z1 & 4294967294U) << 18) ^ b;
        b = ((z2 << 2) ^ z2) >> 27;
        z2 = ((z2 & 4294967288U) << 2) ^ b;
        b = ((z3 << 13) ^ z3) >> 21;
        z3 = ((z3 & 4294967280U) << 7) ^ b;
        b = ((z4 << 3) ^ z4) >> 12;
        z4 = ((z4 & 4294967168U) << 13) ^ b;
        return (z1 ^ z2 ^ z3 ^ z4);
    }

    template <typename T>
    size_t rand_index(const T& s) {
        REQUIRE(!s.empty());
        return rand() % s.size();
    }

    bool rand_bool() {
        return (rand() % 100) > 50;
    }

    template <typename T>
    void rand_insert(std::string& dst, const T& src) {
        dst.insert(rand_index(dst), std::string{src});
    }

    template <typename T>
    void rand_insert_n(std::string& dst, const T& src, size_t n_max) {
        size_t n = rand() % n_max;
        for (size_t i = 0; i < n; ++i) {
            rand_insert(dst, src);
        }
    }
} rng;

struct field {
    std::string value;
    bool is_string = false;
    bool has_spaces_left = false;
    bool has_spaces_right = false;
    bool has_new_line = false;

    field(const std::string& input) {
        value = input;
        is_string = true;

        has_spaces_left = !input.empty() && input.front() == ' ';
        has_spaces_right = !input.empty() && input.back() == ' ';
        has_new_line = input.find_first_of('\n') != std::string::npos;
    }

    field(int input) {
        value = std::to_string(input);
    }

    field(double input) {
        value = std::to_string(input);
    }
};

struct column {
    std::string header;
    std::vector<field> fields;
};

template <typename... Ts>
column make_column(const std::string& input_header,
                   const std::vector<field>& input_fields) {
    using setup = ss::setup<Ts...>;
    std::vector<field> filtered_fields;

    for (const auto& el : input_fields) {
        if (!setup::multiline::enabled && el.has_new_line) {
            continue;
        }

        if (!setup::escape::enabled && !setup::quote::enabled) {
            if (setup::trim_left::enabled && el.has_spaces_left) {
                continue;
            }

            if (setup::trim_right::enabled && el.has_spaces_right) {
                continue;
            }
        }

        filtered_fields.push_back(el);
    }

    column c;
    c.header = input_header;
    c.fields = filtered_fields;
    return c;
}

void replace_all2(std::string& s, const std::string& old_value,
                  const std::string& new_value) {
    for (size_t i = 0; i < 999; ++i) {
        size_t pos = s.find(old_value);
        if (pos == std::string::npos) {
            return;
        }
        s.replace(pos, old_value.size(), new_value);
    }
    FAIL("bad replace");
}

template <typename... Ts>
std::vector<std::string> generate_csv_data(const std::vector<field>& data,
                                           const std::string& delim) {
    (void)delim;
    using setup = ss::setup<Ts...>;
    constexpr static auto escape = '\\';
    constexpr static auto quote = '"';
    constexpr static auto space = ' ';
    constexpr static auto new_line = '\n';
    constexpr static auto helper0 = '#';
    constexpr static auto helper1 = '$';
    // constexpr static auto helper3 = '&';

    std::vector<std::string> output;

    if (setup::escape::enabled && setup::quote::enabled) {
        for (const auto& el : data) {
            auto value = el.value;

            replace_all2(value, {escape, quote}, {helper1});

            bool quote_newline = rng.rand_bool();
            bool quote_spacings = rng.rand_bool();
            bool has_spaces = el.has_spaces_right || el.has_spaces_left;

            // handle escape
            replace_all2(value, {escape}, {helper0});
            rng.rand_insert_n(value, escape, 2);
            if (!quote_newline) {
                replace_all2(value, {new_line}, {helper1});
                replace_all2(value, {helper1}, {escape, new_line});
            }
            replace_all2(value, {escape, escape}, {escape});
            replace_all2(value, {escape, helper0}, {helper0});
            replace_all2(value, {helper0, escape}, {helper0});
            replace_all2(value, {helper0}, {escape, escape});

            replace_all2(value, {helper1}, {escape, quote});

            replace_all2(value, {escape, quote}, {helper1});

            if (rng.rand_bool() || quote_newline ||
                (quote_spacings && has_spaces)) {
                replace_all2(value, {quote}, {helper0});
                if (rng.rand_bool()) {
                    replace_all2(value, {helper0}, {escape, quote});
                } else {
                    replace_all2(value, {helper0}, {quote, quote});
                }
                value = std::string{quote} + value + std::string{quote};
            }

            replace_all2(value, {helper1}, {escape, quote});

            if (!quote_spacings && has_spaces) {
                replace_all2(value, {escape, space}, {helper0});
                replace_all2(value, {space}, {helper0});
                replace_all2(value, {helper0}, {escape, space});
            }

            output.push_back(value);
        }
    } else if (setup::escape::enabled) {
        for (const auto& el : data) {
            auto value = el.value;

            replace_all2(value, {escape}, {helper0});
            rng.rand_insert_n(value, escape, 3);
            replace_all2(value, {new_line}, {helper1});
            replace_all2(value, {helper1}, {escape, new_line});

            replace_all2(value, {escape, escape}, {escape});
            replace_all2(value, {escape, helper0}, {helper0});

            replace_all2(value, {helper0, escape}, {helper0});
            replace_all2(value, {helper0}, {escape, escape});

            if (setup::trim_right::enabled || setup::trim_left::enabled) {
                // escape space
                replace_all2(value, {escape, space}, {helper0});
                replace_all2(value, {space}, {helper0});
                replace_all2(value, {helper0}, {escape, space});
            }

            output.push_back(value);
        }
    } else if (setup::quote::enabled) {
        for (const auto& el : data) {
            auto value = el.value;
            if (rng.rand_bool() || el.has_new_line || el.has_spaces_left ||
                el.has_spaces_right) {
                replace_all2(value, {quote}, {helper0});
                replace_all2(value, {helper0}, {quote, quote});
                value = std::string{quote} + value + std::string{quote};
            }
            output.push_back(value);
        }
    } else {
        for (const auto& el : data) {
            output.push_back(el.value);
        }
    }

    if (setup::trim_right::enabled) {
        for (auto& el : output) {
            size_t n = rng.rand();
            for (size_t i = 0; i < n % 3; ++i) {
                el = el + " ";
            }
        }
    }

    if (setup::trim_left::enabled) {
        for (auto& el : output) {
            size_t n = rng.rand();
            for (size_t i = 0; i < n % 3; ++i) {
                el = " " + el;
            }
        }
    }

    return output;
}

void write_to_file(const std::vector<std::string>& data,
                   const std::string& delim, const std::string& file_name) {
    std::ofstream out{file_name, std::ios_base::app};
    for (size_t i = 0; i < data.size(); ++i) {
        out << data[i];
        if (i != data.size() - 1) {
            out << delim;
        }
    }
    out << std::endl;
    out.close();
}

template <typename... Ts>
void test_combinations(const std::vector<column>& input_data,
                       const std::string& delim, bool include_header) {
    // TODO test without string_error
    using setup = ss::setup<Ts..., ss::string_error>;

    unique_file_name f;
    std::vector<std::vector<field>> expected_data;
    std::vector<std::string> header;
    std::vector<field> field_header;

    for (const auto& el : input_data) {
        header.push_back(el.header);
        field_header.push_back(field{el.header});
    }

    if (include_header) {
        auto header_data = generate_csv_data<Ts...>(field_header, delim);
        write_to_file(header_data, delim, f.name);
    }

    std::vector<int> layout;
    size_t n = 1 + rng.rand() % 10;

    for (size_t i = 0; i < input_data.size(); ++i) {
        layout.push_back(i);
    }

    for (size_t i = 0; i < n; ++i) {
        std::vector<field> raw_data;
        for (const auto& el : input_data) {
            const auto& fields = el.fields;
            if (fields.empty()) {
                continue;
            }

            raw_data.push_back(fields[rng.rand_index(fields)]);
        }

        expected_data.push_back(raw_data);
        auto data = generate_csv_data<Ts...>(raw_data, delim);
        write_to_file(data, delim, f.name);

        /*
        std::cout << "[.";
        for (const auto& el : data) {
            std::cout << el << '.';
        }
        std::cout << "]" << std::endl;
         */
    }

    auto layout_combinations = vector_combinations(layout, layout.size());

    auto remove_duplicates = [](const auto& vec) {
        std::vector<int> unique_vec;
        std::unordered_set<int> vec_set;
        for (const auto& el : vec) {
            if (vec_set.find(el) == vec_set.end()) {
                vec_set.insert(el);
                unique_vec.push_back(el);
            }
        }

        return unique_vec;
    };

    std::vector<std::vector<int>> unique_layout_combinations;
    for (const auto& layout : layout_combinations) {
        unique_layout_combinations.push_back(remove_duplicates(layout));
    }

    if (!include_header) {
        unique_layout_combinations.clear();
        unique_layout_combinations.push_back(layout);
    }

    for (const auto& layout : unique_layout_combinations) {
        ss::parser<setup> p{f.name, delim};

        if (include_header) {
            std::vector<std::string> fields;
            for (const auto& index : layout) {
                fields.push_back(header[index]);
            }

            p.use_fields(fields);

            if (!p.valid()) {
                std::cout << p.error_msg() << std::endl;
            }

            REQUIRE(p.valid());
        }

        auto check_error = [&p] {
            CHECK(p.valid());
            if (!p.valid()) {
                std::cout << p.error_msg() << std::endl;
            }
        };

        int num_columns = layout.size();
        for (size_t i = 0; i < n + 1; ++i) {
            switch (num_columns) {
            case 1: {
                auto s0 = p.template get_next<std::string>();
                if (i < n) {
                    check_error();
                    // std::cout << s0 << std::endl;
                    CHECK(s0 == expected_data[i][layout[0]].value);
                } else {
                    CHECK(p.eof());
                    CHECK(!p.valid());
                }
                break;
            }
            case 2: {
                auto [s0, s1] = p.template get_next<std::string, std::string>();
                if (i < n) {
                    check_error();
                    // std::cout << s0 << ' ' << s1 << std::endl;
                    CHECK(s0 == expected_data[i][layout[0]].value);
                    CHECK(s1 == expected_data[i][layout[1]].value);
                } else {
                    CHECK(p.eof());
                    CHECK(!p.valid());
                }
                break;
            }
            case 3: {
                auto [s0, s1, s2] =
                    p.template get_next<std::string, std::string,
                                        std::string>();
                if (i < n) {
                    check_error();
                    // std::cout << s0 << ' ' << s1 << ' ' << s2 << std::endl;
                    CHECK(s0 == expected_data[i][layout[0]].value);
                    CHECK(s1 == expected_data[i][layout[1]].value);
                    CHECK(s2 == expected_data[i][layout[2]].value);
                } else {
                    CHECK(p.eof());
                    CHECK(!p.valid());
                }
                break;
            }
            case 4: {
                auto [s0, s1, s2, s3] =
                    p.template get_next<std::string, std::string, std::string,
                                        std::string>();
                if (i < n) {
                    check_error();
                    /*
                    std::cout << s0 << ' ' << s1 << ' ' << s2 << ' ' << s3
                              << std::endl;
                              */
                    CHECK(s0 == expected_data[i][layout[0]].value);
                    CHECK(s1 == expected_data[i][layout[1]].value);
                    CHECK(s2 == expected_data[i][layout[2]].value);
                    CHECK(s3 == expected_data[i][layout[3]].value);
                } else {
                    CHECK(p.eof());
                    CHECK(!p.valid());
                }
                break;
            }
            case 5: {
                auto [s0, s1, s2, s3, s4] =
                    p.template get_next<std::string, std::string, std::string,
                                        std::string, std::string>();
                if (i < n) {
                    check_error();
                    //std::cout << s0 << ' ' << s1 << ' ' << s2 << ' ' << s3
                              // << ' ' << s4 << std::endl;
                    CHECK(s0 == expected_data[i][layout[0]].value);
                    CHECK(s1 == expected_data[i][layout[1]].value);
                    CHECK(s2 == expected_data[i][layout[2]].value);
                    CHECK(s3 == expected_data[i][layout[3]].value);
                    CHECK(s4 == expected_data[i][layout[4]].value);
                } else {
                    CHECK(p.eof());
                    CHECK(!p.valid());
                }
                break;
            }
            default:
                FAIL(("Invalid number of columns: " +
                      std::to_string(num_columns)));
                break;
            }
        }
    }
}

// TODO rename
template <typename... Ts>
void test_combinations_impl() {
    column ints0 =
        make_column<Ts...>("ints0", {field{123}, field{45}, field{6}});
    column ints1 =
        make_column<Ts...>("ints1", {field{123}, field{45}, field{6}});
    column ints2 =
        make_column<Ts...>("ints2", {field{123}, field{45}, field{6}});

    column floats0 =
        make_column<Ts...>("floats0", {field{1.23}, field{456.7}, field{0.8},
                                       field{910}, field{123456789.987654321}});
    column floats1 =
        make_column<Ts...>("floats1", {field{1.23}, field{456.7}, field{0.8},
                                       field{910}, field{123456789.987654321}});
    column floats2 =
        make_column<Ts...>("floats2", {field{1.23}, field{456.7}, field{0.8},
                                       field{910}, field{123456789.987654321}});

    column strings0 =
        make_column<Ts...>("strings0", {field{"just"}, field{"some"},
                                        field{"random"}, field{"string"}});

    column strings1 =
        make_column<Ts...>("strings1", {field{"st\"rings"}, field{"w\"\"ith"},
                                        field{"qu\"otes\\"}, field{"\\a\\n\\d"},
                                        field{"escapes\""}});

    column strings2 =
        make_column<Ts...>("strings2",
                           {field{"  with  "}, field{"  spaces"},
                            field{"and  "}, field{"\nnew"}, field{"  \nlines"},
                            field{"  a\n\nn\n\nd  "}, field{" \nso\n  "},
                            field{"on"}});

    auto columns0 = std::vector{ints0, strings0, floats0, strings1, strings2};
    auto columns1 = std::vector{strings2, strings1, floats0, strings0, ints0};
    auto columns2 = std::vector{floats0, strings1, ints0, strings2, strings0};
    auto columns3 = std::vector{ints0, ints1, ints2};
    auto columns4 = std::vector{floats0, floats1, floats2};
    auto columns5 = std::vector{strings1, strings2};
    auto columns6 = std::vector{strings1};
    auto columns7 = std::vector{strings2};

    for (size_t i = 0; i < 3; ++i) {
        for (const auto& delimiter : {",", "-", "--"}) {
            for (const auto& columns :
                 {columns0, columns1, columns2, columns3, columns4, columns5,
                  columns6, columns7}) {
                test_combinations<Ts...>(columns, delimiter, false);
                test_combinations<Ts...>(columns, delimiter, true);
            }
        }
    }
}

TEST_CASE("parser test various cases version 2") {
    // TODO handle crlf
    using quote = ss::quote<'"'>;
    using escape = ss::escape<'\\'>;
    using trim = ss::trim<' '>;
    using triml = ss::trim_left<' '>;
    using trimr = ss::trim_right<' '>;
    using multiline = ss::multiline;

    test_combinations_impl<>();
    test_combinations_impl<trim>();
    test_combinations_impl<triml>();
    test_combinations_impl<trimr>();

    test_combinations_impl<escape>();
    test_combinations_impl<escape, trim>();
    test_combinations_impl<escape, triml>();
    test_combinations_impl<escape, trimr>();

    test_combinations_impl<quote>();
    test_combinations_impl<quote, trim>();
    test_combinations_impl<quote, triml>();
    test_combinations_impl<quote, trimr>();

    test_combinations_impl<escape, quote>();
    test_combinations_impl<escape, quote, trim>();
    test_combinations_impl<escape, quote, triml>();
    test_combinations_impl<escape, quote, trimr>();

    test_combinations_impl<escape, multiline>();
    test_combinations_impl<escape, multiline, trim>();
    test_combinations_impl<escape, multiline, triml>();
    test_combinations_impl<escape, multiline, trimr>();

    test_combinations_impl<quote, multiline>();
    test_combinations_impl<quote, multiline, trim>();
    test_combinations_impl<quote, multiline, triml>();
    test_combinations_impl<quote, multiline, trimr>();

    test_combinations_impl<quote, escape, multiline>();
    test_combinations_impl<quote, escape, multiline, trim>();
    test_combinations_impl<quote, escape, multiline, triml>();
    test_combinations_impl<quote, escape, multiline, trimr>();
}
