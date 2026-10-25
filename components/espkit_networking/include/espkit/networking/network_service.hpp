#pragma once

#include "espkit/networking/mqtt_telemetry.hpp"
#include "espkit/networking/wifi_station.hpp"

namespace espkit::networking {

// Coordinates injected adapters and owns their boot-lifetime configuration.
// Worker calls step(); reporter calls report(). Event callbacks only update adapter state.
class NetworkService final : public application::Work, public application::TelemetrySink {
    public:
    NetworkService(WifiStation& wifi, MqttTelemetry& mqtt) : wifi_(wifi), mqtt_(mqtt) {
    }

    NetworkService(const NetworkService&) = delete;
    NetworkService& operator=(const NetworkService&) = delete;
    void step() noexcept override;
    void report(const domain::Telemetry& status) noexcept override;

    private:
    WifiStation& wifi_;
    MqttTelemetry& mqtt_;
    configuration::NetworkConfig config_{};
    bool attempted_start_{false};
    bool initialized_{false};
    bool attempted_mqtt_{false};
};

} // namespace espkit::networking
