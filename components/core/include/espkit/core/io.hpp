#pragma once

#include "espkit/core/types.hpp"

#include <cstdint>
#include <span>

namespace espkit {

class I2cDevice {
    public:
    virtual ~I2cDevice() = default;
    virtual bool write(std::span<const std::uint8_t> bytes) noexcept = 0;
    virtual bool read(std::span<std::uint8_t> bytes) noexcept = 0;
};

class Delay {
    public:
    virtual ~Delay() = default;
    virtual void sleep_for(Milliseconds duration) noexcept = 0;
};

} // namespace espkit
