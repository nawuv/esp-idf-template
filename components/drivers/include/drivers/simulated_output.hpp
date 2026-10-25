#pragma once

#include "application/ports/device.hpp"

namespace drivers {

// Records state in memory. Never accesses GPIO or physical feedback.
class SimulatedOutput final : public application::OutputSink {
    public:
    domain::OutputError set_enabled(bool enabled) noexcept override {
        enabled_ = enabled;
        return domain::OutputError::none;
    }

    bool enabled() const noexcept {
        return enabled_;
    }

    private:
    bool enabled_{false};
};

} // namespace drivers
