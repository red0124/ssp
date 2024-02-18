#include "test_parser1.hpp"

template <bool buffer_mode, typename... Ts>
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

    auto [p, _] =
        make_parser<buffer_mode, ss::multiline_restricted<2>, ss::quote<'"'>,
                    ss::escape<'\\'>, Ts...>(f.name, ",");
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
    test_multiline_restricted<false>();
    test_multiline_restricted<false, ss::string_error>();
    test_multiline_restricted<false, ss::throw_on_error>();
    test_multiline_restricted<true>();
    test_multiline_restricted<true, ss::string_error>();
    test_multiline_restricted<true, ss::throw_on_error>();
}

template <bool buffer_mode, typename... Ts>
void test_unterminated_line_impl(const std::vector<std::string>& lines,
                                 size_t bad_line) {
    unique_file_name f{"test_parser"};
    std::ofstream out{f.name};
    for (const auto& line : lines) {
        out << line << std::endl;
    }
    out.close();

    auto [p, _] = make_parser<buffer_mode, Ts...>(f.name);
    size_t line = 0;
    while (!p.eof()) {
        auto command = [&p = p] {
            p.template get_next<int, double, std::string>();
        };

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
    test_unterminated_line_impl<false, Ts...>(lines, bad_line);
    test_unterminated_line_impl<false, Ts..., ss::string_error>(lines,
                                                                bad_line);
    test_unterminated_line_impl<false, Ts..., ss::throw_on_error>(lines,
                                                                  bad_line);
    test_unterminated_line_impl<true, Ts...>(lines, bad_line);
    test_unterminated_line_impl<true, Ts..., ss::string_error>(lines, bad_line);
    test_unterminated_line_impl<true, Ts..., ss::throw_on_error>(lines,
                                                                 bad_line);
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
