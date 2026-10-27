#include "domain/command.hpp"
#include "domain/timed_output.hpp"
#include "unity.h"

TEST_CASE("timed output expires without extending the deadline", "[domain]") {
    domain::TimedOutput output({1000, 200});
    TEST_ASSERT_TRUE(output.update(true, {42.0F, 0, true}, 0));
    TEST_ASSERT_TRUE(output.update(true, {42.0F, 999, true}, 999));
    TEST_ASSERT_FALSE(output.update(true, {42.0F, 1000, true}, 1000));
    TEST_ASSERT_TRUE(output.fault());
}

TEST_CASE("command validation rejects expired, over-long, and remote requests", "[domain]") {
    const domain::CommandLimits limits{1000, 15000};
    domain::Command command{};
    command.id = 1;
    command.sequence = 1;
    command.identity = 7;
    command.type = domain::CommandType::set_output;
    command.source = domain::CommandSource::local;
    command.enabled = true;
    command.expires_at = 500;
    command.max_duration_ms = 400;
    TEST_ASSERT_EQUAL(static_cast<int>(domain::CommandValidation::ok),
                      static_cast<int>(domain::validate_command(command, 0, limits, false)));
    command.expires_at = 0;
    TEST_ASSERT_EQUAL(static_cast<int>(domain::CommandValidation::expired),
                      static_cast<int>(domain::validate_command(command, 0, limits, false)));
    command.expires_at = 500;
    command.max_duration_ms = 1000;
    TEST_ASSERT_EQUAL(static_cast<int>(domain::CommandValidation::invalid),
                      static_cast<int>(domain::validate_command(command, 0, limits, false)));
    command.max_duration_ms = 400;
    command.source = domain::CommandSource::remote;
    TEST_ASSERT_EQUAL(static_cast<int>(domain::CommandValidation::unauthorized),
                      static_cast<int>(domain::validate_command(command, 0, limits, false)));
}
