#pragma once

#include "espkit/core/io.hpp"

namespace espkit {

// Boot-lifetime adapter. begin() is explicit; invalid/unset pins never touch GPIO.
class EspI2cDevice final : public I2cDevice {
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

class FreeRtosDelay final : public Delay {
    public:
    void sleep_for(Milliseconds duration) noexcept override;
};

} // namespace espkit
