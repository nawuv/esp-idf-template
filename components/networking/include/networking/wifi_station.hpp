#pragma once

#include "application/ports/device.hpp"
#include "configuration/network_config.hpp"
#include "esp_event.h"
#include "networking/connections.hpp"
#include "networking/telemetry.hpp"

#include <atomic>

namespace networking {

// Boot-lifetime Wi-Fi adapter. Only its worker calls start() and step().
class WifiStation final : public WifiConnection {
    public:
    explicit WifiStation(application::Clock& clock) : clock_(clock) {
    }

    WifiStation(const WifiStation&) = delete;
    WifiStation& operator=(const WifiStation&) = delete;
    bool start(const configuration::NetworkConfig& config) noexcept override;
    void step() noexcept override;

    bool connected() const noexcept override {
        return wifi_up_.load();
    }

    private:
    static void event(void* context, esp_event_base_t base, std::int32_t id, void* data);
    application::Clock& clock_;
    ReconnectBackoff backoff_;
    std::atomic<bool> wifi_started_{false};
    std::atomic<bool> wifi_up_{false};
    std::atomic<bool> connecting_{false};
    esp_event_handler_instance_t wifi_handler_{nullptr};
    esp_event_handler_instance_t ip_handler_{nullptr};
    domain::Milliseconds next_attempt_{0};
    domain::Milliseconds connect_started_{0};
};

} // namespace networking
