#include "drivers/sht3x.hpp"

#include <array>

namespace drivers {

namespace {

constexpr std::uint8_t crc_initial = 0xff;
constexpr std::uint8_t crc_polynomial = 0x31;
constexpr domain::Milliseconds conversion_wait_ms = 20;
constexpr std::array<std::uint8_t, 2> measurement_command{0x24, 0x00};

std::uint8_t crc(std::span<const std::uint8_t> bytes) noexcept {
    auto value = crc_initial;
    for (const auto byte : bytes) {
        value ^= byte;
        for (unsigned bit = 0; bit < 8; ++bit) {
            value = static_cast<std::uint8_t>((value & 0x80) ? (value << 1) ^ crc_polynomial
                                                             : value << 1);
        }
    }
    return value;
}

} // namespace

EnvironmentSample decode_sht3x(std::span<const std::uint8_t> frame) noexcept {
    if (frame.size() != 6 || crc(frame.first(2)) != frame[2] ||
        crc(frame.subspan(3, 2)) != frame[5]) {
        return {};
    }
    const auto temperature = static_cast<unsigned>((frame[0] << 8) | frame[1]);
    const auto humidity = static_cast<unsigned>((frame[3] << 8) | frame[4]);
    return {-45.0F + 175.0F * temperature / 65535.0F, 100.0F * humidity / 65535.0F, true};
}

domain::Reading Sht3xSensor::read(domain::Milliseconds now) noexcept {
    domain::Reading result{0, now, false, domain::MeasurementUnit::celsius,
                           domain::DataOrigin::physical};
    if (!bus_.write(measurement_command)) {
        return result;
    }
    delay_.sleep_for(conversion_wait_ms);
    std::array<std::uint8_t, 6> frame{};
    if (!bus_.read(frame)) {
        return result;
    }
    const auto sample = decode_sht3x(frame);
    return {sample.temperature_celsius, now, sample.valid, domain::MeasurementUnit::celsius,
            domain::DataOrigin::physical};
}

} // namespace drivers
