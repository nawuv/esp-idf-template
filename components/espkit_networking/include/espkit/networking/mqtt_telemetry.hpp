#pragma once

#include "espkit/application/ports/device.hpp"
#include "espkit/configuration/network_config.hpp"
#include "mqtt_client.h"

#include <atomic>

namespace espkit::networking {

// Boot-lifetime MQTT adapter; config passed to start() must outlive the client.
// No remote commands and no offline telemetry backlog.
class MqttTelemetry final : public application::TelemetrySink {
    public:
    MqttTelemetry() = default;
    MqttTelemetry(const MqttTelemetry&) = delete;
    MqttTelemetry& operator=(const MqttTelemetry&) = delete;
    bool start(const configuration::NetworkConfig& config) noexcept;
    void report(const domain::Telemetry& status) noexcept override;

    void drop_report() noexcept {
        ++dropped_;
    }

    std::uint32_t dropped_reports() const noexcept {
        return dropped_.load();
    }

    private:
    static void event(void* context, esp_event_base_t base, std::int32_t id, void* data);
    std::atomic<esp_mqtt_client_handle_t> client_{nullptr};
    std::atomic<bool> mqtt_up_{false};
    std::atomic<std::uint32_t> dropped_{0};
    char telemetry_topic_[160]{};
    char availability_topic_[160]{};
};

} // namespace espkit::networking
