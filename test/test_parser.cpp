#include <ss/parser.hpp>
#include <doctest/doctest.h>
#include <algorithm>
#include <filesystem>

struct unique_file_name {
    const std::string name;

    unique_file_name() : name{std::tmpnam(nullptr)} {
    }

    ~unique_file_name() {
        std::filesystem::remove(name);
    }
};

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
                           const std::vector<T>& data) {
    std::ofstream out{file_name};
    std::vector<const char*> new_lines = {"\n", "\r\n"};
    for (size_t i = 0; i < data.size(); ++i) {
        out << data[i].to_string() << new_lines[i % new_lines.size()];
    }
}

TEST_CASE("testing parser") {
    unique_file_name f;
    std::vector<X> data = {{1, 2, "x"}, {3, 4, "y"},  {5, 6, "z"},
                           {7, 8, "u"}, {9, 10, "v"}, {11, 12, "w"}};
    make_and_write(f.name, data);
    {
        ss::parser p{f.name, ","};
        std::vector<X> i;

        while (!p.eof()) {
            auto a = p.get_next<int, double, std::string>();
            i.emplace_back(ss::to_object<X>(a));
        }

        CHECK(std::equal(i.begin(), i.end(), data.begin()));
    }

    {
        ss::parser p{f.name, ","};
        std::vector<X> i;

        p.ignore_next();
        while (!p.eof()) {
            using tup = std::tuple<int, double, std::string>;
            auto a = p.get_next<tup>();
            i.emplace_back(ss::to_object<X>(a));
        }

        CHECK(std::equal(i.begin(), i.end(), data.begin() + 1));
    }

    {
        ss::parser p{f.name, ","};
        std::vector<X> i;

        while (!p.eof()) {
            i.push_back(p.get_object<X, int, double, std::string>());
        }

        CHECK(std::equal(i.begin(), i.end(), data.begin()));
    }

    {
        ss::parser p{f.name, ","};
        std::vector<X> i;

        while (!p.eof()) {
            using tup = std::tuple<int, double, std::string>;
            i.push_back(p.get_object<X, tup>());
        }

        CHECK(std::equal(i.begin(), i.end(), data.begin()));
    }

    {
        ss::parser p{f.name, ","};
        std::vector<X> i;

        while (!p.eof()) {
            i.push_back(p.get_next<X>());
        }

        CHECK(std::equal(i.begin(), i.end(), data.begin()));
    }

    {
        constexpr int excluded = 3;
        ss::parser p{f.name, ","};
        std::vector<X> i;

        while (!p.eof()) {
            auto a =
                p.get_object<X, ss::ax<int, excluded>, double, std::string>();
            if (p.valid()) {
                i.push_back(a);
            }
        }
        std::vector<X> expected = data;
        std::remove_if(expected.begin(), expected.end(),
                       [](const X& x) { return x.i == excluded; });
        CHECK(std::equal(i.begin(), i.end(), expected.begin()));
    }

    {
        ss::parser p{f.name, ","};
        std::vector<X> i;

        while (!p.eof()) {
            auto a = p.get_object<X, ss::nx<int, 3>, double, std::string>();
            if (p.valid()) {
                i.push_back(a);
            }
        }
        std::vector<X> expected = {{3, 4, "y"}};
        CHECK(std::equal(i.begin(), i.end(), expected.begin()));
    }

    {
        unique_file_name empty_f;
        std::vector<X> empty_data = {};
        make_and_write(empty_f.name, empty_data);

        ss::parser p{empty_f.name, ","};
        std::vector<X> i;

        while (!p.eof()) {
            i.push_back(p.get_next<X>());
        }
        CHECK(i.empty());
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
TEST_CASE("testing composite conversion") {
    unique_file_name f;
    {
        std::ofstream out{f.name};
        for (auto& i :
             {"10,a,11.1", "10,20,11.1", "junk", "10,11.1", "1,11.1,a", "junk",
              "10,junk", "11,junk", "10,11.1,c", "10,20", "10,22.2,f"}) {
            out << i << std::endl;
        }
    }

    ss::parser p{f.name, ","};
    p.set_error_mode(ss::error_mode::error_string);
    auto fail = [] { FAIL(""); };
    auto expect_error = [](auto error) { CHECK(!error.empty()); };

    REQUIRE(p.valid());
    REQUIRE(!p.eof());

    {
        constexpr static auto expectedData = std::tuple{10, 'a', 11.1};

        auto [d1, d2, d3, d4] =
            p.try_next<int, int, double>(fail)
                .or_else<test_struct>(fail)
                .or_else<int, char, double>(
                    [](auto&& data) { CHECK(data == expectedData); })
                .on_error(fail)
                .or_else<test_tuple>(fail)
                .values();

        REQUIRE(p.valid());
        REQUIRE(!d1);
        REQUIRE(!d2);
        REQUIRE(d3);
        REQUIRE(!d4);
        CHECK(*d3 == expectedData);
    }

    {
        REQUIRE(!p.eof());
        constexpr static auto expectedData = std::tuple{10, 20, 11.1};

        auto [d1, d2, d3, d4] =
            p.try_next<int, int, double>([](auto& i1, auto i2, double d) {
                 CHECK(std::tie(i1, i2, d) == expectedData);
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
        REQUIRE(!d2);
        REQUIRE(!d3);
        REQUIRE(!d4);
        CHECK(*d1 == expectedData);
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

        REQUIRE(!p.valid());
        REQUIRE(!d1);
        REQUIRE(!d2);
        REQUIRE(!d3);
        REQUIRE(!d4);
        REQUIRE(!d5);
    }

    {
        REQUIRE(!p.eof());

        auto [d1, d2] =
            p.try_next<int, double>([](auto& i, auto& d) {
                 REQUIRE(std::tie(i, d) == std::tuple{10, 11.1});
             })
                .or_else<int, double>([](auto&, auto&) { FAIL(""); })
                .values();

        REQUIRE(p.valid());
        REQUIRE(d1);
        REQUIRE(!d2);
    }

    {
        REQUIRE(!p.eof());

        auto [d1, d2] = p.try_next<int, double>([](auto&, auto&) { FAIL(""); })
                            .or_else<test_struct>(expect_test_struct)
                            .values();

        REQUIRE(p.valid());
        REQUIRE(!d1);
        REQUIRE(d2);
        CHECK(d2->tied() == std::tuple{1, 11.1, 'a'});
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

        REQUIRE(!p.valid());
        REQUIRE(!d1);
        REQUIRE(!d2);
        REQUIRE(!d3);
        REQUIRE(!d4);
        REQUIRE(!d5);
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
        REQUIRE(!d2);
        CHECK(*d1 == std::tuple{10, std::nullopt});
    }

    {
        REQUIRE(!p.eof());

        auto [d1, d2] = p.try_next<int, std::variant<int, std::string>>()
                            .on_error(fail)
                            .or_else<std::tuple<int, std::string>>(fail)
                            .on_error(fail)
                            .values();

        REQUIRE(p.valid());
        REQUIRE(d1);
        REQUIRE(!d2);
        CHECK(*d1 == std::tuple{11, std::variant<int, std::string>{"junk"}});
    }

    {
        REQUIRE(!p.eof());

        auto [d1, d2] = p.try_object<test_struct, int, double, char>()
                            .or_else<int>(fail)
                            .values();
        REQUIRE(p.valid());
        REQUIRE(d1);
        REQUIRE(!d2);
        CHECK(d1->tied() == std::tuple{10, 11.1, 'c'});
    }

    {
        REQUIRE(!p.eof());

        auto [d1, d2, d3, d4] =
            p.try_next<int, int>([] { return false; })
                .or_else<int, double>([](auto&) { return false; })
                .or_else<int, int>()
                .or_else<int, int>(fail)
                .values();

        REQUIRE(p.valid());
        REQUIRE(!d1);
        REQUIRE(!d2);
        REQUIRE(d3);
        REQUIRE(!d4);
        CHECK(d3.value() == std::tuple{10, 20});
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
        REQUIRE(!d1);
        REQUIRE(!d2);
        REQUIRE(d3);
        REQUIRE(!d4);
        CHECK(d3->tied() == std::tuple{10, 22.2, 'f'});
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

TEST_CASE("testing the moving of parsed values") {
    size_t move_called_one_col;

    {
        unique_file_name f;
        {
            std::ofstream out{f.name};
            out << "x" << std::endl;
        }

        ss::parser p{f.name, ","};
        auto x = p.get_next<my_string>();
        CHECK(move_called < 3);
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
        CHECK(move_called <= 3 * move_called_one_col);
        move_called = 0;
    }

    {
        ss::parser p{f.name, ","};
        auto x = p.get_object<xyz, my_string, my_string, my_string>();
        CHECK(move_called <= 6 * move_called_one_col);
        move_called = 0;
    }

    {
        ss::parser p{f.name, ","};
        auto x = p.get_next<xyz>();
        CHECK(move_called <= 6 * move_called_one_col);
        move_called = 0;
    }
}

TEST_CASE("testing the moving of parsed composite values") {
    // to compile is enough
    return;
    ss::parser* p;
    p->try_next<my_string, my_string, my_string>()
        .or_else<my_string, my_string, my_string, my_string>([](auto&&) {})
        .or_else<my_string>([](auto&) {})
        .or_else<xyz>([](auto&&) {})
        .or_object<xyz, my_string, my_string, my_string>([](auto&&) {})
        .or_else<std::tuple<my_string, my_string, my_string>>(
            [](auto&, auto&, auto&) {});
}

TEST_CASE("testing error mode") {
    unique_file_name f;
    {
        std::ofstream out{f.name};
        out << "junk" << std::endl;
        out << "junk" << std::endl;
    }

    ss::parser p(f.name, ",");

    REQUIRE(!p.eof());
    p.get_next<int>();
    CHECK(!p.valid());
    CHECK(p.error_msg().empty());

    p.set_error_mode(ss::error_mode::error_string);

    REQUIRE(!p.eof());
    p.get_next<int>();
    CHECK(!p.valid());
    CHECK(!p.error_msg().empty());
}
