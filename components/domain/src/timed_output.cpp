#include "domain/timed_output.hpp"

#include <cmath>

namespace domain {

TimedOutput::TimedOutput(OutputPolicy policy) noexcept : policy_(policy) {
}

bool TimedOutput::update(bool requested, Reading reading, Milliseconds now) noexcept {
    const bool invalid = policy_.max_run_ms <= 0 || policy_.max_sample_age_ms <= 0 || now < 0 ||
                         (has_update_ && now < last_update_) || !reading.valid ||
                         !std::isfinite(reading.value) || reading.sampled_at < 0 ||
                         reading.sampled_at > now ||
                         now - reading.sampled_at > policy_.max_sample_age_ms;
    has_update_ = true;
    last_update_ = now;
    if (invalid || (state_ == OutputState::running && now - started_at_ >= policy_.max_run_ms)) {
        trip_fault();
    }
    if (fault()) {
        return false;
    }
    if (!requested) {
        state_ = OutputState::off;
        return false;
    }
    if (state_ == OutputState::off) {
        started_at_ = now;
    }
    state_ = OutputState::running;
    return true;
}

bool TimedOutput::reset_fault() noexcept {
    if (state_ == OutputState::running) {
        return false;
    }
    state_ = OutputState::off;
    return true;
}

} // namespace domain
