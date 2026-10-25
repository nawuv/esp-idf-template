#pragma once

#include "iot/core/ports.hpp"

namespace iot {

class SyntheticSensor final : public SensorSource {
    public:
    explicit SyntheticSensor(float value) noexcept : value_(value) {
    }

    Reading read(Milliseconds now) noexcept override {
        return {value_, now, true};
    }

    private:
    float value_;
};

} // namespace iot
