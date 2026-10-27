#include "networking/wifi_station.hpp"

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "sdkconfig.h"

#include <cstring>

namespace networking {

namespace {

constexpr char tag[] = "wifi";
constexpr domain::Milliseconds association_timeout_ms = 20000;

bool released(esp_err_t result) noexcept {
    return result == ESP_OK || result == ESP_ERR_WIFI_NOT_INIT ||
           result == ESP_ERR_WIFI_NOT_STARTED || result == ESP_ERR_INVALID_STATE;
}

} // namespace

bool WifiStation::advance(WifiPhase next) noexcept {
    if (!wifi_phase_advances(phase_, next)) {
        return false;
    }
    phase_ = next;
    return true;
}

bool WifiStation::unregister_handlers() noexcept {
    if (wifi_handler_ != nullptr) {
        if (esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler_) !=
            ESP_OK) {
            return false;
        }
        wifi_handler_ = nullptr;
    }
    if (ip_handler_ != nullptr) {
        if (esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_handler_) !=
            ESP_OK) {
            return false;
        }
        ip_handler_ = nullptr;
    }
    return true;
}

bool WifiStation::release_current() noexcept {
    switch (phase_) {
    case WifiPhase::started: {
        // Disconnect is best-effort: the station may already be down.
        esp_wifi_disconnect();
        const auto stopped = esp_wifi_stop();
        if (!released(stopped)) {
            ESP_LOGW(tag, "esp_wifi_stop failed: %s", esp_err_to_name(stopped));
            return false;
        }
        return true;
    }
    case WifiPhase::handlers:
        return unregister_handlers();
    case WifiPhase::driver: {
        // Handler registration can fail after the first instance is stored.
        if (!unregister_handlers()) {
            return false;
        }
        const auto deinited = esp_wifi_deinit();
        if (!released(deinited)) {
            ESP_LOGW(tag, "esp_wifi_deinit failed: %s", esp_err_to_name(deinited));
            return false;
        }
        return true;
    }
    case WifiPhase::netif:
        esp_netif_destroy_default_wifi(sta_netif_);
        sta_netif_ = nullptr;
        return true;
    case WifiPhase::process_ready:
    case WifiPhase::idle:
        return true;
    }
    return false;
}

bool WifiStation::release_owned() noexcept {
    while (service_owned(phase_)) {
        if (!release_current()) {
            return false;
        }
        phase_ = release_wifi_phase(phase_);
    }
    wifi_started_.store(false);
    wifi_up_.store(false);
    connecting_.store(false);
    backoff_.reset();
    next_attempt_ = 0;
    connect_started_ = 0;
    return true;
}

bool WifiStation::ensure_process() noexcept {
    if (phase_ == WifiPhase::process_ready) {
        return true;
    }
    if (phase_ != WifiPhase::idle) {
        return false;
    }
    // esp_netif_deinit is not supported. The default event loop is also shared
    // with SNTP and MQTT, so a station restart must not delete it.
    if (esp_netif_init() != ESP_OK) {
        return false;
    }
    const auto loop_result = esp_event_loop_create_default();
    if (loop_result != ESP_OK && loop_result != ESP_ERR_INVALID_STATE) {
        return false;
    }
    return advance(WifiPhase::process_ready);
}

bool WifiStation::start(const configuration::NetworkConfig& config) noexcept {
#if CONFIG_ESPKIT_NETWORK_ENABLED
    if (phase_ == WifiPhase::started) {
        return true;
    }
    if (service_owned(phase_) && !release_owned()) {
        return false;
    }
    if (!ensure_process()) {
        return false;
    }

    // The default helper aborts on failure and would stop control as well.
    // These calls return errors so a partial station can be destroyed instead.
    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_WIFI_STA();
    sta_netif_ = esp_netif_new(&netif_config);
    if (sta_netif_ == nullptr) {
        return false;
    }
    if (!advance(WifiPhase::netif)) {
        esp_netif_destroy(static_cast<esp_netif_t*>(sta_netif_));
        sta_netif_ = nullptr;
        return false;
    }
    if (esp_netif_attach_wifi_station(static_cast<esp_netif_t*>(sta_netif_)) != ESP_OK ||
        esp_wifi_set_default_wifi_sta_handlers() != ESP_OK) {
        release_owned();
        return false;
    }

    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&init) != ESP_OK) {
        release_owned();
        return false;
    }
    if (!advance(WifiPhase::driver)) {
        esp_wifi_deinit();
        release_owned();
        return false;
    }
    // Credentials stay in the owned configuration copy, not Wi-Fi's flash store.
    if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK) {
        release_owned();
        return false;
    }
    if (esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event, this,
                                            &wifi_handler_) != ESP_OK ||
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event, this,
                                            &ip_handler_) != ESP_OK) {
        release_owned();
        return false;
    }
    if (!advance(WifiPhase::handlers)) {
        release_owned();
        return false;
    }

    wifi_config_t wifi{};
    std::memcpy(wifi.sta.ssid, config.wifi_ssid, std::strlen(config.wifi_ssid));
    std::memcpy(wifi.sta.password, config.wifi_password, std::strlen(config.wifi_password));
    wifi.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK ||
        esp_wifi_set_config(WIFI_IF_STA, &wifi) != ESP_OK) {
        release_owned();
        return false;
    }
    if (esp_wifi_start() != ESP_OK) {
        release_owned();
        return false;
    }
    if (!advance(WifiPhase::started)) {
        esp_wifi_stop();
        release_owned();
        return false;
    }
    return true;
#else
    (void)config;
    return false;
#endif
}

void WifiStation::stop() noexcept {
#if CONFIG_ESPKIT_NETWORK_ENABLED
    // Drop the link flag before the driver is gone so reporters stop enqueueing.
    wifi_up_.store(false);
    connecting_.store(false);
    if (!release_owned()) {
        ESP_LOGW(tag, "Wi-Fi cleanup is incomplete and will resume on the next stop or start");
    }
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
    if (phase_ != WifiPhase::started) {
        return;
    }
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
