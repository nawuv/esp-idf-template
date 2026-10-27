#pragma once

#include "application/ports/device.hpp"
#include "configuration/network_config.hpp"
#include "esp_event.h"
#include "networking/connections.hpp"
#include "networking/lifecycle.hpp"
#include "networking/telemetry.hpp"

#include <atomic>

namespace networking {

// Boot-lifetime station adapter. The network worker calls start, stop, and step.
// The event callback only stores atomics. Process-owned TCP/IP and the default
// event loop stay initialized across stop(); the station netif and driver do not.
class WifiStation final : public WifiConnection {
    public:
    explicit WifiStation(application::Clock& clock) : clock_(clock) {
    }

    WifiStation(const WifiStation&) = delete;
    WifiStation& operator=(const WifiStation&) = delete;
    bool start(const configuration::NetworkConfig& config) noexcept override;
    void stop() noexcept override;
    void step() noexcept override;

    bool connected() const noexcept override {
        return wifi_up_.load();
    }

    private:
    bool ensure_process() noexcept;
    bool advance(WifiPhase next) noexcept;
    bool release_owned() noexcept;
    bool release_current() noexcept;
    bool unregister_handlers() noexcept;

    static void event(void* context, esp_event_base_t base, std::int32_t id, void* data);
    application::Clock& clock_;
    ReconnectBackoff backoff_;
    WifiPhase phase_{WifiPhase::idle};
    // esp_netif_t stays in the .cpp. The header must not require esp_netif.
    void* sta_netif_{nullptr};
    std::atomic<bool> wifi_started_{false};
    std::atomic<bool> wifi_up_{false};
    std::atomic<bool> connecting_{false};
    esp_event_handler_instance_t wifi_handler_{nullptr};
    esp_event_handler_instance_t ip_handler_{nullptr};
    domain::Milliseconds next_attempt_{0};
    domain::Milliseconds connect_started_{0};
};

} // namespace networking
