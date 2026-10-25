#pragma once

#include "espkit/core/types.hpp"

namespace espkit::device_defaults {

// Deployment defaults belong here; runtime configuration may override them later.
inline constexpr float synthetic_reading = 42.0F;
inline constexpr Milliseconds sensor_period_ms = 500;
inline constexpr Milliseconds control_period_ms = 100;
inline constexpr Milliseconds reporter_period_ms = 2000;

} // namespace espkit::device_defaults
