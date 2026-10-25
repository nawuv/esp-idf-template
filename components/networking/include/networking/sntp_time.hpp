#pragma once

#include "networking/connections.hpp"

namespace networking {

class SntpTime final : public NetworkTime {
    public:
    void begin() noexcept override;
    bool ready() const noexcept override;
};

} // namespace networking
