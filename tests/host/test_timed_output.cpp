#include "espkit/domain/timed_output.hpp"

#include <gtest/gtest.h>
#include <limits>

namespace {

espkit::domain::Reading fresh(espkit::domain::Milliseconds now) {
    return {42.0F, now, true};
}

TEST(TimedOutput, StartsOffAndAcceptsLocalRequest) {
    espkit::domain::TimedOutput output({1000, 200});
    EXPECT_FALSE(output.update(false, fresh(0), 0));
    EXPECT_TRUE(output.update(true, fresh(100), 100));
    EXPECT_FALSE(output.update(false, fresh(150), 150));
}

TEST(TimedOutput, RepeatedRequestsDoNotExtendDeadline) {
    espkit::domain::TimedOutput output({1000, 200});
    ASSERT_TRUE(output.update(true, fresh(10), 10));
    EXPECT_TRUE(output.update(true, fresh(1009), 1009));
    EXPECT_FALSE(output.update(true, fresh(1010), 1010));
    EXPECT_TRUE(output.fault());
    EXPECT_FALSE(output.update(true, fresh(1011), 1011));
}

TEST(TimedOutput, StaleReadingStopsAndLatchesFault) {
    espkit::domain::TimedOutput output({1000, 200});
    ASSERT_TRUE(output.update(true, fresh(0), 0));
    EXPECT_FALSE(output.update(true, fresh(0), 201));
    EXPECT_FALSE(output.update(true, fresh(202), 202));
    ASSERT_TRUE(output.reset_fault());
    EXPECT_TRUE(output.update(true, fresh(203), 203));
}

TEST(TimedOutput, InvalidAndFutureSamplesFailClosed) {
    espkit::domain::TimedOutput invalid({1000, 200});
    EXPECT_FALSE(invalid.update(true, {42, 0, false}, 0));
    EXPECT_TRUE(invalid.fault());
    espkit::domain::TimedOutput future({1000, 200});
    EXPECT_FALSE(future.update(true, fresh(10), 0));
    EXPECT_TRUE(future.fault());
    espkit::domain::TimedOutput nan({1000, 200});
    EXPECT_FALSE(nan.update(true, {std::numeric_limits<float>::quiet_NaN(), 0, true}, 0));
    EXPECT_TRUE(nan.fault());
}

TEST(TimedOutput, ClockRegressionFailsClosed) {
    espkit::domain::TimedOutput output({1000, 200});
    ASSERT_TRUE(output.update(true, fresh(100), 100));
    EXPECT_FALSE(output.update(true, fresh(99), 99));
    EXPECT_TRUE(output.fault());
}

TEST(TimedOutput, FaultCannotBeResetWhileRunning) {
    espkit::domain::TimedOutput output({1000, 200});
    ASSERT_TRUE(output.update(true, fresh(0), 0));
    EXPECT_FALSE(output.reset_fault());
}

TEST(TimedOutput, InvalidPolicyFailsClosed) {
    espkit::domain::TimedOutput output({0, 200});
    EXPECT_FALSE(output.update(true, fresh(0), 0));
    EXPECT_TRUE(output.fault());
}

TEST(TimedOutput, SampleAgeBoundaryIsInclusive) {
    espkit::domain::TimedOutput output({1000, 200});
    EXPECT_TRUE(output.update(true, fresh(0), 200));
    EXPECT_FALSE(output.update(true, fresh(0), 201));
}

} // namespace
