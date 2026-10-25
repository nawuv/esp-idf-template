#pragma once

#include "domain/policy_defaults.hpp"

namespace domain {

struct OutputPolicy {
    Milliseconds max_run_ms{policy_defaults::default_max_run_ms};
    Milliseconds max_sample_age_ms{policy_defaults::default_max_sample_age_ms};
};

enum class OutputState {
    off,
    running,
    fault
};

// Single-owner state machine. Repeated ON requests cannot extend a running interval.
class TimedOutput {
    public:
    explicit TimedOutput(OutputPolicy policy) noexcept;
    bool update(bool requested, Reading reading, Milliseconds now) noexcept;
    bool reset_fault() noexcept;

    void trip_fault() noexcept {
        state_ = OutputState::fault;
    }

    bool fault() const noexcept {
        return state_ == OutputState::fault;
    }

    OutputState state() const noexcept {
        return state_;
    }

    private:
    OutputPolicy policy_;
    Milliseconds started_at_{0};
    Milliseconds last_update_{0};
    bool has_update_{false};
    OutputState state_{OutputState::off};
};

} // namespace domain
