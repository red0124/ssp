#define SEGMENT_NAME "segment2"
#include "test_parser2.hpp"

TEST_CASE("parser test various cases version 2 segment 2") {
#ifdef CMAKE_GITHUB_CI
    using quote = ss::quote<'"'>;
    using escape = ss::escape<'\\'>;

    test_option_combinations3<quote>();
    test_option_combinations3<escape, quote>();
#endif
}

