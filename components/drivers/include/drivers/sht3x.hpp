#pragma once

#include "application/ports/device.hpp"
#include "application/ports/io.hpp"

#include <span>

namespace drivers {

struct EnvironmentSample {
    float temperature_celsius{0};
    float relative_humidity_percent{0};
    bool valid{false};
};

// SHT30/31/35-DIS single-shot, high repeatability, no clock stretching.
EnvironmentSample decode_sht3x(std::span<const std::uint8_t> frame) noexcept;

class Sht3xSensor final : public application::SensorSource {
    public:
    Sht3xSensor(application::I2cDevice& bus, application::Delay& delay) : bus_(bus), delay_(delay) {
    }

    domain::Reading read(domain::Milliseconds now) noexcept override;

    private:
    application::I2cDevice& bus_;
    application::Delay& delay_;
};

} // namespace drivers
