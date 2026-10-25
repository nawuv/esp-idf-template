#include "espkit/domain/timed_output.hpp"
#include "unity.h"

TEST_CASE("timed output expires without extending the deadline", "[domain]") {
    espkit::domain::TimedOutput output({1000, 200});
    TEST_ASSERT_TRUE(output.update(true, {42.0F, 0, true}, 0));
    TEST_ASSERT_TRUE(output.update(true, {42.0F, 999, true}, 999));
    TEST_ASSERT_FALSE(output.update(true, {42.0F, 1000, true}, 1000));
    TEST_ASSERT_TRUE(output.fault());
}
