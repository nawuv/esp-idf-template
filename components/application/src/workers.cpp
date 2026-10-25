#include "espkit/application/workers.hpp"

namespace espkit {

void SensorWorker::step() noexcept {
    readings_.publish(sensor_.read(clock_.now()));
}

void ControlWorker::step() noexcept {
    Reading reading{};
    if (!readings_.latest(reading)) {
        reading.valid = false;
    }
    const auto now = clock_.now();
    const bool requested = requests_.enable_requested(now);
    const bool enabled = policy_.update(requested, reading, now);
    auto error = output_.set_enabled(enabled);
    auto applied = enabled ? AppliedOutput::on : AppliedOutput::off;
    if (error != OutputError::none) {
        policy_.trip_fault();
        // Best-effort stop. Failure must never be reported as a confirmed OFF.
        const auto stop_error = output_.set_enabled(false);
        applied = stop_error == OutputError::none ? AppliedOutput::off : AppliedOutput::unknown;
        if (stop_error != OutputError::none) {
            error = stop_error;
        }
    }
    telemetry_.publish({now, requested, applied, policy_.fault(), error, reading});
}

void ReporterWorker::step() noexcept {
    Telemetry telemetry{};
    if (telemetry_.latest(telemetry)) {
        sink_.report(telemetry);
    }
}

} // namespace espkit
