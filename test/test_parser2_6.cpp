#define SEGMENT_NAME "segment6"
#include "test_parser2.hpp"

TEST_CASE("parser test various cases version 2 segment 6") {
    using quote = ss::quote<'"'>;
    using escape = ss::escape<'\\'>;
    using multiline = ss::multiline;

    test_option_combinations3<escape, quote, multiline>();
}

