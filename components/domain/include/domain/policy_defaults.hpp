#pragma once

#include "domain/types.hpp"

namespace domain::policy_defaults {

inline constexpr Milliseconds default_max_run_ms = 5000;
inline constexpr Milliseconds default_max_sample_age_ms = 2000;

} // namespace domain::policy_defaults
