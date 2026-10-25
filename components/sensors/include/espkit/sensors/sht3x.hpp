#pragma once

#include "espkit/core/io.hpp"
#include "espkit/core/ports.hpp"

#include <span>

namespace espkit {

struct EnvironmentSample {
    float temperature_celsius{0};
    float relative_humidity_percent{0};
    bool valid{false};
};

// SHT30/31/35-DIS single-shot, high repeatability, no clock stretching.
EnvironmentSample decode_sht3x(std::span<const std::uint8_t> frame) noexcept;

class Sht3xSensor final : public SensorSource {
    public:
    Sht3xSensor(I2cDevice& bus, Delay& delay) : bus_(bus), delay_(delay) {
    }

    Reading read(Milliseconds now) noexcept override;

    private:
    I2cDevice& bus_;
    Delay& delay_;
};

} // namespace espkit
