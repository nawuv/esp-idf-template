#pragma once

#include "espkit/core/ports.hpp"

namespace espkit {

// Records state in memory. Never accesses GPIO or physical feedback.
class SimulatedOutput final : public OutputSink {
    public:
    OutputError set_enabled(bool enabled) noexcept override {
        enabled_ = enabled;
        return OutputError::none;
    }

    bool enabled() const noexcept {
        return enabled_;
    }

    private:
    bool enabled_{false};
};

} // namespace espkit
