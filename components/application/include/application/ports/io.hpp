#pragma once

#include "domain/types.hpp"

#include <cstdint>
#include <span>

namespace application {

class I2cDevice {
    public:
    virtual ~I2cDevice() = default;
    virtual bool write(std::span<const std::uint8_t> bytes) noexcept = 0;
    virtual bool read(std::span<std::uint8_t> bytes) noexcept = 0;
};

class Delay {
    public:
    virtual ~Delay() = default;
    virtual void sleep_for(domain::Milliseconds duration) noexcept = 0;
};

} // namespace application
