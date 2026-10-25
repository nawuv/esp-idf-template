#pragma once

#include "application/ports/device.hpp"

namespace drivers {

class SyntheticSensor final : public application::SensorSource {
    public:
    explicit SyntheticSensor(float value) noexcept : value_(value) {
    }

    domain::Reading read(domain::Milliseconds now) noexcept override {
        return {value_, now, true, domain::MeasurementUnit::unspecified,
                domain::DataOrigin::synthetic};
    }

    private:
    float value_;
};

} // namespace drivers
