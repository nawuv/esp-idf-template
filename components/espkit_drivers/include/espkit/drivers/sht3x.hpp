#pragma once

#include "espkit/application/ports/device.hpp"
#include "espkit/application/ports/io.hpp"

#include <span>

namespace espkit::drivers {

struct EnvironmentSample {
    float temperature_celsius{0};
    float relative_humidity_percent{0};
    bool valid{false};
};

// SHT30/31/35-DIS single-shot, high repeatability, no clock stretching.
EnvironmentSample decode_sht3x(std::span<const std::uint8_t> frame) noexcept;

class Sht3xSensor final : public espkit::application::SensorSource {
    public:
    Sht3xSensor(espkit::application::I2cDevice& bus, espkit::application::Delay& delay)
        : bus_(bus), delay_(delay) {
    }

    espkit::domain::Reading read(espkit::domain::Milliseconds now) noexcept override;

    private:
    espkit::application::I2cDevice& bus_;
    espkit::application::Delay& delay_;
};

} // namespace espkit::drivers
