#include "test_parser1.hpp"

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

TEST_CASE("test null buffer") {
    {
        ss::parser p{nullptr, 10, ","};
        CHECK_FALSE(p.valid());
    }

    {
        ss::parser<ss::string_error> p{nullptr, 10, ","};
        CHECK_FALSE(p.valid());
    }

    try {
        ss::parser<ss::throw_on_error> p{nullptr, 10, ","};
        FAIL("Expected exception...");
    } catch (const std::exception& e) {
        CHECK_FALSE(std::string{e.what()}.empty());
    }
}

template <bool buffer_mode, typename... Ts>
void test_various_cases() {
    unique_file_name f{"test_parser"};
    std::vector<X> data = {{1, 2, "x"}, {3, 4, "y"},  {5, 6, "z"},
                           {7, 8, "u"}, {9, 10, "v"}, {11, 12, "w"}};
    make_and_write(f.name, data);
    auto csv_data_buffer = make_buffer(f.name);
    {
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
        ss::parser p0{std::move(p)};
        p = std::move(p0);
        std::vector<X> i;

        auto [p2, __] = make_parser<buffer_mode, Ts...>(f.name, ",");
        std::vector<X> i2;

        auto move_rotate = [&p = p, &p0 = p0] {
            auto p1 = std::move(p);
            p0 = std::move(p1);
            p = std::move(p0);
        };

        while (!p.eof()) {
            move_rotate();
            auto a = p.template get_next<int, double, std::string>();
            i.emplace_back(ss::to_object<X>(a));
        }

        for (const auto& a : p2.template iterate<int, double, std::string>()) {
            i2.emplace_back(ss::to_object<X>(a));
        }

        CHECK_EQ(i, data);
        CHECK_EQ(i2, data);
    }

    {
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
        std::vector<X> i;

        auto [p2, __] = make_parser<buffer_mode, Ts...>(f.name, ",");
        std::vector<X> i2;

        auto [p3, ___] = make_parser<buffer_mode, Ts...>(f.name, ",");
        std::vector<X> i3;

        std::vector<X> expected = {std::begin(data) + 1, std::end(data)};
        using tup = std::tuple<int, double, std::string>;

        p.ignore_next();
        while (!p.eof()) {
            auto a = p.template get_next<tup>();
            i.emplace_back(ss::to_object<X>(a));
        }

        p2.ignore_next();
        for (const auto& a : p2.template iterate<tup>()) {
            i2.emplace_back(ss::to_object<X>(a));
        }

        p3.ignore_next();
        for (auto it = p3.template iterate<tup>().begin();
             it != p3.template iterate<tup>().end(); ++it) {
            i3.emplace_back(ss::to_object<X>(*it));
        }

        CHECK_EQ(i, expected);
        CHECK_EQ(i2, expected);
        CHECK_EQ(i3, expected);
    }

    {
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
        std::vector<X> i;
        auto [p2, __] = make_parser<buffer_mode, Ts...>(f.name, ",");
        std::vector<X> i2;

        while (!p.eof()) {
            i.push_back(p.template get_object<X, int, double, std::string>());
        }

        for (auto&& a :
             p2.template iterate_object<X, int, double, std::string>()) {
            i2.push_back(std::move(a));
        }

        CHECK_EQ(i, data);
        CHECK_EQ(i2, data);
    }

    {
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
        std::vector<X> i;

        for (auto&& a :
             p.template iterate_object<X, int, double, std::string>()) {
            i.push_back(std::move(a));
        }

        CHECK_EQ(i, data);
    }

    {
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
        std::vector<X> i;

        auto [p2, __] = make_parser<buffer_mode, Ts...>(f.name, ",");
        std::vector<X> i2;

        using tup = std::tuple<int, double, std::string>;
        while (!p.eof()) {
            i.push_back(p.template get_object<X, tup>());
        }

        for (auto it = p2.template iterate_object<X, tup>().begin();
             it != p2.template iterate_object<X, tup>().end(); it++) {
            i2.push_back({it->i, it->d, it->s});
        }

        CHECK_EQ(i, data);
        CHECK_EQ(i2, data);
    }

    {
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
        std::vector<X> i;

        using tup = std::tuple<int, double, std::string>;
        for (auto&& a : p.template iterate_object<X, tup>()) {
            i.push_back(std::move(a));
        }

        CHECK_EQ(i, data);
    }

    {
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
        std::vector<X> i;

        while (!p.eof()) {
            i.push_back(p.template get_next<X>());
        }

        CHECK_EQ(i, data);
    }

    {
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
        std::vector<X> i;

        for (auto&& a : p.template iterate<X>()) {
            i.push_back(std::move(a));
        }

        CHECK_EQ(i, data);
    }

    {
        constexpr int excluded = 3;
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
        std::vector<X> i;

        auto [p2, __] = make_parser<buffer_mode, Ts...>(f.name, ",");
        std::vector<X> i2;

        while (!p.eof()) {
            try {
                auto a = p.template get_object<X, ss::ax<int, excluded>, double,
                                               std::string>();
                if (p.valid()) {
                    i.push_back(a);
                }
            } catch (...) {
                // ignore
            };
        }

        if (!ss::setup<Ts...>::throw_on_error) {
            for (auto&& a : p2.template iterate_object<X, ss::ax<int, excluded>,
                                                       double, std::string>()) {
                if (p2.valid()) {
                    i2.push_back(std::move(a));
                }
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

        if (!ss::setup<Ts...>::throw_on_error) {
            CHECK_EQ(i2, expected);
        }
    }

    {
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
        std::vector<X> i;

        auto [p2, __] = make_parser<buffer_mode, Ts...>(f.name, ",");
        std::vector<X> i2;

        while (!p.eof()) {
            try {
                auto a = p.template get_object<X, ss::nx<int, 3>, double,
                                               std::string>();
                if (p.valid()) {
                    i.push_back(a);
                }
            } catch (...) {
                // ignore
            }
        }

        if (!ss::setup<Ts...>::throw_on_error) {
            for (auto&& a : p2.template iterate_object<X, ss::nx<int, 3>,
                                                       double, std::string>()) {
                if (p2.valid()) {
                    i2.push_back(std::move(a));
                }
            }
        }

        std::vector<X> expected = {{3, 4, "y"}};
        CHECK_EQ(i, expected);
        if (!ss::setup<Ts...>::throw_on_error) {
            CHECK_EQ(i2, expected);
        }
    }

    {
        unique_file_name empty_f{"test_parser"};
        std::vector<X> empty_data = {};

        make_and_write(empty_f.name, empty_data);

        auto [p, _] = make_parser<buffer_mode, Ts...>(empty_f.name, ",");
        std::vector<X> i;

        auto [p2, __] = make_parser<buffer_mode, Ts...>(empty_f.name, ",");
        std::vector<X> i2;

        while (!p.eof()) {
            i.push_back(p.template get_next<X>());
        }

        for (auto&& a : p2.template iterate<X>()) {
            i2.push_back(std::move(a));
        }

        CHECK(i.empty());
        CHECK(i2.empty());
    }
}

TEST_CASE("parser test various cases") {
    test_various_cases<false>();
    test_various_cases<false, ss::string_error>();
    test_various_cases<false, ss::throw_on_error>();
    test_various_cases<true>();
    test_various_cases<true, ss::string_error>();
    test_various_cases<true, ss::throw_on_error>();
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

template <bool buffer_mode, typename... Ts>
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

    auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
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
    test_composite_conversion<false, ss::string_error>();
    test_composite_conversion<true, ss::string_error>();
}
