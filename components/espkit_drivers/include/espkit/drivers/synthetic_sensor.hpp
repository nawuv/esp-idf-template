#pragma once

#include "espkit/application/ports/device.hpp"

namespace espkit::drivers {

class SyntheticSensor final : public espkit::application::SensorSource {
    public:
    explicit SyntheticSensor(float value) noexcept : value_(value) {
    }

    espkit::domain::Reading read(espkit::domain::Milliseconds now) noexcept override {
        return {value_, now, true, espkit::domain::MeasurementUnit::unspecified,
                espkit::domain::DataOrigin::synthetic};
    }

    private:
    float value_;
};

} // namespace espkit::drivers
