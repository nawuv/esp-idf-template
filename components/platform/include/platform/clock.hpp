#pragma once

#include "application/ports/device.hpp"

namespace platform {

class EspClock final : public application::Clock {
    public:
    domain::Milliseconds now() const noexcept override;
};

} // namespace platform
