#include "test_helpers.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <ss/parser.hpp>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

// parser tests v2

namespace {
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
    std::string line;
    for (size_t i = 0; i < data.size(); ++i) {
        line += data[i];
        if (i != data.size() - 1) {
            line += delim;
        }
    }

    out << line << std::endl;
}

#define CHECK_EQ_CRLF(V1, V2)                                                  \
    if (V1 != V2) {                                                            \
        auto tmp1 = V1;                                                        \
        auto tmp2 = V2;                                                        \
        replace_all2(tmp1, "\r\n", "\n");                                      \
        replace_all2(tmp2, "\r\n", "\n");                                      \
        CHECK(tmp1 == tmp2);                                                   \
    } else {                                                                   \
        CHECK(V1 == V2);                                                       \
    }

template <typename... Ts>
void test_combinations(const std::vector<column>& input_data,
                       const std::string& delim, bool include_header) {
    // TODO test without string_error
    using setup = ss::setup<Ts..., ss::string_error>;

    unique_file_name f{"test_parser2"};
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
                    CHECK_EQ_CRLF(s0, expected_data[i][layout[0]].value);
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
                    CHECK_EQ_CRLF(s0, expected_data[i][layout[0]].value);
                    CHECK_EQ_CRLF(s1, expected_data[i][layout[1]].value);
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
                    CHECK_EQ_CRLF(s0, expected_data[i][layout[0]].value);
                    CHECK_EQ_CRLF(s1, expected_data[i][layout[1]].value);
                    CHECK_EQ_CRLF(s2, expected_data[i][layout[2]].value);
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
                    CHECK_EQ_CRLF(s0, expected_data[i][layout[0]].value);
                    CHECK_EQ_CRLF(s1, expected_data[i][layout[1]].value);
                    CHECK_EQ_CRLF(s2, expected_data[i][layout[2]].value);
                    CHECK_EQ_CRLF(s3, expected_data[i][layout[3]].value);
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
                    // std::cout << s0 << ' ' << s1 << ' ' << s2 << ' ' << s3
                    //  << ' ' << s4 << std::endl;
                    CHECK_EQ_CRLF(s0, expected_data[i][layout[0]].value);
                    CHECK_EQ_CRLF(s1, expected_data[i][layout[1]].value);
                    CHECK_EQ_CRLF(s2, expected_data[i][layout[2]].value);
                    CHECK_EQ_CRLF(s3, expected_data[i][layout[3]].value);
                    CHECK_EQ_CRLF(s4, expected_data[i][layout[4]].value);
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
} /* namespace */

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
    /* TODO uncomment
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
    */
    test_combinations_impl<quote, escape, multiline, trimr>();
}
