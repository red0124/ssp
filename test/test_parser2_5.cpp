#define SEGMENT_NAME "segment5"
#include "test_parser2.hpp"

TEST_CASE("parser test various cases version 2 segment 5") {
#ifdef CMAKE_GITHUB_CI
    using quote = ss::quote<'"'>;
    using escape = ss::escape<'\\'>;
    using multiline = ss::multiline;
    using trimr = ss::trim_right<' '>;
    using triml = ss::trim_left<' '>;

    test_option_combinations<escape, quote, multiline, triml>();
    test_option_combinations<escape, quote, multiline, trimr>();
#endif
}

