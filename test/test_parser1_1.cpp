#include "test_parser1.hpp"

TEST_CASE("test file not found") {
    unique_file_name f{"file_not_found"};

    {
        ss::parser p{f.name, ","};
        CHECK_FALSE(p.valid());
    }

    {
        ss::parser<ss::string_error> p{f.name, ","};
        CHECK_FALSE(p.valid());
        CHECK_FALSE(p.error_msg().empty());
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
        CHECK_FALSE(p.error_msg().empty());
    }

    try {
        ss::parser<ss::throw_on_error> p{nullptr, 10, ","};
        FAIL("Expected exception...");
    } catch (const std::exception& e) {
        CHECK_FALSE(std::string{e.what()}.empty());
    }
}

struct Y {
    constexpr static auto delim = ",";
    std::string s1;
    std::string s2;
    std::string s3;

    std::string to_string() const {
        return std::string{}
            .append(s1)
            .append(delim)
            .append(s2)
            .append(delim)
            .append(s3);
    }

    auto tied() const {
        return std::tie(s1, s2, s3);
    }
};

TEST_CASE_TEMPLATE("test position method", T, ParserOptionCombinations) {
    constexpr auto buffer_mode = T::BufferMode::value;
    using ErrorMode = typename T::ErrorMode;

    unique_file_name f{"position_method"};
    std::vector<Y> data = {{"1", "21", "x"},   {"321", "4", "y"},
                           {"54", "6", "zz"},  {"7", "876", "uuuu"},
                           {"910", "10", "v"}, {"10", "321", "ww"}};
    make_and_write(f.name, data);

    auto [p, buff] = make_parser<buffer_mode, ErrorMode>(f.name);
    auto data_at = [&buff = buff, &f = f](auto n) {
        if (!buff.empty()) {
            return buff[n];
        } else {
            auto file = fopen(f.name.c_str(), "r");
            fseek(file, n, SEEK_SET);
            return static_cast<char>(fgetc(file));
        }
    };

    while (!p.eof()) {
        auto curr_char = p.position();
        const auto& [s1, s2, s3] =
            p.template get_next<std::string, std::string, std::string>();

        auto s = s1 + "," + s2 + "," + s3;

        for (size_t i = 0; i < s1.size(); ++i) {
            CHECK_EQ(data_at(curr_char + i), s[i]);
        }

        auto last_char = data_at(curr_char + s.size());
        CHECK((last_char == '\n' || last_char == '\r'));
    }
}

TEST_CASE_TEMPLATE("test line method", T, ParserOptionCombinations) {
    constexpr auto buffer_mode = T::BufferMode::value;
    using ErrorMode = typename T::ErrorMode;

    unique_file_name f{"line_method"};
    std::vector<Y> data = {{"1", "21", "x"},   {"321", "4", "y"},
                           {"54", "6", "zz"},  {"7", "876", "uuuu"},
                           {"910", "10", "v"}, {"10", "321", "ww"}};
    make_and_write(f.name, data);

    auto [p, buff] = make_parser<buffer_mode, ErrorMode>(f.name);

    auto expected_line = 0;
    CHECK_EQ(p.line(), expected_line);

    while (!p.eof()) {
        auto _ = p.template get_next<std::string, std::string, std::string>();
        ++expected_line;
        CHECK_EQ(p.line(), expected_line);
    }

    CHECK_EQ(p.line(), data.size());
}

TEST_CASE_TEMPLATE("parser test various valid cases", T,
                   ParserOptionCombinations) {
    constexpr auto buffer_mode = T::BufferMode::value;
    using ErrorMode = typename T::ErrorMode;

    unique_file_name f{"various_valid_cases"};
    std::vector<X> data = {{1, 2, "x"}, {3, 4, "y"},  {5, 6, "z"},
                           {7, 8, "u"}, {9, 10, "v"}, {11, 12, "w"}};
    make_and_write(f.name, data);
    auto csv_data_buffer = make_buffer(f.name);
    {
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        ss::parser p0{std::move(p)};
        p = std::move(p0);
        std::vector<X> i;

        auto [p2, __] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
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
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        std::vector<X> i;

        auto [p2, __] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        std::vector<X> i2;

        auto [p3, ___] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
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
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        std::vector<X> i;
        auto [p2, __] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
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
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        std::vector<X> i;

        for (auto&& a :
             p.template iterate_object<X, int, double, std::string>()) {
            i.push_back(std::move(a));
        }

        CHECK_EQ(i, data);
    }

    {
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        std::vector<X> i;

        auto [p2, __] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
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
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        std::vector<X> i;

        using tup = std::tuple<int, double, std::string>;
        for (auto&& a : p.template iterate_object<X, tup>()) {
            i.push_back(std::move(a));
        }

        CHECK_EQ(i, data);
    }

    {
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        std::vector<X> i;

        while (!p.eof()) {
            i.push_back(p.template get_next<X>());
        }

        CHECK_EQ(i, data);
    }

    {
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        std::vector<X> i;

        for (auto&& a : p.template iterate<X>()) {
            i.push_back(std::move(a));
        }

        CHECK_EQ(i, data);
    }

    {
        constexpr int excluded = 3;
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        std::vector<X> i;

        auto [p2, __] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
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

        if (!T::ThrowOnError) {
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

        if (!T::ThrowOnError) {
            CHECK_EQ(i2, expected);
        }
    }

    {
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        std::vector<X> i;

        auto [p2, __] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
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

        if (!T::ThrowOnError) {
            for (auto&& a : p2.template iterate_object<X, ss::nx<int, 3>,
                                                       double, std::string>()) {
                if (p2.valid()) {
                    i2.push_back(std::move(a));
                }
            }
        }

        std::vector<X> expected = {{3, 4, "y"}};
        CHECK_EQ(i, expected);
        if (!T::ThrowOnError) {
            CHECK_EQ(i2, expected);
        }
    }

    {
        unique_file_name empty_f{"various_valid_cases"};
        std::vector<X> empty_data = {};

        make_and_write(empty_f.name, empty_data);

        auto [p, _] = make_parser<buffer_mode, ErrorMode>(empty_f.name, ",");
        std::vector<X> i;

        auto [p2, __] = make_parser<buffer_mode, ErrorMode>(empty_f.name, ",");
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

TEST_CASE_TEMPLATE("parser test composite conversion", BufferMode,
                   std::true_type, std::false_type) {
    constexpr auto buffer_mode = BufferMode::value;
    unique_file_name f{"composite_conversion"};
    {
        std::ofstream out{f.name};
        for (auto& i :
             {"10,a,11.1", "10,20,11.1", "junk", "10,11.1", "1,11.1,a", "junk",
              "10,junk", "11,junk", "10,11.1,c", "10,20", "10,22.2,f"}) {
            out << i << std::endl;
        }
    }

    auto [p, _] = make_parser<buffer_mode, ss::string_error>(f.name, ",");
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

template <bool buffer_mode, typename... Ts>
void test_no_new_line_at_eof_impl(const std::vector<X>& data) {
    unique_file_name f{"no_new_line_at_eof"};
    make_and_write(f.name, data, {}, false);

    auto [p, _] = make_parser<buffer_mode, Ts...>(f.name);
    std::vector<X> parsed_data;

    for (const auto& el : p.template iterate<X>()) {
        parsed_data.push_back(el);
    }

    CHECK_EQ(data, parsed_data);
}

template <bool buffer_mode, typename... Ts>
void test_no_new_line_at_eof() {
    test_no_new_line_at_eof_impl<buffer_mode, Ts...>({});
    test_no_new_line_at_eof_impl<buffer_mode, Ts...>({{1, 2, "X"}});
    test_no_new_line_at_eof_impl<buffer_mode, Ts...>({{1, 2, "X"}, {}});
    test_no_new_line_at_eof_impl<buffer_mode, Ts...>(
        {{1, 2, "X"}, {3, 4, "YY"}});
    test_no_new_line_at_eof_impl<buffer_mode, Ts...>(
        {{1, 2, "X"}, {3, 4, "YY"}, {}});
    test_no_new_line_at_eof_impl<buffer_mode, Ts...>(
        {{1, 2, "X"}, {3, 4, "YY"}, {5, 6, "ZZZ"}, {7, 8, "UUU"}});

    for (size_t i = 0; i < 2 * ss::get_line_initial_buffer_size; ++i) {
        test_no_new_line_at_eof_impl<buffer_mode, Ts...>(
            {{1, 2, std::string(i, 'X')}});

        for (size_t j = 0; j < 2 * ss::get_line_initial_buffer_size; j += 13) {

            test_no_new_line_at_eof_impl<buffer_mode, Ts...>(
                {{1, 2, std::string(i, 'X')}, {3, 4, std::string(j, 'Y')}});

            test_no_new_line_at_eof_impl<buffer_mode, Ts...>(
                {{1, 2, std::string(j, 'X')}, {3, 4, std::string(i, 'Y')}});
        }
    }
}

TEST_CASE_TEMPLATE("test no new line at end of data", T,
                   ParserOptionCombinations) {
    constexpr auto buffer_mode = T::BufferMode::value;
    using ErrorMode = typename T::ErrorMode;
    test_no_new_line_at_eof<buffer_mode, ErrorMode>();
}
