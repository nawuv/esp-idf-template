#pragma once

#include "application/ports/device.hpp"

namespace platform {

class LogTelemetry final : public application::TelemetrySink {
    public:
    // The label is borrowed and must outlive this adapter.
    explicit LogTelemetry(const char* output_mode) : output_mode_(output_mode) {
    }

    void report(const domain::Telemetry& value) noexcept override;

    private:
    const char* output_mode_;
};

} // namespace platform
