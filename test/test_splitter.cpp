#include "test_helpers.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <ss/splitter.hpp>

// TODO make ss::quote accept only one argument

namespace {
constexpr static auto combinations_size_default = 4;
size_t combinations_size = combinations_size_default;

struct set_combinations_size {
    set_combinations_size(size_t size) {
        combinations_size = size;
    }
    ~set_combinations_size() {
        combinations_size = combinations_size_default;
    }
};

std::vector<std::string> words(const ss::split_input& input) {
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

std::vector<std::vector<std::string>> vector_combinations(
    const std::vector<std::string>& v, size_t n) {
    std::vector<std::vector<std::string>> ret;
    if (n <= 1) {
        for (const auto& i : v) {
            ret.push_back({i});
        }
        return ret;
    }

    auto inner_combinations = vector_combinations(v, n - 1);
    for (const auto& i : v) {
        for (auto j : inner_combinations) {
            j.insert(j.begin(), i);
            ret.push_back(move(j));
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
    for (size_t i = 0; i < combinations_size; ++i) {
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
void test_combinations(matches_type& matches, std::vector<std::string> delims) {

    ss::splitter<Matchers...> s;
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
            CHECK(words(vec) == expectations[i]);
        }
    }
}

TEST_CASE("testing splitter no setup") {
    {
        matches_type p{{{"x"}, "x"},        {{"\""}, "\""},
                       {{""}, ""},          {{"\n"}, "\n"},
                       {{"\"\""}, "\"\""},  {{"\" \\ \""}, "\" \\ \""},
                       {{"     "}, "     "}};
        test_combinations(p, {",", ";", "\t", "::"});
    }
}

TEST_CASE("testing splitter quote") {
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

TEST_CASE("testing splitter trim") {
    auto guard = set_combinations_size(3);
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

TEST_CASE("testing splitter escape") {
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

TEST_CASE("testing splitter quote and trim") {
    auto guard = set_combinations_size(3);
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

TEST_CASE("testing splitter quote and escape") {
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

TEST_CASE("testing splitter escape and trim") {
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

TEST_CASE("testing splitter quote and escape and trim") {
    auto guard = set_combinations_size(3);
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
