#pragma once

#include <cstdint>

namespace iot::runtime_constants {

// ESP-IDF task stack sizes are expressed in bytes.
inline constexpr std::uint32_t sensor_stack_bytes = 3072;
inline constexpr std::uint32_t control_stack_bytes = 4096;
inline constexpr std::uint32_t reporter_stack_bytes = 3072;
inline constexpr unsigned sensor_priority = 3;
inline constexpr unsigned control_priority = 4;
inline constexpr unsigned reporter_priority = 2;
inline constexpr unsigned latest_snapshot_capacity = 1;
inline constexpr std::int64_t microseconds_per_millisecond = 1000;
inline constexpr std::uint64_t milliseconds_per_second = 1000;

} // namespace iot::runtime_constants
