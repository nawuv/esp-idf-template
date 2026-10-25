#pragma once

#include "iot/core/types.hpp"

namespace iot::device_defaults {

// Deployment defaults belong here; runtime configuration may override them later.
inline constexpr float synthetic_reading = 42.0F;
inline constexpr Milliseconds sensor_period_ms = 500;
inline constexpr Milliseconds control_period_ms = 100;
inline constexpr Milliseconds reporter_period_ms = 2000;

} // namespace iot::device_defaults
