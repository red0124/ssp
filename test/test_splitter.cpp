#include <iostream>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../include/ss/splitter.hpp"
#include "doctest.h"
#include <algorithm>

TEST_CASE("testing splitter with escaping") {
    std::vector<std::string> values{"10",   "he\\\"llo",
                                    "\\\"", "\\\"a\\,a\\\"",
                                    "3.33", "a\\\""};

    char buff[128];
    // with quote
    ss::splitter<ss::quote<'"'>, ss::escape<'\\'>> s;
    std::string delim = ",";
    for (size_t i = 0; i < values.size() * values.size(); ++i) {
        std::string input1;
        std::string input2;
        for (size_t j = 0; j < values.size(); ++j) {
            if (i & (1 << j) && j != 2 && j != 3) {
                input1.append(values[j]);
                input2.append(values.at(values.size() - 1 - j));
            } else {
                input1.append("\"" + values[j] + "\"");
                input2.append("\"" + values.at(values.size() - 1 - j) + "\"");
            }
            input1.append(delim);
            input2.append(delim);
        }
        input1.pop_back();
        input2.pop_back();
        input1.append("\0\"");
        input2.append("\0\"");

        memcpy(buff, input1.c_str(), input1.size() + 1);
        auto tup1 = s.split(buff, delim);
        CHECK(tup1.size() == 6);

        memcpy(buff, input2.c_str(), input2.size() + 1);
        auto tup2 = s.split(buff, delim);
        CHECK(tup2.size() == 6);
    }
}
