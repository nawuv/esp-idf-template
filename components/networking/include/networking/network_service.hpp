#pragma once

#include "networking/connections.hpp"

namespace networking {

enum class NetworkState {
    disabled,
    starting,
    waiting_for_wifi,
    waiting_for_time,
    running,
    failed
};

// Worker owns step(); reporter owns report(). Configure before starting either task.
class NetworkService final : public application::Work, public application::TelemetrySink {
    public:
    NetworkService(WifiConnection& wifi, MqttConnection& mqtt, NetworkTime& time)
        : wifi_(wifi), mqtt_(mqtt), time_(time) {
    }

    NetworkService(const NetworkService&) = delete;
    NetworkService& operator=(const NetworkService&) = delete;
    // Boot-time only. Copies validated configuration and owns its client lifetime.
    void configure(const configuration::NetworkConfig& config) noexcept;
    void step() noexcept override;
    void report(const domain::Telemetry& status) noexcept override;

    // Worker-thread inspection only. Initialization failure is terminal until reboot.
    NetworkState state() const noexcept {
        return state_;
    }

    private:
    WifiConnection& wifi_;
    MqttConnection& mqtt_;
    NetworkTime& time_;
    configuration::NetworkConfig config_{};
    NetworkState state_{NetworkState::disabled};
    bool needs_time_{false};
};

} // namespace networking
