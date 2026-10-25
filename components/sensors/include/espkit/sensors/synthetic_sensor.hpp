#pragma once

#include "espkit/core/ports.hpp"

namespace espkit {

class SyntheticSensor final : public SensorSource {
    public:
    explicit SyntheticSensor(float value) noexcept : value_(value) {
    }

    Reading read(Milliseconds now) noexcept override {
        return {value_, now, true, MeasurementUnit::unspecified, DataOrigin::synthetic};
    }

    private:
    float value_;
};

} // namespace espkit
