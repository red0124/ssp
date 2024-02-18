#define SEGMENT_NAME "segment4"
#include "test_parser2.hpp"

TEST_CASE("parser test various cases version 2 segment 4") {
#ifdef CMAKE_GITHUB_CI
    using quote = ss::quote<'"'>;
    using escape = ss::escape<'\\'>;
    using multiline = ss::multiline;
    using multiline_r = ss::multiline_restricted<10>;

    test_option_combinations3<escape, quote, multiline>();
    test_option_combinations3<escape, quote, multiline_r>();
#endif
}

