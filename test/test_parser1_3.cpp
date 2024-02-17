#include "test_parser1.hpp"

template <typename... Ts>
void test_multiline_restricted() {
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
    auto bad_lines = 15;
    auto num_errors = 0;

    ss::parser<ss::multiline_restricted<2>, ss::quote<'"'>, ss::escape<'\\'>,
               Ts...>
        p{f.name, ","};
    std::vector<X> i;

    while (!p.eof()) {
        try {
            auto a = p.template get_next<int, double, std::string>();
            if (p.valid()) {
                i.emplace_back(ss::to_object<X>(a));
            } else {
                ++num_errors;
            }
        } catch (const std::exception& e) {
            ++num_errors;
        }
    }

    CHECK(bad_lines == num_errors);

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

    if (i.size() != data.size()) {
        CHECK_EQ(i.size(), data.size());
    }

    CHECK_EQ(i, data);
}

TEST_CASE("parser test multiline restricted") {
    test_multiline_restricted();
    test_multiline_restricted<ss::string_error>();
    test_multiline_restricted<ss::throw_on_error>();
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
            command();
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

TEST_CASE("parser test csv on multiline with errors") {
    using multiline = ss::multiline_restricted<3>;
    using escape = ss::escape<'\\'>;
    using quote = ss::quote<'"'>;

    // unterminated escape
    {
        const std::vector<std::string> lines{"1,2,just\\"};
        test_unterminated_line<multiline, escape>(lines, 0);
        test_unterminated_line<multiline, escape, quote>(lines, 0);
    }

    {
        const std::vector<std::string> lines{"1,2,just\\", "9,8,second"};
        test_unterminated_line<multiline, escape>(lines, 0);
        test_unterminated_line<multiline, escape, quote>(lines, 0);
    }

    {
        const std::vector<std::string> lines{"9,8,first", "1,2,just\\"};
        test_unterminated_line<multiline, escape>(lines, 1);
        test_unterminated_line<multiline, escape, quote>(lines, 1);
    }

    {
        const std::vector<std::string> lines{"9,8,first", "1,2,just\\",
                                             "3,4,third"};
        test_unterminated_line<multiline, escape>(lines, 1);
        test_unterminated_line<multiline, escape, quote>(lines, 1);
    }

    {
        const std::vector<std::string> lines{"9,8,first",
                                             "1,2,just\\\nstrings\\",
                                             "3,4,th\\\nird"};
        test_unterminated_line<multiline, escape>(lines, 1);
        test_unterminated_line<multiline, escape, quote>(lines, 1);
    }

    {
        const std::vector<std::string> lines{"9,8,first", "3,4,second",
                                             "1,2,just\\"};
        test_unterminated_line<multiline, escape>(lines, 2);
        test_unterminated_line<multiline, escape, quote>(lines, 2);
    }

    {
        const std::vector<std::string> lines{"9,8,\\first", "3,4,second",
                                             "1,2,jus\\t\\"};
        test_unterminated_line<multiline, escape>(lines, 2);
        test_unterminated_line<multiline, escape, quote>(lines, 2);
    }

    // unterminated quote
    {
        const std::vector<std::string> lines{"1,2,\"just"};
        test_unterminated_line<multiline, escape, quote>(lines, 0);
        test_unterminated_line<multiline, quote>(lines, 0);
    }

    {
        const std::vector<std::string> lines{"1,2,\"just", "9,8,second"};
        test_unterminated_line<multiline, escape, quote>(lines, 0);
        test_unterminated_line<multiline, quote>(lines, 0);
    }

    {
        const std::vector<std::string> lines{"9,8,first", "1,2,\"just"};
        test_unterminated_line<multiline, escape, quote>(lines, 1);
        test_unterminated_line<multiline, quote>(lines, 1);
    }

    {
        const std::vector<std::string> lines{"9,8,first", "1,2,\"just",
                                             "3,4,th\\,ird"};
        test_unterminated_line<multiline, escape, quote>(lines, 1);
        test_unterminated_line<multiline, quote>(lines, 1);
    }

    {
        const std::vector<std::string> lines{"9,8,first", "3,4,second",
                                             "1,2,\"just"};
        test_unterminated_line<multiline, escape, quote>(lines, 2);
        test_unterminated_line<multiline, quote>(lines, 2);
    }

    {
        const std::vector<std::string> lines{"9,8,\"first\"",
                                             "\"3\",4,\"sec,ond\"",
                                             "1,2,\"ju\"\"st"};
        test_unterminated_line<multiline, escape, quote>(lines, 2);
        test_unterminated_line<multiline, quote>(lines, 2);
    }

    // unterminated quote and escape
    {
        const std::vector<std::string> lines{"1,2,\"just\\"};
        test_unterminated_line<multiline, escape, quote>(lines, 0);
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

    {
        const std::vector<std::string> lines{"9,8,first", "1,2,\"just\n\\"};
        test_unterminated_line<multiline, escape, quote>(lines, 1);
    }

    {
        const std::vector<std::string> lines{"9,8,first", "1,2,\"just\n\\",
                                             "4,3,thrid"};
        test_unterminated_line<multiline, escape, quote>(lines, 1);
    }

    {
        const std::vector<std::string> lines{"9,8,f\\\nirst", "1,2,\"just\n\\",
                                             "4,3,thrid"};
        test_unterminated_line<multiline, escape, quote>(lines, 1);
    }

    {
        const std::vector<std::string> lines{"9,8,\"f\ni\nrst\"",
                                             "1,2,\"just\n\\", "4,3,thrid"};
        test_unterminated_line<multiline, escape, quote>(lines, 1);
    }

    // multiline limmit reached escape
    {
        const std::vector<std::string> lines{"1,2,\\\n\\\n\\\n\\\njust"};
        test_unterminated_line<multiline, escape>(lines, 0);
        test_unterminated_line<multiline, escape, quote>(lines, 0);
    }

    {
        const std::vector<std::string> lines{"9,8,first",
                                             "1,2,\\\n\\\n\\\n\\\njust"};
        test_unterminated_line<multiline, escape>(lines, 1);
        test_unterminated_line<multiline, escape, quote>(lines, 1);
    }

    {
        const std::vector<std::string> lines{"9,8,fi\\\nrs\\\nt",
                                             "1,2,\\\n\\\n\\\n\\\njust"};
        test_unterminated_line<multiline, escape>(lines, 1);
        test_unterminated_line<multiline, escape, quote>(lines, 1);
    }

    {
        const std::vector<std::string> lines{"9,8,first",
                                             "1,2,\\\n\\\n\\\n\\\njust",
                                             "4,3,third"};
        test_unterminated_line<multiline, escape>(lines, 1);
        test_unterminated_line<multiline, escape, quote>(lines, 1);
    }

    // multiline limmit reached quote
    {
        const std::vector<std::string> lines{"1,2,\"\n\n\n\n\njust\""};
        test_unterminated_line<multiline, escape, quote>(lines, 0);
        test_unterminated_line<multiline, quote>(lines, 0);
    }

    {
        const std::vector<std::string> lines{"9,8,first",
                                             "1,2,\"\n\n\n\n\njust\""};
        test_unterminated_line<multiline, escape, quote>(lines, 1);
        test_unterminated_line<multiline, quote>(lines, 1);
    }

    {
        const std::vector<std::string> lines{"9,8,\"fir\nst\"",
                                             "1,2,\"\n\n\n\n\njust\""};
        test_unterminated_line<multiline, escape, quote>(lines, 1);
        test_unterminated_line<multiline, quote>(lines, 1);
    }

    // multiline limmit reached quote and escape
    {
        const std::vector<std::string> lines{"1,2,\"\\\n\n\\\n\\\n\\\njust"};
        test_unterminated_line<multiline, escape, quote>(lines, 0);
    }

    {
        const std::vector<std::string> lines{"9,8,first",
                                             "1,2,\"\\\n\n\\\n\\\n\\\njust"};
        test_unterminated_line<multiline, escape, quote>(lines, 1);
    }

    {
        const std::vector<std::string> lines{"9,8,fi\\\nrst",
                                             "1,2,\"\\\n\n\\\n\\\n\\\njust"};
        test_unterminated_line<multiline, escape, quote>(lines, 1);
    }

    {
        const std::vector<std::string> lines{"9,8,\"fi\nrst\"",
                                             "1,2,\"\\\n\n\\\n\\\n\\\njust"};
        test_unterminated_line<multiline, escape, quote>(lines, 1);
    }

    {
        const std::vector<std::string> lines{"9,8,\"fi\nr\\\nst\"",
                                             "1,2,\"\\\n\n\\\n\\\n\\\njust"};
        test_unterminated_line<multiline, escape, quote>(lines, 1);
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

template <typename Setup, typename... Ts>
static void test_fields_impl(const std::string file_name,
                             const std::vector<X>& data,
                             const std::vector<std::string>& fields) {
    using CaseType = std::tuple<Ts...>;

    ss::parser<Setup> p{file_name, ","};
    CHECK_FALSE(p.field_exists("Unknown"));
    p.use_fields(fields);
    std::vector<CaseType> i;

    for (const auto& a : p.template iterate<CaseType>()) {
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

template <typename... Ts>
static void test_fields(const std::string file_name, const std::vector<X>& data,
                        const std::vector<std::string>& fields) {
    test_fields_impl<ss::setup<>, Ts...>(file_name, data, fields);
    test_fields_impl<ss::setup<ss::string_error>, Ts...>(file_name, data,
                                                         fields);
    test_fields_impl<ss::setup<ss::throw_on_error>, Ts...>(file_name, data,
                                                           fields);
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
