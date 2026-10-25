#pragma once

#include <cstdint>

namespace iot {

using Milliseconds = std::int64_t;

struct Reading {
    float value;
    Milliseconds sampled_at;
    bool valid;
};

enum class OutputError {
    none,
    unavailable,
    hardware_fault
};
// Software acknowledgement, not physical feedback from the actuator.
enum class AppliedOutput {
    unknown,
    off,
    on
};

struct Telemetry {
    Milliseconds uptime;
    bool output_requested;
    AppliedOutput applied_output;
    bool fault;
    OutputError output_error;
};

} // namespace iot
