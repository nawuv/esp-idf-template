#pragma once

#include "iot/core/types.hpp"

namespace iot::policy_defaults {

inline constexpr Milliseconds default_max_run_ms = 5000;
inline constexpr Milliseconds default_max_sample_age_ms = 2000;

} // namespace iot::policy_defaults
