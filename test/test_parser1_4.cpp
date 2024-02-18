#include "test_parser1.hpp"

template <typename T, typename Tuple>
struct has_type;

template <typename T, typename... Us>
struct has_type<T, std::tuple<Us...>>
    : std::disjunction<std::is_same<T, Us>...> {};

template <bool buffer_mode, typename Setup, typename... Ts>
static void test_fields_impl(const std::string file_name,
                             const std::vector<X>& data,
                             const std::vector<std::string>& fields) {
    using CaseType = std::tuple<Ts...>;

    auto [p, _] = make_parser<buffer_mode, Setup>(file_name, ",");
    CHECK_FALSE(p.field_exists("Unknown"));
    p.use_fields(fields);
    std::vector<CaseType> i;

    for (const auto& a : p.template iterate<CaseType>()) {
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

template <typename... Ts>
static void test_fields(const std::string file_name, const std::vector<X>& data,
                        const std::vector<std::string>& fields) {
    test_fields_impl<false, ss::setup<>, Ts...>(file_name, data, fields);
    test_fields_impl<false, ss::setup<ss::string_error>, Ts...>(file_name, data,
                                                                fields);
    test_fields_impl<false, ss::setup<ss::throw_on_error>, Ts...>(file_name,
                                                                  data, fields);
    test_fields_impl<true, ss::setup<>, Ts...>(file_name, data, fields);
    test_fields_impl<true, ss::setup<ss::string_error>, Ts...>(file_name, data,
                                                               fields);
    test_fields_impl<true, ss::setup<ss::throw_on_error>, Ts...>(file_name,
                                                                 data, fields);
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

template <bool buffer_mode, typename... Ts>
void test_invalid_fields_impl(const std::vector<std::string>& lines,
                              const std::vector<std::string>& fields) {
    unique_file_name f{"test_parser"};
    {
        std::ofstream out{f.name};
        for (const auto& line : lines) {
            out << line << std::endl;
        }
    }

    {
        // No fields specified
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
        auto command = [&p = p] { p.use_fields(); };
        expect_error_on_command(p, command);
    }

    {
        // Unknown field
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
        auto command = [&p = p] { p.use_fields("Unknown"); };
        expect_error_on_command(p, command);
    }

    {
        // Field used multiple times
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
        auto command = [&p = p, &fields = fields] {
            p.use_fields(fields.at(0), fields.at(0));
        };
        if (!fields.empty()) {
            expect_error_on_command(p, command);
        }
    }

    {
        // Mapping out of range
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
        auto command = [&p = p, &fields = fields] {
            p.use_fields(fields.at(0));
            p.template get_next<std::string, std::string>();
        };
        if (!fields.empty()) {
            expect_error_on_command(p, command);
        }
    }

    {
        // Invalid header
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
        auto command = [&p = p, &fields = fields] { p.use_fields(fields); };

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
    test_invalid_fields_impl<false>(lines, fields);
    test_invalid_fields_impl<false, ss::string_error>(lines, fields);
    test_invalid_fields_impl<false, ss::throw_on_error>(lines, fields);
    test_invalid_fields_impl<true>(lines, fields);
    test_invalid_fields_impl<true, ss::string_error>(lines, fields);
    test_invalid_fields_impl<true, ss::throw_on_error>(lines, fields);
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

template <bool buffer_mode, typename... Ts>
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
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name);

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
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name);

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
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name);

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
    test_invalid_rows_with_header<false>();
    test_invalid_rows_with_header<false, ss::string_error>();
    test_invalid_rows_with_header<false, ss::throw_on_error>();
    test_invalid_rows_with_header<true>();
    test_invalid_rows_with_header<true, ss::string_error>();
    test_invalid_rows_with_header<true, ss::throw_on_error>();
}

template <bool buffer_mode, typename... Ts>
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
        auto [p, _] =
            make_parser<buffer_mode, ss::ignore_empty, Ts...>(f.name, ",");

        std::vector<X> i;
        for (const auto& a : p.template iterate<X>()) {
            i.push_back(a);
        }

        CHECK_EQ(i, expected);
    }

    {
        auto [p, _] = make_parser<buffer_mode, Ts...>(f.name, ",");
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
    test_ignore_empty_impl<false>(data);
    test_ignore_empty_impl<false, ss::string_error>(data);
    test_ignore_empty_impl<false, ss::throw_on_error>(data);
    test_ignore_empty_impl<true>(data);
    test_ignore_empty_impl<true, ss::string_error>(data);
    test_ignore_empty_impl<true, ss::throw_on_error>(data);
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
