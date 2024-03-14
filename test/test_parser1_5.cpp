#include "test_parser1.hpp"

TEST_CASE_TEMPLATE("test empty fields header", T, ParserOptionCombinations) {
    constexpr auto buffer_mode = T::BufferMode::value;
    using ErrorMode = typename T::ErrorMode;

    unique_file_name f{"empty_fields_header"};

    // Empty header
    {
        std::ofstream out{f.name};
        out << "" << std::endl;
        out << "1" << std::endl;
    }

    {
        std::vector<std::string> expected_header = {""};
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name);
        CHECK_EQ_ARRAY(expected_header, p.header());
        CHECK_EQ("", p.raw_header());
        CHECK(p.valid());
    }

    // All empty header fields
    {
        std::ofstream out{f.name};
        out << ",," << std::endl;
        out << "1,2,3" << std::endl;
    }

    {
        std::vector<std::string> expected_header = {"", "", ""};
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name);
        CHECK_EQ_ARRAY(expected_header, p.header());
        CHECK_EQ(",,", p.raw_header());
        CHECK(p.valid());

        auto command1 = [&p = p] { std::ignore = p.field_exists("Int"); };
        expect_error_on_command(p, command1);

        auto command2 = [&p = p] { p.use_fields("Int"); };
        expect_error_on_command(p, command2);
    }

    // One empty field
    const std::vector<std::string> valid_fields = {"Int0", "Int1", ""};

    using svec = std::vector<std::string>;
    const std::vector<std::vector<std::string>> valid_field_combinations =
        {svec{"Int0"},
         svec{"Int1"},
         svec{""},
         svec{"", "Int0"},
         svec{"Int0", "Int1"},
         svec{"Int1", ""},
         svec{"Int0", "", "Int1"},
         svec{"", "Int1", "Int0"}};

    // Last header field empty
    {
        std::ofstream out{f.name};
        out << "Int0,Int1," << std::endl;
        out << "1,2,3" << std::endl;
    }

    {
        std::vector<std::string> expected_header = {"Int0", "Int1", ""};
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name);
        CHECK_EQ_ARRAY(expected_header, p.header());
        CHECK_EQ("Int0,Int1,", p.raw_header());
        CHECK(p.valid());

        for (const auto& field : valid_fields) {
            CHECK(p.field_exists(field));
            CHECK(p.valid());
        }

        for (const auto& fields : valid_field_combinations) {
            p.use_fields(fields);
            CHECK(p.valid());
        }
    }

    // First header field empty
    {
        std::ofstream out{f.name};
        out << ",Int0,Int1" << std::endl;
        out << "1,2,3" << std::endl;
    }

    {
        std::vector<std::string> expected_header = {"", "Int0", "Int1"};
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name);
        CHECK_EQ_ARRAY(expected_header, p.header());
        CHECK_EQ(",Int0,Int1", p.raw_header());
        CHECK(p.valid());

        for (const auto& field : valid_fields) {
            CHECK(p.field_exists(field));
            CHECK(p.valid());
        }

        for (const auto& fields : valid_field_combinations) {
            p.use_fields(fields);
            CHECK(p.valid());
        }
    }

    // Middle header field empty
    {
        std::ofstream out{f.name};
        out << "Int0,,Int1" << std::endl;
        out << "1,2,3" << std::endl;
    }

    {
        std::vector<std::string> expected_header = {"Int0", "", "Int1"};
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name);
        CHECK_EQ_ARRAY(expected_header, p.header());
        CHECK_EQ("Int0,,Int1", p.raw_header());
        CHECK(p.valid());

        for (const auto& field : valid_fields) {
            CHECK(p.field_exists(field));
            CHECK(p.valid());
        }

        for (const auto& fields : valid_field_combinations) {
            p.use_fields(fields);
            CHECK(p.valid());
        }
    }
}

template <typename T, typename... Ts>
void test_unterminated_quote_header() {
    constexpr auto buffer_mode = T::BufferMode::value;
    using ErrorMode = typename T::ErrorMode;

    unique_file_name f{"unterminated_quote_header"};

    {
        std::ofstream out{f.name};
        out << "\"Int" << std::endl;
        out << "1" << std::endl;
    }

    {
        auto [p, _] = make_parser<buffer_mode, ErrorMode, Ts...>(f.name);

        auto command0 = [&p = p] { std::ignore = p.header(); };
        expect_error_on_command(p, command0);
        CHECK_EQ(p.raw_header(), "\"Int");

        auto command1 = [&p = p] { std::ignore = p.field_exists("Int"); };
        expect_error_on_command(p, command1);

        auto command2 = [&p = p] { p.use_fields("Int"); };
        expect_error_on_command(p, command2);
    }
}

TEST_CASE_TEMPLATE("test unterminated quote header", T,
                   ParserOptionCombinations) {
    using quote = ss::quote<'"'>;
    using escape = ss::escape<'\\'>;
    test_unterminated_quote_header<T, quote>();
    test_unterminated_quote_header<T, quote, ss::multiline>();
    test_unterminated_quote_header<T, quote, escape>();
    test_unterminated_quote_header<T, quote, escape, ss::multiline>();
}

template <typename T, typename... Ts>
void test_unterminated_escape_header() {
    constexpr auto buffer_mode = T::BufferMode::value;
    using ErrorMode = typename T::ErrorMode;

    unique_file_name f{"unterminated_escape_header"};

    // Unterminated escape in header
    {
        std::ofstream out{f.name};
        out << "Int\\" << std::endl;
        out << "1" << std::endl;
    }

    {
        auto [p, _] = make_parser<buffer_mode, ErrorMode, Ts...>(f.name);

        auto command0 = [&p = p] { std::ignore = p.header(); };
        expect_error_on_command(p, command0);
        CHECK_EQ(p.raw_header(), "Int\\");

        auto command1 = [&p = p] { std::ignore = p.field_exists("Int"); };
        expect_error_on_command(p, command1);

        auto command2 = [&p = p] { p.use_fields("Int"); };
        expect_error_on_command(p, command2);
    }
}

TEST_CASE_TEMPLATE("test unterminated escape header", T,
                   ParserOptionCombinations) {
    using quote = ss::quote<'"'>;
    using escape = ss::escape<'\\'>;
    test_unterminated_escape_header<T, escape>();
    test_unterminated_escape_header<T, escape, ss::multiline>();
    test_unterminated_escape_header<T, escape, quote>();
    test_unterminated_escape_header<T, escape, quote, ss::multiline>();
}

template <typename T>
void test_ignore_empty(const std::vector<X>& data) {
    constexpr auto buffer_mode = T::BufferMode::value;
    using ErrorMode = typename T::ErrorMode;

    unique_file_name f{"ignore_empty"};
    make_and_write(f.name, data);

    std::vector<X> expected;
    for (const auto& d : data) {
        if (d.s != X::empty) {
            expected.push_back(d);
        }
    }

    {
        auto [p, _] =
            make_parser<buffer_mode, ErrorMode, ss::ignore_empty>(f.name, ",");

        std::vector<X> i;
        for (const auto& a : p.template iterate<X>()) {
            i.push_back(a);
        }

        CHECK_EQ(i, expected);
    }

    {
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        std::vector<X> i;
        size_t n = 0;
        while (!p.eof()) {
            try {
                ++n;
                const auto& a = p.template get_next<X>();
                if (data.at(n - 1).s == X::empty) {
                    CHECK_FALSE(p.valid());
                    continue;
                }
                i.push_back(a);
            } catch (...) {
                CHECK_EQ(data.at(n - 1).s, X::empty);
            }
        }

        CHECK_EQ(i, expected);
    }
}

TEST_CASE_TEMPLATE("test various cases with empty lines", T,
                   ParserOptionCombinations) {
    test_ignore_empty<T>(
        {{1, 2, "x"}, {3, 4, "y"}, {9, 10, "v"}, {11, 12, "w"}});

    test_ignore_empty<T>(
        {{1, 2, X::empty}, {3, 4, "y"}, {9, 10, "v"}, {11, 12, "w"}});

    test_ignore_empty<T>(
        {{1, 2, "x"}, {3, 4, "y"}, {9, 10, "v"}, {11, 12, X::empty}});

    test_ignore_empty<T>(
        {{1, 2, "x"}, {5, 6, X::empty}, {9, 10, "v"}, {11, 12, "w"}});

    test_ignore_empty<T>(
        {{1, 2, X::empty}, {5, 6, X::empty}, {9, 10, "v"}, {11, 12, "w"}});

    test_ignore_empty<T>(
        {{1, 2, X::empty}, {3, 4, "y"}, {9, 10, "v"}, {11, 12, X::empty}});

    test_ignore_empty<T>(
        {{1, 2, "x"}, {3, 4, "y"}, {9, 10, X::empty}, {11, 12, X::empty}});

    test_ignore_empty<T>(
        {{1, 2, X::empty}, {3, 4, "y"}, {9, 10, X::empty}, {11, 12, X::empty}});

    test_ignore_empty<T>({{1, 2, X::empty},
                          {3, 4, X::empty},
                          {9, 10, X::empty},
                          {11, 12, X::empty}});

    test_ignore_empty<T>(
        {{1, 2, "x"}, {3, 4, X::empty}, {9, 10, X::empty}, {11, 12, X::empty}});

    test_ignore_empty<T>(
        {{1, 2, X::empty}, {3, 4, X::empty}, {9, 10, X::empty}, {11, 12, "w"}});

    test_ignore_empty<T>({{11, 12, X::empty}});

    test_ignore_empty<T>({});
}
