#pragma once

#include "espkit/core/types.hpp"

namespace espkit::policy_defaults {

inline constexpr Milliseconds default_max_run_ms = 5000;
inline constexpr Milliseconds default_max_sample_age_ms = 2000;

} // namespace espkit::policy_defaults
