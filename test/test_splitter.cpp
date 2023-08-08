#include "test_helpers.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <ss/splitter.hpp>

namespace {
constexpr static auto num_combinations_default = 4;
size_t num_combinations = num_combinations_default;

struct set_num_combinations {
    set_num_combinations(size_t size) {
        num_combinations = size;
    }
    ~set_num_combinations() {
        num_combinations = num_combinations_default;
    }
};

std::vector<std::string> words(const ss::split_data& input) {
    std::vector<std::string> ret;
    for (const auto& [begin, end] : input) {
        ret.emplace_back(begin, end);
    }
    return ret;
}

[[maybe_unused]] std::string concat(const std::vector<std::string>& v) {
    std::string ret = "[";
    for (const auto& i : v) {
        ret.append(i).append(",");
    }
    ret.back() = (']');
    return ret;
}

template <typename... Ts>
size_t strings_size(const std::string& s, const Ts&... ss) {
    if constexpr (sizeof...(Ts) > 0) {
        return s.size() + strings_size(ss...);
    }
    return s.size();
}

template <typename... Ts>
void concat_to(std::string& dst, const std::string& s, const Ts&... ss) {
    dst.append(s);
    if constexpr (sizeof...(Ts) > 0) {
        concat_to(dst, ss...);
    }
}

template <typename... Ts>
std::string concat(const Ts&... ss) {
    std::string ret;
    ret.reserve(strings_size(ss...));
    concat_to(ret, ss...);
    return ret;
}

using case_type = std::vector<std::string>;
auto spaced(const case_type& input, const std::string& s) {
    case_type ret = input;
    for (const auto& i : input) {
        ret.push_back(concat(s, i, s));
        ret.push_back(concat(i, s));
        ret.push_back(concat(s, i));
        ret.push_back(concat(s, s, i));
        ret.push_back(concat(s, s, i, s, s));
        ret.push_back(concat(i, s, s));
    }

    return ret;
}

auto spaced(const case_type& input, const std::string& s1,
            const std::string& s2) {
    case_type ret = input;
    for (const auto& i : input) {
        ret.push_back(concat(s1, i, s2));
        ret.push_back(concat(s2, i, s1));
        ret.push_back(concat(s2, s2, s1, s1, i));
        ret.push_back(concat(i, s1, s2, s1, s2));
        ret.push_back(concat(s1, s1, s1, i, s2, s2, s2));
        ret.push_back(concat(s2, s2, s2, i, s1, s1, s1));
    }

    return ret;
}

auto spaced_left(const case_type& input, const std::string& s) {
    case_type ret = input;
    for (const auto& i : input) {
        ret.push_back(concat(i));
        ret.push_back(concat(s, i));
        ret.push_back(concat(s, s, i));
        ret.push_back(concat(s, s, s, i));
    }

    return ret;
}

auto spaced_right(const case_type& input, const std::string& s) {
    case_type ret = input;
    for (const auto& i : input) {
        ret.push_back(concat(i));
        ret.push_back(concat(i, s));
        ret.push_back(concat(i, s, s));
        ret.push_back(concat(i, s, s, s));
    }

    return ret;
}

std::vector<std::string> combinations(const std::vector<std::string>& v,
                                      const std::string& delim, size_t n) {
    if (n <= 1) {
        return v;
    }
    std::vector<std::string> ret;
    auto inner_combinations = combinations(v, delim, n - 1);
    for (const auto& i : v) {
        for (const auto& j : inner_combinations) {
            ret.push_back(concat(i, delim, j));
        }
    }
    return ret;
}

std::pair<std::vector<std::string>, std::vector<std::vector<std::string>>>
make_combinations(const std::vector<std::string>& input,
                  const std::vector<std::string>& output,
                  const std::string& delim) {
    std::vector<std::string> lines;
    std::vector<std::vector<std::string>> expectations;
    for (size_t i = 0; i < num_combinations; ++i) {
        auto l = combinations(input, delim, i);
        lines.reserve(lines.size() + l.size());
        lines.insert(lines.end(), l.begin(), l.end());

        auto e = vector_combinations(output, i);
        expectations.reserve(expectations.size() + e.size());
        expectations.insert(expectations.end(), e.begin(), e.end());
    }

    return {std::move(lines), std::move(expectations)};
}
} /* namespace */

/* ********************************** */
/* ********************************** */

using matches_type = std::vector<std::pair<case_type, std::string>>;

template <typename... Matchers>
static inline void test_combinations(matches_type& matches,
                                     std::vector<std::string> delims) {

    ss::splitter<Matchers...> s;
    ss::splitter<Matchers..., ss::throw_on_error> st;

    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
    for (const auto& [cases, e] : matches) {
        for (const auto& c : cases) {
            inputs.emplace_back(c);
            outputs.emplace_back(e);
        }
    }

    for (const auto& delim : delims) {
        auto [lines, expectations] = make_combinations(inputs, outputs, delim);

        REQUIRE(lines.size() == expectations.size());

        for (size_t i = 0; i < lines.size(); ++i) {
            auto vec = s.split(buff(lines[i].c_str()), delim);
            CHECK(s.valid());
            CHECK_EQ(words(vec), expectations[i]);

            try {
                auto vec = st.split(buff(lines[i].c_str()), delim);
                CHECK_EQ(words(vec), expectations[i]);
            } catch (ss::exception& e) {
                FAIL(std::string{e.what()});
            }
        }
    }
}

TEST_CASE("splitter test with no setup") {
    {
        matches_type p{{{"x"}, "x"},        {{"\""}, "\""},
                       {{""}, ""},          {{"\n"}, "\n"},
                       {{"\"\""}, "\"\""},  {{"\" \\ \""}, "\" \\ \""},
                       {{"     "}, "     "}};
        test_combinations(p, {",", ";", "\t", "::"});
    }
}

TEST_CASE("splitter test with quote") {
    case_type case1 = {R"("""")"};
    case_type case2 = {R"("x""x")", R"(x"x)"};
    case_type case3 = {R"("")", R"()"};
    case_type case4 = {R"("x")", R"(x)"};
    case_type case5 = {R"("""""")"};
    case_type case6 = {R"("\")", R"(\)"};
    case_type case7 = {R"("xxxxxxxxxx")", R"(xxxxxxxxxx)"};

    std::vector<std::string> delims = {",", "::", " ", "\t", "\n"};

    {
        matches_type p{{case1, "\""},        {case2, "x\"x"}, {case3, ""},
                       {case4, "x"},         {case5, "\"\""}, {case6, "\\"},
                       {case7, "xxxxxxxxxx"}};
        test_combinations<ss::quote<'"'>>(p, delims);
    }

    case_type case8 = {R"(",")"};
    case_type case9 = {R"("x,")"};
    case_type case10 = {R"(",x")"};
    case_type case11 = {R"("x,x")"};
    case_type case12 = {R"(",,")"};
    {
        matches_type p{{case1, "\""}, {case3, ""},    {case8, ","},
                       {case9, "x,"}, {case10, ",x"}, {case11, "x,x"},
                       {case12, ",,"}};
        test_combinations<ss::quote<'"'>>(p, {","});
    }

    case_type case13 = {R"("::")"};
    case_type case14 = {R"("x::")"};
    case_type case15 = {R"("::x")"};
    case_type case16 = {R"("x::x")"};
    case_type case17 = {R"("::::")"};

    {
        matches_type p{{case1, "\""},   {case3, ""},     {case13, "::"},
                       {case14, "x::"}, {case15, "::x"}, {case16, "x::x"},
                       {case17, "::::"}};
        test_combinations<ss::quote<'"'>>(p, {"::"});
    }
}

TEST_CASE("splitter test with trim") {
    auto guard = set_num_combinations(3);
    case_type case1 = spaced({R"(x)"}, " ");
    case_type case2 = spaced({R"(yy)"}, " ");
    case_type case3 = spaced({R"(y y)"}, " ");
    case_type case4 = spaced({R"()"}, " ");

    std::vector<std::string> delims = {",", "::", "\t", "\n"};

    {
        matches_type p{{case1, "x"},
                       {case2, "yy"},
                       {case3, "y y"},
                       {case4, ""}};
        test_combinations<ss::trim<' '>>(p, delims);
    }

    case_type case5 = spaced({"z"}, "\t");
    case_type case6 = spaced({"ab"}, " ", "\t");
    case_type case7 = spaced({"a\tb"}, " ", "\t");
    case_type case8 = spaced({"a \t b"}, " ", "\t");

    {
        matches_type p{{case1, "x"},    {case2, "yy"},    {case3, "y y"},
                       {case4, ""},     {case5, "z"},     {case6, "ab"},
                       {case7, "a\tb"}, {case8, "a \t b"}};
        test_combinations<ss::trim<' ', '\t'>>(p, {",", "::", "\n"});
    }
}

TEST_CASE("splitter test with escape") {
    case_type case1 = {R"(x)", R"(\x)"};
    case_type case2 = {R"(xx)", R"(\xx)", R"(x\x)", R"(\x\x)"};
    case_type case3 = {R"(\\)"};

    std::vector<std::string> delims = {",", "::", " ", "\t", "\n"};

    {
        matches_type p{{case1, "x"}, {case2, "xx"}, {case3, "\\"}};
        test_combinations<ss::escape<'\\'>>(p, delims);
    }

    case_type case4 = {R"(\,)"};
    case_type case5 = {R"(x#,)"};
    case_type case6 = {R"(#,x)"};
    case_type case7 = {R"(x\,x)"};

    {
        matches_type p{{case1, "x"},  {case2, "xx"}, {case3, "\\"},
                       {case4, ","},  {case5, "x,"}, {case6, ",x"},
                       {case7, "x,x"}};
        test_combinations<ss::escape<'\\', '#'>>(p, {","});
    }

    case_type case8 = {R"(\:\:)"};
    case_type case9 = {R"(x\::x)"};

    {
        matches_type p{{case1, "x"},
                       {case2, "xx"},
                       {case3, "\\"},
                       {case8, "::"},
                       {case9, "x::x"}};
        test_combinations<ss::escape<'\\'>>(p, {"::"});
    }
}

TEST_CASE("splitter test with quote and trim") {
    auto guard = set_num_combinations(3);
    case_type case1 = spaced({R"("""")"}, " ");
    case_type case2 = spaced({R"("x""x")", R"(x"x)"}, " ");
    case_type case3 = spaced({R"("")", R"()"}, " ");
    case_type case4 = spaced({R"("x")", R"(x)"}, " ");
    case_type case5 = spaced({R"("""""")"}, " ");
    case_type case6 = spaced({R"("\")", R"(\)"}, " ");
    case_type case7 = spaced({R"("xxxxxxxxxx")", R"(xxxxxxxxxx)"}, " ");

    std::vector<std::string> delims = {",", "::", "\t", "\n"};

    {
        matches_type p{{case1, "\""},        {case2, "x\"x"}, {case3, ""},
                       {case4, "x"},         {case5, "\"\""}, {case6, "\\"},
                       {case7, "xxxxxxxxxx"}};
        test_combinations<ss::quote<'"'>, ss::trim<' '>>(p, delims);
    }

    case_type case8 = spaced({R"(",")"}, " ", "\t");
    case_type case9 = spaced({R"("x,")"}, " ", "\t");
    case_type case10 = spaced({R"(",x")"}, " ", "\t");
    case_type case11 = spaced({R"("x,x")"}, " ", "\t");
    case_type case12 = spaced({R"(",,")"}, " ", "\t");

    {
        matches_type p{{case1, "\""}, {case3, ""},    {case8, ","},
                       {case9, "x,"}, {case10, ",x"}, {case11, "x,x"},
                       {case12, ",,"}};
        test_combinations<ss::quote<'"'>, ss::trim<' ', '\t'>>(p, {","});
    }
}

TEST_CASE("splitter test with quote and escape") {
    case_type case1 = {R"("\"")", R"(\")", R"("""")"};
    case_type case2 = {R"("x\"x")", R"(x\"x)", R"(x"x)", R"("x""x")"};
    case_type case3 = {R"("")", R"()"};
    case_type case4 = {R"("x")", R"(x)"};
    case_type case5 = {R"("\"\"")", R"("""""")", R"("\"""")", R"("""\"")"};
    case_type case6 = {R"("\\")", R"(\\)"};
    case_type case7 = {R"("xxxxxxxxxx")", R"(xxxxxxxxxx)"};

    std::vector<std::string> delims = {",", "::", " ", "\t", "\n"};

    ss::splitter<ss::quote<'"'>, ss::escape<'\\'>> s;

    {
        matches_type p{{case1, "\""},        {case2, "x\"x"}, {case3, ""},
                       {case4, "x"},         {case5, "\"\""}, {case6, "\\"},
                       {case7, "xxxxxxxxxx"}};
        test_combinations<ss::quote<'"'>, ss::escape<'\\'>>(p, delims);
    }

    case_type case8 = {R"('xxxxxxxxxx')", R"(xxxxxxxxxx)"};
    case_type case9 = {R"('')", R"()"};
    case_type case10 = {R"('#\')", R"(#\)"};
    case_type case11 = {R"('#'')", R"(#')", R"('''')"};
    case_type case12 = {R"('##')", R"(##)"};
    {
        matches_type p{{case8, "xxxxxxxxxx"},
                       {case9, ""},
                       {case10, "\\"},
                       {case11, "'"},
                       {case12, "#"}};
        test_combinations<ss::quote<'\''>, ss::escape<'#'>>(p, delims);
    }

    case_type case13 = {R"("x,x")",  R"(x\,x)",   R"(x#,x)",
                        R"("x\,x")", R"("x#,x")", R"("x#,x")"};
    case_type case14 = {R"("#\\#")", R"(#\\#)", R"(\\##)", R"("\\##")"};

    {
        matches_type p{{case1, "\""},
                       {case2, "x\"x"},
                       {case3, ""},
                       {case13, "x,x"},
                       {case14, "\\#"}};
        test_combinations<ss::quote<'"'>, ss::escape<'\\', '#'>>(p, {","});
    }
}

TEST_CASE("splitter test with escape and trim") {
    case_type case0 = spaced({R"(\ x\ )", R"(\ \x\ )"}, " ");
    case_type case1 = spaced({R"(x)", R"(\x)"}, " ");
    case_type case3 = spaced({R"(\\)"}, " ");

    std::vector<std::string> delims = {",", "::", "\t", "\n"};

    {
        matches_type p{{case0, " x "}, {case1, "x"}, {case3, "\\"}};
        test_combinations<ss::escape<'\\'>, ss::trim<' '>>(p, delims);
    }

    case_type case4 = spaced({R"(\,)"}, " ");
    case_type case6 = spaced({R"(#,x)"}, " ");
    case_type case7 = spaced({R"(x\,x)"}, " ");

    {
        matches_type p{{case1, "x"},
                       {case3, "\\"},
                       {case4, ","},
                       {case6, ",x"},
                       {case7, "x,x"}};
        test_combinations<ss::escape<'\\', '#'>, ss::trim<' '>>(p, {","});
    }

    case_type case8 = spaced({R"(\:\:)"}, " ", "\t");
    case_type case9 = spaced({R"(x\::x)"}, " ", "\t");

    {
        matches_type p{{case1, "x"},
                       {case3, "\\"},
                       {case8, "::"},
                       {case9, "x::x"}};
        test_combinations<ss::escape<'\\'>, ss::trim<' ', '\t'>>(p, {"::"});
    }
}

TEST_CASE("splitter test with quote and escape and trim") {
    auto guard = set_num_combinations(3);
    case_type case1 = spaced({R"("\"")", R"(\")", R"("""")"}, " ");
    case_type case2 =
        spaced({R"("x\"x")", R"(x\"x)", R"(x"x)", R"("x""x")"}, " ");
    case_type case3 = spaced({R"("")", R"()"}, " ");
    case_type case4 = spaced({R"("x")", R"(x)"}, " ");
    case_type case5 =
        spaced({R"("\"\"")", R"("""""")", R"("\"""")", R"("""\"")"}, " ");
    case_type case6 = spaced({R"("\\")", R"(\\)"}, " ");
    case_type case7 = spaced({R"("xxxxxxxxxx")", R"(xxxxxxxxxx)"}, " ");

    std::vector<std::string> delims = {"::", "\n"};

    {
        matches_type p{{case1, "\""},   {case2, "x\"x"}, {case3, ""},
                       {case5, "\"\""}, {case6, "\\"},   {case7, "xxxxxxxxxx"}};
        test_combinations<ss::quote<'"'>, ss::escape<'\\'>,
                          ss::trim<' '>>(p, delims);
    }

    case_type case8 = spaced({R"('xxxxxxxxxx')", R"(xxxxxxxxxx)"}, " ", "\t");
    case_type case9 = spaced({R"('')", R"()"}, " ", "\t");
    case_type case10 = spaced({R"('#\')", R"(#\)"}, " ", "\t");
    case_type case11 = spaced({R"('#'')", R"(#')", R"('''')"}, " ", "\t");
    case_type case12 = spaced({R"('##')", R"(##)"}, " ", "\t");
    {
        matches_type p{{case8, "xxxxxxxxxx"},
                       {case9, ""},
                       {case10, "\\"},
                       {case11, "'"},
                       {case12, "#"}};
        test_combinations<ss::quote<'\''>, ss::escape<'#'>,
                          ss::trim<' ', '\t'>>(p, {","});
    }

    case_type case13 = spaced({R"("x,x")", R"(x\,x)", R"(x#,x)", R"("x\,x")",
                               R"("x#,x")", R"("x#,x")"},
                              " ", "\t");
    case_type case14 =
        spaced({R"("#\\#")", R"(#\\#)", R"(\\##)", R"("\\##")"}, " ", "\t");

    {
        matches_type p{{case1, "\""},
                       {case2, "x\"x"},
                       {case3, ""},
                       {case13, "x,x"},
                       {case14, "\\#"}};
        test_combinations<ss::quote<'"'>, ss::escape<'\\', '#'>,
                          ss::trim<' ', '\t'>>(p, {","});
    }
}

TEST_CASE("splitter test constnes if quoting and escaping are disabled") {
    // to compile is enough
    return;
    const char* const line{};
    ss::splitter s1;
    ss::splitter<ss::trim<' '>> s2;
    s1.split(line);
    s2.split(line);
}

TEST_CASE("splitter test error mode") {

    {
        // empty delimiter
        ss::splitter<ss::string_error> s;
        s.split(buff("just,some,strings"), "");
        CHECK_FALSE(s.valid());
        CHECK_FALSE(s.unterminated_quote());
        CHECK_FALSE(s.error_msg().empty());

        try {
            ss::splitter<ss::throw_on_error> s;
            s.split(buff("just,some,strings"), "");
            FAIL("expected exception");
        } catch (ss::exception& e) {
            CHECK_FALSE(std::string{e.what()}.empty());
            CHECK_FALSE(s.unterminated_quote());
        }
    }

    {
        // unterminated quote
        ss::splitter<ss::string_error, ss::quote<'"'>> s;
        s.split(buff("\"just"));
        CHECK_FALSE(s.valid());
        CHECK(s.unterminated_quote());
        CHECK_FALSE(s.error_msg().empty());
    }
}

template <typename Splitter>
static inline auto expect_unterminated_quote(Splitter& s,
                                             const std::string& line) {
    try {
        auto vec = s.split(buff(line.c_str()));
        CHECK(s.valid());
        CHECK(s.unterminated_quote());
        return vec;
    } catch (ss::exception& e) {
        FAIL(std::string{e.what()});
        return decltype(s.split(buff(line.c_str()))){};
    }
}

namespace ss {
// Used to test resplit since it is only accessible via friend class converter
template <typename... Matchers>
class converter {
public:
    ss::splitter<Matchers...> splitter;
    auto resplit(char* new_line, size_t new_line_size) {
        return splitter.resplit(new_line, new_line_size);
    }

    size_t size_shifted() {
        return splitter.size_shifted();
    }
};
} /* ss */

TEST_CASE("splitter test resplit unterminated quote") {

    {
        ss::converter<ss::quote<'"'>, ss::multiline, ss::escape<'\\'>> c;
        auto& s = c.splitter;

        auto vec = expect_unterminated_quote(s, R"("x)");

        CHECK_EQ(vec.size(), 1);
        REQUIRE(s.unterminated_quote());

        {
            auto new_line =
                buff.append_overwrite_last(R"(a\x)", c.size_shifted());

            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.unterminated_quote());
            CHECK_EQ(vec.size(), 1);
        }

        {
            auto new_line =
                buff.append_overwrite_last(R"(")", c.size_shifted());

            vec = c.resplit(new_line, strlen(new_line));
            REQUIRE(s.valid());
            CHECK_FALSE(s.unterminated_quote());
            REQUIRE_EQ(vec.size(), 1);
            CHECK_EQ(words(vec)[0], "xax");
        }
    }

    {
        ss::converter<ss::quote<'"'>, ss::multiline> c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, "\"just");
        CHECK_EQ(vec.size(), 1);

        auto new_line = buff.append(R"(",strings)");
        vec = c.resplit(new_line, strlen(new_line));
        CHECK(s.valid());
        CHECK_FALSE(s.unterminated_quote());
        std::vector<std::string> expected{"just", "strings"};
        CHECK_EQ(words(vec), expected);
    }

    {
        ss::converter<ss::quote<'"'>, ss::multiline> c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, "just,some,\"random");
        std::vector<std::string> expected{"just", "some", "just,some,\""};
        CHECK_EQ(words(vec), expected);

        auto new_line = buff.append(R"(",strings)");
        vec = c.resplit(new_line, strlen(new_line));
        CHECK(s.valid());
        CHECK_FALSE(s.unterminated_quote());
        expected = {"just", "some", "random", "strings"};
        CHECK_EQ(words(vec), expected);
    }

    {
        ss::converter<ss::quote<'"'>, ss::multiline> c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, R"("just","some","ran"")");
        std::vector<std::string> expected{"just", "some", R"("just","some",")"};
        CHECK_EQ(words(vec), expected);

        auto new_line =
            buff.append_overwrite_last(R"(,dom","strings")", c.size_shifted());
        vec = c.resplit(new_line, strlen(new_line));
        CHECK(s.valid());
        CHECK_FALSE(s.unterminated_quote());
        expected = {"just", "some", "ran\",dom", "strings"};
        CHECK_EQ(words(vec), expected);
    }

    {
        ss::converter<ss::quote<'"'>, ss::multiline> c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, R"("just","some","ran)");
        std::vector<std::string> expected{"just", "some", R"("just","some",")"};
        CHECK_EQ(words(vec), expected);
        REQUIRE(s.unterminated_quote());

        {
            auto new_line = buff.append(R"(,dom)");
            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.valid());
            CHECK(s.unterminated_quote());
            CHECK_EQ(words(vec), expected);
        }

        {
            auto new_line = buff.append(R"(",strings)");
            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.valid());
            CHECK_FALSE(s.unterminated_quote());
            expected = {"just", "some", "ran,dom", "strings"};
            CHECK_EQ(words(vec), expected);
        }
    }

    {
        ss::converter<ss::quote<'"'>, ss::escape<'\\'>, ss::multiline> c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, R"("just\"some","ra)");
        std::vector<std::string> expected{"just\"some"};
        auto w = words(vec);
        w.pop_back();
        CHECK_EQ(w, expected);
        REQUIRE(s.unterminated_quote());
        {
            auto new_line = buff.append(R"(n,dom",str\"ings)");
            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.valid());
            CHECK_FALSE(s.unterminated_quote());
            expected = {"just\"some", "ran,dom", "str\"ings"};
            CHECK_EQ(words(vec), expected);
        }
    }

    {
        ss::converter<ss::quote<'"'>, ss::escape<'\\'>, ss::multiline> c;
        auto& s = c.splitter;
        auto vec =
            expect_unterminated_quote(s, "3,4,"
                                         "\"just0\\\n1\\\n22\\\n33333x\\\n4");

        std::vector<std::string> expected{"3", "4"};
        auto w = words(vec);
        w.pop_back();
        CHECK_EQ(w, expected);
        REQUIRE(s.unterminated_quote());
        {
            auto new_line =
                buff.append_overwrite_last("\nx5strings\"", c.size_shifted());
            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.valid());
            CHECK_FALSE(s.unterminated_quote());
            expected = {"3", "4", "just0\n1\n22\n33333x\n4\nx5strings"};
            CHECK_EQ(words(vec), expected);
        }
    }
    {
        ss::converter<ss::quote<'"'>, ss::escape<'\\'>, ss::multiline> c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, R"("just\"some","ra"")");
        std::vector<std::string> expected{"just\"some"};
        auto w = words(vec);
        w.pop_back();
        CHECK_EQ(w, expected);
        REQUIRE(s.unterminated_quote());
        {
            auto new_line = buff.append_overwrite_last(R"(n,dom",str\"ings)",
                                                       c.size_shifted());
            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.valid());
            CHECK_FALSE(s.unterminated_quote());
            expected = {"just\"some", "ra\"n,dom", "str\"ings"};
            CHECK_EQ(words(vec), expected);
        }
    }

    {
        ss::converter<ss::quote<'"'>, ss::escape<'\\'>, ss::multiline> c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, R"("just\"some","r\a\a\\\a\")");
        std::vector<std::string> expected{"just\"some"};
        auto w = words(vec);
        w.pop_back();
        CHECK_EQ(w, expected);
        REQUIRE(s.unterminated_quote());
        {
            auto new_line = buff.append_overwrite_last(R"(n,dom",str\"ings)",
                                                       c.size_shifted());
            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.valid());
            CHECK_FALSE(s.unterminated_quote());
            expected = {"just\"some", "raa\\a\"n,dom", "str\"ings"};
            CHECK_EQ(words(vec), expected);
        }
    }

    {
        ss::converter<ss::quote<'"'>, ss::trim<' '>, ss::multiline> c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, R"(  "just" ,some,  "ra )");
        std::vector<std::string> expected{"just", "some"};
        auto w = words(vec);
        w.pop_back();
        CHECK_EQ(w, expected);
        REQUIRE(s.unterminated_quote());
        {
            auto new_line = buff.append(R"( n,dom"  , strings   )");
            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.valid());
            CHECK_FALSE(s.unterminated_quote());
            expected = {"just", "some", "ra  n,dom", "strings"};
            CHECK_EQ(words(vec), expected);
        }
    }

    {
        ss::converter<ss::quote<'"'>, ss::trim<' '>, ss::escape<'\\'>,
                      ss::multiline>
            c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, R"(  "ju\"st" ,some,  "ra \")");
        std::vector<std::string> expected{"ju\"st", "some"};
        auto w = words(vec);
        w.pop_back();
        CHECK_EQ(w, expected);
        REQUIRE(s.unterminated_quote());
        {
            auto new_line =
                buff.append_overwrite_last(R"( n,dom"  , strings   )",
                                           c.size_shifted());
            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.valid());
            CHECK_FALSE(s.unterminated_quote());
            expected = {"ju\"st", "some", "ra \" n,dom", "strings"};
            CHECK_EQ(words(vec), expected);
        }
    }

    {
        ss::converter<ss::quote<'"'>, ss::escape<'\\'>, ss::multiline> c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, R"("just\"some","ra)");
        std::vector<std::string> expected{"just\"some"};
        auto w = words(vec);
        w.pop_back();
        CHECK_EQ(w, expected);
        REQUIRE(s.unterminated_quote());
        {
            auto new_line = buff.append(R"(n,dom",str\"ings)");
            // invalid resplit size
            vec = c.resplit(new_line, 4);
            CHECK(!s.valid());
        }
    }
}

TEST_CASE("splitter test resplit unterminated quote with exceptions") {

    try {
        ss::converter<ss::quote<'"'>, ss::multiline, ss::escape<'\\'>,
                      ss::throw_on_error>
            c;
        auto& s = c.splitter;

        auto vec = expect_unterminated_quote(s, R"("x)");

        CHECK_EQ(vec.size(), 1);
        REQUIRE(s.unterminated_quote());

        {
            auto new_linet =
                buff.append_overwrite_last(R"(a\x)", c.size_shifted());

            vec = c.resplit(new_linet, strlen(new_linet));

            CHECK(s.unterminated_quote());
            CHECK_EQ(vec.size(), 1);
        }

        {
            auto new_linet =
                buff.append_overwrite_last(R"(")", c.size_shifted());

            vec = c.resplit(new_linet, strlen(new_linet));
            REQUIRE(s.valid());
            CHECK_FALSE(s.unterminated_quote());
            REQUIRE_EQ(vec.size(), 1);
            CHECK_EQ(words(vec)[0], "xax");
        }
    } catch (ss::exception& e) {
        FAIL(std::string{e.what()});
    }

    try {
        ss::converter<ss::quote<'"'>, ss::multiline, ss::throw_on_error> c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, "\"just");
        CHECK_EQ(vec.size(), 1);

        auto new_line = buff.append(R"(",strings)");
        vec = c.resplit(new_line, strlen(new_line));
        CHECK(s.valid());
        CHECK_FALSE(s.unterminated_quote());
        std::vector<std::string> expected{"just", "strings"};
        CHECK_EQ(words(vec), expected);
    } catch (ss::exception& e) {
        FAIL(std::string{e.what()});
    }

    try {
        ss::converter<ss::quote<'"'>, ss::multiline, ss::throw_on_error> c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, "just,some,\"random");
        std::vector<std::string> expected{"just", "some", "just,some,\""};
        CHECK_EQ(words(vec), expected);

        auto new_line = buff.append(R"(",strings)");
        vec = c.resplit(new_line, strlen(new_line));
        CHECK(s.valid());
        CHECK_FALSE(s.unterminated_quote());
        expected = {"just", "some", "random", "strings"};
        CHECK_EQ(words(vec), expected);
    } catch (ss::exception& e) {
        FAIL(std::string{e.what()});
    }

    try {
        ss::converter<ss::quote<'"'>, ss::multiline, ss::throw_on_error> c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, R"("just","some","ran"")");
        std::vector<std::string> expected{"just", "some", R"("just","some",")"};
        CHECK_EQ(words(vec), expected);

        auto new_line =
            buff.append_overwrite_last(R"(,dom","strings")", c.size_shifted());
        vec = c.resplit(new_line, strlen(new_line));
        CHECK(s.valid());
        CHECK_FALSE(s.unterminated_quote());
        expected = {"just", "some", "ran\",dom", "strings"};
        CHECK_EQ(words(vec), expected);
    } catch (ss::exception& e) {
        FAIL(std::string{e.what()});
    }

    try {
        ss::converter<ss::quote<'"'>, ss::multiline, ss::throw_on_error> c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, R"("just","some","ran)");
        std::vector<std::string> expected{"just", "some", R"("just","some",")"};
        CHECK_EQ(words(vec), expected);
        REQUIRE(s.unterminated_quote());

        {
            auto new_line = buff.append(R"(,dom)");
            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.valid());
            CHECK(s.unterminated_quote());
            CHECK_EQ(words(vec), expected);
        }

        {
            auto new_line = buff.append(R"(",strings)");
            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.valid());
            CHECK_FALSE(s.unterminated_quote());
            expected = {"just", "some", "ran,dom", "strings"};
            CHECK_EQ(words(vec), expected);
        }
    } catch (ss::exception& e) {
        FAIL(std::string{e.what()});
    }

    try {
        ss::converter<ss::quote<'"'>, ss::escape<'\\'>, ss::multiline,
                      ss::throw_on_error>
            c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, R"("just\"some","ra)");
        std::vector<std::string> expected{"just\"some"};
        auto w = words(vec);
        w.pop_back();
        CHECK_EQ(w, expected);
        REQUIRE(s.unterminated_quote());
        {
            auto new_line = buff.append(R"(n,dom",str\"ings)");
            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.valid());
            CHECK_FALSE(s.unterminated_quote());
            expected = {"just\"some", "ran,dom", "str\"ings"};
            CHECK_EQ(words(vec), expected);
        }
    } catch (ss::exception& e) {
        FAIL(std::string{e.what()});
    }

    try {
        ss::converter<ss::quote<'"'>, ss::escape<'\\'>, ss::multiline,
                      ss::throw_on_error>
            c;
        auto& s = c.splitter;
        auto vec =
            expect_unterminated_quote(s, "3,4,"
                                         "\"just0\\\n1\\\n22\\\n33333x\\\n4");

        std::vector<std::string> expected{"3", "4"};
        auto w = words(vec);
        w.pop_back();
        CHECK_EQ(w, expected);
        REQUIRE(s.unterminated_quote());
        {
            auto new_line =
                buff.append_overwrite_last("\nx5strings\"", c.size_shifted());
            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.valid());
            CHECK_FALSE(s.unterminated_quote());
            expected = {"3", "4", "just0\n1\n22\n33333x\n4\nx5strings"};
            CHECK_EQ(words(vec), expected);
        }
    } catch (ss::exception& e) {
        FAIL(std::string{e.what()});
    }

    try {
        ss::converter<ss::quote<'"'>, ss::escape<'\\'>, ss::multiline,
                      ss::throw_on_error>
            c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, R"("just\"some","ra"")");
        std::vector<std::string> expected{"just\"some"};
        auto w = words(vec);
        w.pop_back();
        CHECK_EQ(w, expected);
        REQUIRE(s.unterminated_quote());
        {
            auto new_line = buff.append_overwrite_last(R"(n,dom",str\"ings)",
                                                       c.size_shifted());
            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.valid());
            CHECK_FALSE(s.unterminated_quote());
            expected = {"just\"some", "ra\"n,dom", "str\"ings"};
            CHECK_EQ(words(vec), expected);
        }
    } catch (ss::exception& e) {
        FAIL(std::string{e.what()});
    }

    try {
        ss::converter<ss::quote<'"'>, ss::escape<'\\'>, ss::multiline,
                      ss::throw_on_error>
            c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, R"("just\"some","r\a\a\\\a\")");
        std::vector<std::string> expected{"just\"some"};
        auto w = words(vec);
        w.pop_back();
        CHECK_EQ(w, expected);
        REQUIRE(s.unterminated_quote());
        {
            auto new_line = buff.append_overwrite_last(R"(n,dom",str\"ings)",
                                                       c.size_shifted());
            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.valid());
            CHECK_FALSE(s.unterminated_quote());
            expected = {"just\"some", "raa\\a\"n,dom", "str\"ings"};
            CHECK_EQ(words(vec), expected);
        }
    } catch (ss::exception& e) {
        FAIL(std::string{e.what()});
    }

    try {
        ss::converter<ss::quote<'"'>, ss::trim<' '>, ss::multiline,
                      ss::throw_on_error>
            c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, R"(  "just" ,some,  "ra )");
        std::vector<std::string> expected{"just", "some"};
        auto w = words(vec);
        w.pop_back();
        CHECK_EQ(w, expected);
        REQUIRE(s.unterminated_quote());
        {
            auto new_line = buff.append(R"( n,dom"  , strings   )");
            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.valid());
            CHECK_FALSE(s.unterminated_quote());
            expected = {"just", "some", "ra  n,dom", "strings"};
            CHECK_EQ(words(vec), expected);
        }
    } catch (ss::exception& e) {
        FAIL(std::string{e.what()});
    }

    try {
        ss::converter<ss::quote<'"'>, ss::trim<' '>, ss::escape<'\\'>,
                      ss::multiline>
            c;
        auto& s = c.splitter;
        auto vec = expect_unterminated_quote(s, R"(  "ju\"st" ,some,  "ra \")");
        std::vector<std::string> expected{"ju\"st", "some"};
        auto w = words(vec);
        w.pop_back();
        CHECK_EQ(w, expected);
        REQUIRE(s.unterminated_quote());
        {
            auto new_line =
                buff.append_overwrite_last(R"( n,dom"  , strings   )",
                                           c.size_shifted());
            vec = c.resplit(new_line, strlen(new_line));
            CHECK(s.valid());
            CHECK_FALSE(s.unterminated_quote());
            expected = {"ju\"st", "some", "ra \" n,dom", "strings"};
            CHECK_EQ(words(vec), expected);
        }
    } catch (ss::exception& e) {
        FAIL(std::string{e.what()});
    }
}

template <typename... Ts>
void test_invalid_splits() {
    ss::converter<ss::quote<'"'>, ss::trim<' '>, ss::escape<'\\'>, Ts...> c;
    auto& s = c.splitter;

    auto check_error_msg = [&] {
        if constexpr (ss::setup<Ts...>::string_error) {
            CHECK_FALSE(s.error_msg().empty());
        }
    };

    // empty delimiter
    s.split(buff("some,random,strings"), "");
    CHECK_FALSE(s.valid());
    CHECK_FALSE(s.unterminated_quote());
    check_error_msg();

    // mismatched delimiter
    s.split(buff(R"(some,"random,"strings")"));
    CHECK_FALSE(s.valid());
    CHECK_FALSE(s.unterminated_quote());
    check_error_msg();

    // unterminated escape
    s.split(buff(R"(some,random,strings\)"));
    CHECK_FALSE(s.valid());
    CHECK_FALSE(s.unterminated_quote());
    check_error_msg();

    // unterminated escape
    s.split(buff(R"(some,random,"strings\)"));
    CHECK_FALSE(s.valid());
    CHECK_FALSE(s.unterminated_quote());
    check_error_msg();

    // unterminated quote
    s.split(buff("some,random,\"strings"));
    CHECK_FALSE(s.valid());
    CHECK(s.unterminated_quote());
    check_error_msg();

    // invalid resplit
    char new_line[] = "some";
    c.resplit(new_line, strlen(new_line));
    CHECK_FALSE(s.valid());
    check_error_msg();
}

TEST_CASE("splitter test invalid splits") {
    test_invalid_splits();
    test_invalid_splits<ss::string_error>();
}

TEST_CASE("splitter test invalid splits with exceptions") {
    ss::converter<ss::throw_on_error, ss::quote<'"'>, ss::trim<' '>,
                  ss::escape<'\\'>>
        c;
    auto& s = c.splitter;

    // empty delimiter
    REQUIRE_EXCEPTION(s.split(buff("some,random,strings"), ""));
    CHECK_FALSE(s.unterminated_quote());

    // mismatched delimiter
    REQUIRE_EXCEPTION(s.split(buff(R"(some,"random,"strings")")));
    CHECK_FALSE(s.unterminated_quote());

    // unterminated escape
    REQUIRE_EXCEPTION(s.split(buff(R"(some,random,strings\)")));
    CHECK_FALSE(s.unterminated_quote());

    // unterminated escape
    REQUIRE_EXCEPTION(s.split(buff(R"(some,random,"strings\)")));
    CHECK_FALSE(s.unterminated_quote());

    // unterminated quote
    REQUIRE_EXCEPTION(s.split(buff("some,random,\"strings")));
    CHECK(s.unterminated_quote());

    // invalid resplit
    char new_line[] = "some";
    REQUIRE_EXCEPTION(c.resplit(new_line, strlen(new_line)));
}

TEST_CASE("splitter test with trim_left") {
    auto guard = set_num_combinations(3);
    case_type case1 = spaced_left({R"(x )"}, " ");
    case_type case2 = spaced_left({R"(yy )"}, " ");
    case_type case3 = spaced_left({R"(y y )"}, " ");
    case_type case4 = spaced_left({R"()"}, " ");

    std::vector<std::string> delims = {",", "::", "\t", "\n"};

    {
        matches_type p{{case1, "x "},
                       {case2, "yy "},
                       {case3, "y y "},
                       {case4, ""}};
        test_combinations<ss::trim_left<' '>>(p, delims);
    }

    case_type case5 = spaced_left({"z "}, "\t");
    case_type case6 = spaced_left({"ab\t "}, " \t");
    case_type case7 = spaced_left({"a\tb "}, " \t");
    case_type case8 = spaced_left({"a \t b "}, " \t");

    {
        matches_type p{{case1, "x "},    {case2, "yy "},    {case3, "y y "},
                       {case4, ""},      {case5, "z "},     {case6, "ab\t "},
                       {case7, "a\tb "}, {case8, "a \t b "}};
        test_combinations<ss::trim_left<' ', '\t'>>(p, {",", "::", "\n"});
    }
}

TEST_CASE("splitter test with trim_right") {
    auto guard = set_num_combinations(3);
    case_type case1 = spaced_right({R"( x)"}, " ");
    case_type case2 = spaced_right({R"( yy)"}, " ");
    case_type case3 = spaced_right({R"( y y)"}, " ");
    case_type case4 = spaced_right({R"()"}, " ");

    std::vector<std::string> delims = {",", "::", "\t", "\n"};

    {
        matches_type p{{case1, " x"},
                       {case2, " yy"},
                       {case3, " y y"},
                       {case4, ""}};
        test_combinations<ss::trim_right<' '>>(p, delims);
    }

    case_type case5 = spaced_right({" z"}, "\t");
    case_type case6 = spaced_right({"\t ab"}, " \t");
    case_type case7 = spaced_right({"\ta\tb"}, " \t");
    case_type case8 = spaced_right({" \t a \t b"}, " \t");

    {
        matches_type p{{case1, " x"},     {case2, " yy"},
                       {case3, " y y"},   {case4, ""},
                       {case5, " z"},     {case6, "\t ab"},
                       {case7, "\ta\tb"}, {case8, " \t a \t b"}};
        test_combinations<ss::trim_right<' ', '\t'>>(p, {",", "::", "\n"});
    }
}

TEST_CASE("splitter test with trim_right and trim_left") {
    auto guard = set_num_combinations(3);
    case_type case1 = spaced_right({R"(-x)"}, "-");
    case_type case2 = spaced_left({R"(yy_)"}, "_");
    case_type case3 = spaced_right({R"(-y y)"}, "-");
    case_type case4 = spaced_left({R"()"}, "-");
    case_type case5 = spaced_left({R"()"}, "_");
    case_type case6 = {"___---", "_-", "______-"};

    std::vector<std::string> delims = {",", "::", "\t", "\n"};

    {
        matches_type p{{case1, "-x"}, {case2, "yy_"}, {case3, "-y y"},
                       {case4, ""},   {case5, ""},    {case6, ""}};
        test_combinations<ss::trim_left<'_'>, ss::trim_right<'-'>>(p, delims);
    }
}

TEST_CASE("splitter test with quote and escape, trim_left and trim_right") {
    auto guard = set_num_combinations(3);
    case_type case1 = spaced_left({R"("\"")", R"(\")", R"("""")"}, "_");
    case_type case2 =
        spaced_left({R"("x\"x")", R"(x\"x)", R"(x"x)", R"("x""x")"}, "_");
    case_type case3 = spaced_left({R"("")", R"()"}, "_");
    case_type case4 = spaced_left({R"("x")", R"(x)"}, "_");
    case_type case5 =
        spaced_right({R"("\"\"")", R"("""""")", R"("\"""")", R"("""\"")"}, "-");
    case_type case6 = spaced_right({R"("\\")", R"(\\)"}, "-");
    case_type case7 = spaced_right({R"("xxxxxxxxxx")", R"(xxxxxxxxxx)"}, "-");

    std::vector<std::string> delims = {"::", "\n"};

    {
        matches_type p{{case1, "\""},   {case2, "x\"x"}, {case3, ""},
                       {case5, "\"\""}, {case6, "\\"},   {case7, "xxxxxxxxxx"}};
        test_combinations<ss::quote<'"'>, ss::escape<'\\'>, ss::trim_left<'_'>,
                          ss::trim_right<'-'>>(p, delims);
    }
}
