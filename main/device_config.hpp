#pragma once

#include "domain/timed_output.hpp"
#include "domain/types.hpp"

#include <cstdint>

namespace device::defaults {

// Deployment defaults and FreeRTOS task budgets are owned by this device.
inline constexpr std::uint32_t sensor_stack_bytes = 3072;
inline constexpr std::uint32_t control_stack_bytes = 4096;
inline constexpr std::uint32_t reporter_stack_bytes = 3072;
inline constexpr std::uint32_t network_stack_bytes = 8192;
inline constexpr unsigned network_priority = 2;
inline constexpr unsigned sensor_priority = 3;
inline constexpr unsigned control_priority = 4;
inline constexpr unsigned reporter_priority = 2;

inline constexpr domain::OutputPolicy output_policy{};
inline constexpr float synthetic_reading = 42.0F;
inline constexpr domain::Milliseconds network_period_ms = 1000;
inline constexpr domain::Milliseconds sensor_period_ms = 500;
inline constexpr domain::Milliseconds control_period_ms = 100;
inline constexpr domain::Milliseconds reporter_period_ms = 2000;

} // namespace device::defaults
