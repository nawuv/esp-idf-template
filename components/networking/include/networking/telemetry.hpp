#pragma once

#include "domain/types.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace networking {

// Returns payload length (excluding NUL), or zero for invalid/insufficient storage.
std::size_t encode_telemetry(const domain::Telemetry& status, std::span<char> buffer) noexcept;

class ReconnectBackoff {
    public:
    std::uint32_t next_delay(std::uint32_t random) noexcept;

    void reset() noexcept {
        attempt_ = 0;
    }

    private:
    unsigned attempt_{0};
};

} // namespace networking
