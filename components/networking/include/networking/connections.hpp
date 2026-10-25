#pragma once

#include "application/ports/device.hpp"
#include "configuration/network_config.hpp"

namespace networking {

class WifiConnection {
    public:
    virtual ~WifiConnection() = default;
    virtual bool start(const configuration::NetworkConfig& config) noexcept = 0;
    virtual void step() noexcept = 0;
    virtual bool connected() const noexcept = 0;
};

class MqttConnection : public application::TelemetrySink {
    public:
    virtual bool start(const configuration::NetworkConfig& config) noexcept = 0;
    virtual void drop_report() noexcept = 0;
};

class NetworkTime {
    public:
    virtual ~NetworkTime() = default;
    virtual void begin() noexcept = 0;
    virtual bool ready() const noexcept = 0;
};

} // namespace networking
