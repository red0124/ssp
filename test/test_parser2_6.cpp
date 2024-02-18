#define SEGMENT_NAME "segment4"
#include "test_parser2.hpp"

TEST_CASE("parser test various cases version 2 segment 4") {
    using quote = ss::quote<'"'>;
    using escape = ss::escape<'\\'>;
    using multiline = ss::multiline;

#ifdef CMAKE_GITHUB_CI
    using trim = ss::trim<' '>;

    test_option_combinations3<escape, quote, multiline>();
    test_option_combinations3<escape, quote, multiline, trim>();
#else
    test_option_combinations3<escape, quote, multiline>();
#endif
}

