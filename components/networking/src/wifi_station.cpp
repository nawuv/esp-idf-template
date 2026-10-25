#include "networking/wifi_station.hpp"

#include "esp_netif.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "sdkconfig.h"

#include <cstring>

namespace networking {

namespace {

constexpr domain::Milliseconds association_timeout_ms = 20000;
}

bool WifiStation::start(const configuration::NetworkConfig& config) noexcept {
#if CONFIG_ESPKIT_NETWORK_ENABLED
    const auto netif_result = esp_netif_init();
    if (netif_result != ESP_OK) {
        return false;
    }
    const auto loop_result = esp_event_loop_create_default();
    if (loop_result != ESP_OK && loop_result != ESP_ERR_INVALID_STATE) {
        return false;
    }
    if (!esp_netif_create_default_wifi_sta()) {
        return false;
    }
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&init) != ESP_OK) {
        return false;
    }
    // Credentials stay in application-owned NVS configuration, not Wi-Fi's flash store.
    if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK) {
        return false;
    }
    if (esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event, this,
                                            &wifi_handler_) != ESP_OK ||
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event, this,
                                            &ip_handler_) != ESP_OK) {
        return false;
    }
    wifi_config_t wifi{};
    std::memcpy(wifi.sta.ssid, config.wifi_ssid, std::strlen(config.wifi_ssid));
    std::memcpy(wifi.sta.password, config.wifi_password, std::strlen(config.wifi_password));
    wifi.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    return esp_wifi_set_mode(WIFI_MODE_STA) == ESP_OK &&
           esp_wifi_set_config(WIFI_IF_STA, &wifi) == ESP_OK && esp_wifi_start() == ESP_OK;
#else
    (void)config;
    return false;
#endif
}

void WifiStation::event(void* context, esp_event_base_t base, std::int32_t id, void*) {
    auto& self = *static_cast<WifiStation*>(context);
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        self.wifi_started_.store(true);
    }
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        self.wifi_up_.store(false);
        self.connecting_.store(false);
    }
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        self.wifi_up_.store(true);
        self.connecting_.store(false);
    }
}

void WifiStation::step() noexcept {
#if CONFIG_ESPKIT_NETWORK_ENABLED
    if (wifi_up_.load()) {
        backoff_.reset();
        return;
    }
    const auto now = clock_.now();
    if (connecting_.load() && now - connect_started_ >= association_timeout_ms) {
        esp_wifi_disconnect();
        connecting_.store(false);
        next_attempt_ = now + backoff_.next_delay(esp_random());
        return;
    }
    if (wifi_started_.load() && !connecting_.load() && now >= next_attempt_) {
        next_attempt_ = now + backoff_.next_delay(esp_random());
        connect_started_ = now;
        connecting_.store(true);
        if (esp_wifi_connect() != ESP_OK) {
            connecting_.store(false);
        }
    }
#endif
}

} // namespace networking
