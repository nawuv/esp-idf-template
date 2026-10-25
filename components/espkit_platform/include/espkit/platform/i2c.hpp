#pragma once

#include "espkit/application/ports/io.hpp"

namespace espkit::platform {

// Boot-lifetime adapter. begin() is explicit; invalid/unset pins never touch GPIO.
class EspI2cDevice final : public espkit::application::I2cDevice {
    public:
    ~EspI2cDevice() override;
    EspI2cDevice() = default;
    EspI2cDevice(const EspI2cDevice&) = delete;
    EspI2cDevice& operator=(const EspI2cDevice&) = delete;
    bool begin(int sda, int scl, unsigned address, unsigned clock_hz, int timeout_ms) noexcept;
    bool write(std::span<const std::uint8_t> bytes) noexcept override;
    bool read(std::span<std::uint8_t> bytes) noexcept override;

    private:
    void* bus_{nullptr};
    void* device_{nullptr};
    int timeout_ms_{0};
};

class FreeRtosDelay final : public espkit::application::Delay {
    public:
    void sleep_for(espkit::domain::Milliseconds duration) noexcept override;
};

} // namespace espkit::platform
