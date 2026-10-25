#pragma once

#include <cstdint>

namespace espkit::platform::runtime_constants {

// ESP-IDF task stack sizes are expressed in bytes.
inline constexpr unsigned latest_snapshot_capacity = 1;
inline constexpr std::int64_t microseconds_per_millisecond = 1000;
inline constexpr std::uint64_t milliseconds_per_second = 1000;

} // namespace espkit::platform::runtime_constants
