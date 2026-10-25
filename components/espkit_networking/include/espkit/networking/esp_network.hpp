#pragma once

#include "esp_event.h"
#include "espkit/configuration/network_config.hpp"
#include "espkit/core/ports.hpp"
#include "espkit/networking/telemetry.hpp"
#include "mqtt_client.h"

#include <atomic>

namespace espkit {

// Boot-lifetime service. Only the network worker initializes or retries connections.
// Reporter calls never write outputs, accept commands, or wait for network delivery.
class EspNetwork final : public Work, public TelemetrySink {
    public:
    explicit EspNetwork(Clock& clock) : clock_(clock) {
    }

    EspNetwork(const EspNetwork&) = delete;
    EspNetwork& operator=(const EspNetwork&) = delete;
    void step() noexcept override;
    void report(const Telemetry& status) noexcept override;

    std::uint32_t dropped_reports() const noexcept {
        return dropped_.load();
    }

    private:
    bool start() noexcept;
    bool start_mqtt() noexcept;
    static void wifi_event(void* context, esp_event_base_t base, std::int32_t id, void* data);
    static void mqtt_event(void* context, esp_event_base_t base, std::int32_t id, void* data);
    Clock& clock_;
    NetworkConfig config_{};
    ReconnectBackoff backoff_;
    std::atomic<esp_mqtt_client_handle_t> client_{nullptr};
    std::atomic<bool> wifi_started_{false};
    std::atomic<bool> wifi_up_{false};
    std::atomic<bool> connecting_{false};
    std::atomic<bool> mqtt_up_{false};
    std::atomic<std::uint32_t> dropped_{0};
    esp_event_handler_instance_t wifi_handler_{nullptr};
    esp_event_handler_instance_t ip_handler_{nullptr};
    char telemetry_topic_[160]{};
    char availability_topic_[160]{};
    Milliseconds next_attempt_{0};
    Milliseconds connect_started_{0};
    bool attempted_start_{false};
    bool initialized_{false};
    bool attempted_mqtt_{false};
};

} // namespace espkit
