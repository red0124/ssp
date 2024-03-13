#include "test_parser1.hpp"

template <typename T, typename Tuple>
struct has_type;

template <typename T, typename... Us>
struct has_type<T, std::tuple<Us...>>
    : std::disjunction<std::is_same<T, Us>...> {};

template <typename T, typename... Ts>
static void test_fields(const std::string file_name, const std::vector<X>& data,
                        const std::vector<std::string>& header,
                        const std::vector<std::string>& fields) {
    constexpr auto buffer_mode = T::BufferMode::value;
    using ErrorMode = typename T::ErrorMode;
    using CaseType = std::tuple<Ts...>;

    auto [p, _] = make_parser<buffer_mode, ErrorMode>(file_name, ",");
    CHECK_FALSE(p.field_exists("Unknown"));
    p.use_fields(fields);

    CHECK_EQ_ARRAY(header, p.header());
    CHECK_EQ(merge_header(p.header(), ","), p.raw_header());
    std::vector<CaseType> i;

    for (const auto& a : p.template iterate<CaseType>()) {
        CHECK_EQ_ARRAY(header, p.header());
        CHECK_EQ(merge_header(p.header(), ","), p.raw_header());
        i.push_back(a);
    }

    CHECK_EQ(i.size(), data.size());
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

TEST_CASE_TEMPLATE("test various cases with header", T,
                   ParserOptionCombinations) {
    unique_file_name f{"various_cases_with_header"};
    using str = std::string;

    constexpr static auto Int = "Int";
    constexpr static auto Dbl = "Double";
    constexpr static auto Str = "String";
    const std::vector<std::string> header{Int, Dbl, Str};

    std::vector<X> data = {{1, 2, "x"}, {3, 4, "y"},  {5, 6, "z"},
                           {7, 8, "u"}, {9, 10, "v"}, {11, 12, "w"}};

    make_and_write(f.name, data, header);
    const auto& o = f.name;
    const auto& d = data;

    {
        ss::parser<ss::string_error> p{f.name, ","};
        std::vector<X> i;

        for (const auto& a : p.iterate<int, double, std::string>()) {
            CHECK_EQ(header, p.header());
            CHECK_EQ(merge_header(p.header(), ","), p.raw_header());
            i.emplace_back(ss::to_object<X>(a));
        }

        CHECK_NE(i, data);
    }

    {
        ss::parser<ss::string_error> p{f.name, ","};
        std::vector<X> i;

        p.ignore_next();
        for (const auto& a : p.iterate<int, double, std::string>()) {
            CHECK_EQ(header, p.header());
            CHECK_EQ(merge_header(p.header(), ","), p.raw_header());
            i.emplace_back(ss::to_object<X>(a));
        }

        CHECK_EQ(i, data);
    }

    {
        ss::parser<ss::string_error> p{f.name, ","};
        CHECK_EQ(header, p.header());
        CHECK_EQ(merge_header(p.header(), ","), p.raw_header());

        p.use_fields(Int, Dbl);
        CHECK_EQ(header, p.header());
        CHECK_EQ(merge_header(p.header(), ","), p.raw_header());

        {
            auto [int_, double_] = p.get_next<int, double>();
            CHECK_EQ(int_, data[0].i);
            CHECK_EQ(double_, data[0].d);
        }

        p.use_fields(Dbl, Int);
        CHECK_EQ(header, p.header());
        CHECK_EQ(merge_header(p.header(), ","), p.raw_header());

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
                    '>(o, d, header, {' + ', '.join(arg_params) + '});'
                print(call)
        */

    test_fields<T, str>(o, d, header, {Str});
    test_fields<T, int>(o, d, header, {Int});
    test_fields<T, double>(o, d, header, {Dbl});
    test_fields<T, str, int>(o, d, header, {Str, Int});
    test_fields<T, str, double>(o, d, header, {Str, Dbl});
    test_fields<T, int, str>(o, d, header, {Int, Str});
    test_fields<T, int, double>(o, d, header, {Int, Dbl});
    test_fields<T, double, str>(o, d, header, {Dbl, Str});
    test_fields<T, double, int>(o, d, header, {Dbl, Int});
    test_fields<T, str, int, double>(o, d, header, {Str, Int, Dbl});
    test_fields<T, str, double, int>(o, d, header, {Str, Dbl, Int});
    test_fields<T, int, str, double>(o, d, header, {Int, Str, Dbl});
    test_fields<T, int, double, str>(o, d, header, {Int, Dbl, Str});
    test_fields<T, double, str, int>(o, d, header, {Dbl, Str, Int});
    test_fields<T, double, int, str>(o, d, header, {Dbl, Int, Str});
}

template <typename T>
void test_invalid_fields(const std::vector<std::string>& lines,
                         const std::vector<std::string>& fields) {
    constexpr auto buffer_mode = T::BufferMode::value;
    using ErrorMode = typename T::ErrorMode;

    auto check_header = [&lines](auto& p) {
        if (lines.empty()) {
            CHECK(p.header().empty());
            CHECK_EQ(merge_header(p.header(), ","), p.raw_header());
        } else {
            CHECK_EQ(lines[0], merge_header(p.header()));
            CHECK_EQ(merge_header(p.header(), ","), p.raw_header());
        }
        CHECK(p.valid());
    };

    unique_file_name f{"invalid_fields"};
    {
        std::ofstream out{f.name};
        for (const auto& line : lines) {
            out << line << std::endl;
        }
    }

    {
        // No fields specified
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        auto command = [&p = p] { p.use_fields(); };
        expect_error_on_command(p, command);
        check_header(p);
    }

    {
        // Unknown field
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        auto command = [&p = p] { p.use_fields("Unknown"); };
        expect_error_on_command(p, command);
        check_header(p);
    }

    {
        // Field used multiple times
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        auto command = [&p = p, &fields = fields] {
            p.use_fields(fields.at(0), fields.at(0));
        };
        if (!fields.empty()) {
            expect_error_on_command(p, command);
        }
        check_header(p);
    }

    {
        // Mapping out of range
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        auto command = [&p = p, &fields = fields] {
            p.use_fields(fields.at(0));
            std::ignore = p.template get_next<std::string, std::string>();
        };
        check_header(p);

        if (!fields.empty()) {
            expect_error_on_command(p, command);
        }
        check_header(p);
    }

    {
        // Invalid header
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name, ",");
        auto command = [&p = p, &fields = fields] { p.use_fields(fields); };
        check_header(p);

        if (!fields.empty()) {
            // Pass if there are no duplicates, fail otherwise
            if (std::unordered_set<std::string>{fields.begin(), fields.end()}
                    .size() != fields.size()) {
                expect_error_on_command(p, command);
            } else {
                command();
                CHECK(p.valid());
                if (!p.valid()) {
                    if constexpr (T::StringError) {
                        std::cout << p.error_msg() << std::endl;
                    }
                }
            }
        }
        check_header(p);
    }
}

TEST_CASE_TEMPLATE("test invalid fheader fields usage", T,
                   ParserOptionCombinations) {
    test_invalid_fields<T>({}, {});

    test_invalid_fields<T>({"Int"}, {"Int"});
    test_invalid_fields<T>({"Int", "1"}, {"Int"});
    test_invalid_fields<T>({"Int", "1", "2"}, {"Int"});

    test_invalid_fields<T>({"Int,String"}, {"Int", "String"});
    test_invalid_fields<T>({"Int,String", "1,hi"}, {"Int", "String"});
    test_invalid_fields<T>({"Int,String", "2,hello"}, {"Int", "String"});

    test_invalid_fields<T>({"Int,String,Double"}, {"Int", "String", "Double"});
    test_invalid_fields<T>({"Int,String,Double", "1,hi,2.34"},
                           {"Int", "String", "Double"});
    test_invalid_fields<T>({"Int,String,Double", "1,hi,2.34", "2,hello,3.45"},
                           {"Int", "String", "Double"});

    test_invalid_fields<T>({"Int,Int,Int"}, {"Int", "Int", "Int"});
    test_invalid_fields<T>({"Int,Int,Int", "1,2,3"}, {"Int", "Int", "Int"});

    test_invalid_fields<T>({"Int,String,Int"}, {"Int", "String", "Int"});
    test_invalid_fields<T>({"Int,String,Int", "1,hi,3"},
                           {"Int", "String", "Int"});
}

TEST_CASE_TEMPLATE("test invalid rows with header", T,
                   ParserOptionCombinations) {
    constexpr auto buffer_mode = T::BufferMode::value;
    using ErrorMode = typename T::ErrorMode;

    unique_file_name f{"invalid_rows_with_header"};
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

    std::vector<std::string> header = {"Int", "String", "Double"};

    {
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name);
        CHECK_EQ_ARRAY(header, p.header());
        CHECK_EQ(merge_header(p.header()), p.raw_header());

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
        CHECK_EQ_ARRAY(header, p.header());
        CHECK_EQ(merge_header(p.header()), p.raw_header());
    }

    {
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name);
        CHECK_EQ_ARRAY(header, p.header());
        CHECK_EQ(merge_header(p.header()), p.raw_header());

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
        CHECK_EQ_ARRAY(header, p.header());
        CHECK_EQ(merge_header(p.header()), p.raw_header());
    }

    {
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name);
        CHECK_EQ_ARRAY(header, p.header());
        CHECK_EQ(merge_header(p.header()), p.raw_header());

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
        CHECK_EQ_ARRAY(header, p.header());
        CHECK_EQ(merge_header(p.header()), p.raw_header());
    }
}

TEST_CASE_TEMPLATE("test invalid header", T, ParserOptionCombinations) {
    constexpr auto buffer_mode = T::BufferMode::value;
    using ErrorMode = typename T::ErrorMode;

    unique_file_name f{"invalid_header"};

    // Empty header
    {
        std::ofstream out{f.name};
        out << "" << std::endl;
        out << "1" << std::endl;
    }

    {
        auto [p, _] = make_parser<buffer_mode, ErrorMode>(f.name);
        CHECK(p.header().empty());
        CHECK_EQ(merge_header(p.header()), p.raw_header());
        CHECK(p.valid());
    }

    // Unterminated quote in header
    {
        std::ofstream out{f.name};
        out << "\"Int" << std::endl;
        out << "1" << std::endl;
    }

    {
        auto [p, _] =
            make_parser<buffer_mode, ErrorMode, ss::quote<'"'>>(f.name);
        auto command = [&p = p] { std::ignore = p.header(); };
        expect_error_on_command(p, command);
        CHECK_EQ(p.raw_header(), "\"Int");
    }

    {
        auto [p, _] =
            make_parser<buffer_mode, ErrorMode, ss::quote<'"'>, ss::multiline>(
                f.name);
        auto command = [&p = p] { std::ignore = p.header(); };
        expect_error_on_command(p, command);
        CHECK_EQ(p.raw_header(), "\"Int");
    }

    {
        auto [p, _] = make_parser<buffer_mode, ErrorMode, ss::quote<'"'>,
                                  ss::escape<'\\'>, ss::multiline>(f.name);
        auto command = [&p = p] { std::ignore = p.header(); };
        expect_error_on_command(p, command);
        CHECK_EQ(p.raw_header(), "\"Int");
    }

    // Unterminated escape in header
    {
        std::ofstream out{f.name};
        out << "Int\\" << std::endl;
        out << "1" << std::endl;
    }

    {
        auto [p, _] =
            make_parser<buffer_mode, ErrorMode, ss::escape<'\\'>>(f.name);
        auto command = [&p = p] { std::ignore = p.header(); };
        expect_error_on_command(p, command);
        CHECK_EQ(p.raw_header(), "Int\\");
    }

    {
        auto [p, _] = make_parser<buffer_mode, ErrorMode, ss::escape<'\\'>,
                                  ss::multiline>(f.name);
        auto command = [&p = p] { std::ignore = p.header(); };
        expect_error_on_command(p, command);
        CHECK_EQ(p.raw_header(), "Int\\");
    }

    {
        auto [p, _] = make_parser<buffer_mode, ErrorMode, ss::escape<'\\'>,
                                  ss::quote<'"'>, ss::multiline>(f.name);
        auto command = [&p = p] { std::ignore = p.header(); };
        expect_error_on_command(p, command);
        CHECK_EQ(p.raw_header(), "Int\\");
    }
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
