#pragma once

#include <cstdint>

namespace domain {

using Milliseconds = std::int64_t;

enum class MeasurementUnit {
    unspecified,
    celsius,
    relative_humidity_percent
};

enum class DataOrigin {
    unspecified,
    synthetic,
    physical
};

struct Reading {
    float value;
    Milliseconds sampled_at;
    bool valid;
    MeasurementUnit unit{MeasurementUnit::unspecified};
    DataOrigin origin{DataOrigin::unspecified};
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
    Reading reading{};
};

} // namespace domain
