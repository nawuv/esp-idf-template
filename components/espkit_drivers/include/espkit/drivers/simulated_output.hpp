#pragma once

#include "espkit/application/ports/device.hpp"

namespace espkit::drivers {

// Records state in memory. Never accesses GPIO or physical feedback.
class SimulatedOutput final : public espkit::application::OutputSink {
    public:
    espkit::domain::OutputError set_enabled(bool enabled) noexcept override {
        enabled_ = enabled;
        return espkit::domain::OutputError::none;
    }

    bool enabled() const noexcept {
        return enabled_;
    }

    private:
    bool enabled_{false};
};

} // namespace espkit::drivers
