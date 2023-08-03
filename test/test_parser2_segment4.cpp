#include "test_parser2.hpp"

TEST_CASE("parser test various cases version 2 segment 4") {
    using quote = ss::quote<'"'>;
    using escape = ss::escape<'\\'>;
    using multiline = ss::multiline;

#ifdef CMAKE_GITHUB_CI
    using trimr = ss::trim_right<' '>;
    using triml = ss::trim_left<' '>;

    test_option_combinations<escape, quote, multiline, triml>();
    test_option_combinations<escape, quote, multiline, trimr>();
#else
    test_option_combinations3<escape, quote, multiline>();
#endif
}

