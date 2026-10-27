#pragma once

#include "application/ports/device.hpp"
#include "configuration/network_config.hpp"

namespace networking {

// Station link used by the network worker. start() returns false only after
// service-owned resources are released, or leaves them recorded so stop() can
// finish that release. stop() is safe when start() never succeeded and safe to
// call again. After stop(), connected() is false.
class WifiConnection {
    public:
    virtual ~WifiConnection() = default;
    virtual bool start(const configuration::NetworkConfig& config) noexcept = 0;
    virtual void stop() noexcept = 0;
    virtual void step() noexcept = 0;
    virtual bool connected() const noexcept = 0;
};

class MqttConnection : public application::TelemetrySink {
    public:
    virtual bool start(const configuration::NetworkConfig& config) noexcept = 0;
    // Idempotent. Drops the client so a later start() can create another one.
    virtual void stop() noexcept = 0;
    virtual void drop_report() noexcept = 0;
};

class NetworkTime {
    public:
    virtual ~NetworkTime() = default;
    virtual void begin() noexcept = 0;
    virtual void stop() noexcept = 0;
    // Wall-clock readiness for TLS. This can stay true after stop(); stopping
    // SNTP does not roll the clock backwards.
    virtual bool ready() const noexcept = 0;
};

} // namespace networking
