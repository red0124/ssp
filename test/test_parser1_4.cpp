#include "test_parser1.hpp"

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
        auto command = [&] { p.use_fields(fields.at(0), fields.at(0)); };
        if (!fields.empty()) {
            expect_error_on_command(p, command);
        }
    }

    {
        // Mapping out of range
        ss::parser<Ts...> p{f.name, ","};
        auto command = [&] {
            p.use_fields(fields.at(0));
            p.template get_next<std::string, std::string>();
        };
        if (!fields.empty()) {
            expect_error_on_command(p, command);
        }
    }

    {
        // Invalid header
        ss::parser<Ts...> p{f.name, ","};
        auto command = [&] { p.use_fields(fields); };

        if (!fields.empty()) {
            // Pass if there are no duplicates, fail otherwise
            if (std::unordered_set<std::string>{fields.begin(), fields.end()}
                    .size() != fields.size()) {
                expect_error_on_command(p, command);
            } else {
                command();
                CHECK(p.valid());
                if (!p.valid()) {
                    if constexpr (ss::setup<Ts...>::string_error) {
                        std::cout << p.error_msg() << std::endl;
                    }
                }
            }
        }
    }
}

template <typename... Ts>
void test_invalid_fields(const std::vector<std::string>& lines,
                         const std::vector<std::string>& fields) {
    test_invalid_fields_impl(lines, fields);
    test_invalid_fields_impl<ss::string_error>(lines, fields);
    test_invalid_fields_impl<ss::throw_on_error>(lines, fields);
}

TEST_CASE("parser test invalid header fields usage") {
    test_invalid_fields({}, {});

    test_invalid_fields({"Int"}, {"Int"});
    test_invalid_fields({"Int", "1"}, {"Int"});
    test_invalid_fields({"Int", "1", "2"}, {"Int"});

    test_invalid_fields({"Int,String"}, {"Int", "String"});
    test_invalid_fields({"Int,String", "1,hi"}, {"Int", "String"});
    test_invalid_fields({"Int,String", "2,hello"}, {"Int", "String"});

    test_invalid_fields({"Int,String,Double"}, {"Int", "String", "Double"});
    test_invalid_fields({"Int,String,Double", "1,hi,2.34"},
                        {"Int", "String", "Double"});
    test_invalid_fields({"Int,String,Double", "1,hi,2.34", "2,hello,3.45"},
                        {"Int", "String", "Double"});

    test_invalid_fields({"Int,Int,Int"}, {"Int", "Int", "Int"});
    test_invalid_fields({"Int,Int,Int", "1,2,3"}, {"Int", "Int", "Int"});

    test_invalid_fields({"Int,String,Int"}, {"Int", "String", "Int"});
    test_invalid_fields({"Int,String,Int", "1,hi,3"}, {"Int", "String", "Int"});
}

template <typename... Ts>
void test_invalid_rows_with_header() {
    unique_file_name f{"test_parser"};
    {
        std::ofstream out{f.name};
        out << "Int,String,Double" << std::endl;
        out << "1,line1,2.34" << std::endl;
        out << "2,line2" << std::endl;
        out << "3,line3,67.8" << std::endl;
        out << "4,line4,67.8,9" << std::endl;
        out << "5,line5,9.10" << std::endl;
        out << "six,line6,10.11" << std::endl;
    }

    {
        ss::parser<Ts...> p{f.name};

        p.use_fields("Int", "String", "Double");
        using data = std::tuple<int, std::string, double>;
        std::vector<data> i;

        CHECK(p.valid());

        while (!p.eof()) {
            try {
                const auto& t = p.template get_next<data>();
                if (p.valid()) {
                    i.push_back(t);
                }
            } catch (const ss::exception&) {
                continue;
            }
        }

        std::vector<data> expected = {{1, "line1", 2.34},
                                      {3, "line3", 67.8},
                                      {5, "line5", 9.10}};
        CHECK_EQ(i, expected);
    }

    {
        ss::parser<Ts...> p{f.name};

        p.use_fields("Double", "Int");
        using data = std::tuple<double, int>;
        std::vector<data> i;

        CHECK(p.valid());

        while (!p.eof()) {
            try {
                const auto& t = p.template get_next<data>();
                if (p.valid()) {
                    i.push_back(t);
                }
            } catch (const ss::exception&) {
                continue;
            }
        }

        std::vector<data> expected = {{2.34, 1}, {67.8, 3}, {9.10, 5}};
        CHECK_EQ(i, expected);
    }

    {
        ss::parser<Ts...> p{f.name};

        p.use_fields("String", "Double");
        using data = std::tuple<std::string, double>;
        std::vector<data> i;

        CHECK(p.valid());

        while (!p.eof()) {
            try {
                const auto& t = p.template get_next<data>();
                if (p.valid()) {
                    i.push_back(t);
                }
            } catch (const ss::exception&) {
                continue;
            }
        }

        std::vector<data> expected = {{"line1", 2.34},
                                      {"line3", 67.8},
                                      {"line5", 9.10},
                                      {"line6", 10.11}};
        CHECK_EQ(i, expected);
    }
}

TEST_CASE("parser test invalid rows with header") {
    test_invalid_rows_with_header();
    test_invalid_rows_with_header<ss::string_error>();
    test_invalid_rows_with_header<ss::throw_on_error>();
}

template <typename... Ts>
void test_ignore_empty_impl(const std::vector<X>& data) {
    unique_file_name f{"test_parser"};
    make_and_write(f.name, data);

    std::vector<X> expected;
    for (const auto& d : data) {
        if (d.s != X::empty) {
            expected.push_back(d);
        }
    }

    {
        ss::parser<ss::ignore_empty, Ts...> p{f.name, ","};

        std::vector<X> i;
        for (const auto& a : p.template iterate<X>()) {
            i.push_back(a);
        }

        CHECK_EQ(i, expected);
    }

    {
        ss::parser<Ts...> p{f.name, ","};
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

template <typename... Ts>
void test_ignore_empty(const std::vector<X>& data) {
    test_ignore_empty_impl(data);
    test_ignore_empty_impl<ss::string_error>(data);
    test_ignore_empty_impl<ss::throw_on_error>(data);
}

TEST_CASE("parser test various cases with empty lines") {
    test_ignore_empty({{1, 2, "x"}, {3, 4, "y"}, {9, 10, "v"}, {11, 12, "w"}});

    test_ignore_empty(
        {{1, 2, X::empty}, {3, 4, "y"}, {9, 10, "v"}, {11, 12, "w"}});

    test_ignore_empty(
        {{1, 2, "x"}, {3, 4, "y"}, {9, 10, "v"}, {11, 12, X::empty}});

    test_ignore_empty(
        {{1, 2, "x"}, {5, 6, X::empty}, {9, 10, "v"}, {11, 12, "w"}});

    test_ignore_empty(
        {{1, 2, X::empty}, {5, 6, X::empty}, {9, 10, "v"}, {11, 12, "w"}});

    test_ignore_empty(
        {{1, 2, X::empty}, {3, 4, "y"}, {9, 10, "v"}, {11, 12, X::empty}});

    test_ignore_empty(
        {{1, 2, "x"}, {3, 4, "y"}, {9, 10, X::empty}, {11, 12, X::empty}});

    test_ignore_empty(
        {{1, 2, X::empty}, {3, 4, "y"}, {9, 10, X::empty}, {11, 12, X::empty}});

    test_ignore_empty({{1, 2, X::empty},
                       {3, 4, X::empty},
                       {9, 10, X::empty},
                       {11, 12, X::empty}});

    test_ignore_empty(
        {{1, 2, "x"}, {3, 4, X::empty}, {9, 10, X::empty}, {11, 12, X::empty}});

    test_ignore_empty(
        {{1, 2, X::empty}, {3, 4, X::empty}, {9, 10, X::empty}, {11, 12, "w"}});

    test_ignore_empty({{11, 12, X::empty}});

    test_ignore_empty({});
}
