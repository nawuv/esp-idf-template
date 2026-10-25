#include "espkit/networking/esp_network.hpp"

#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include "sdkconfig.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace espkit {

namespace {

constexpr char tag[] = "espkit_network";
constexpr unsigned payload_capacity = 512;
constexpr unsigned outbox_limit_bytes = 4096;
constexpr unsigned mqtt_retry_ms = 5000;
constexpr unsigned network_timeout_ms = 3000;
constexpr Milliseconds association_timeout_ms = 20000;

} // namespace

bool EspNetwork::start() noexcept {
#if CONFIG_ESPKIT_NETWORK_ENABLED
#if CONFIG_ESPKIT_ALLOW_PLAINTEXT_MQTT
    constexpr bool allow_plaintext = true;
#else
    constexpr bool allow_plaintext = false;
#endif
    if (!load_network_config(config_, allow_plaintext)) {
        ESP_LOGW(tag, "Networking disabled: missing/invalid NVS settings; local control continues");
        return false;
    }
    std::snprintf(telemetry_topic_, sizeof(telemetry_topic_), "iot/v1/%s/%s/telemetry",
                  config_.site_id, config_.device_id);
    std::snprintf(availability_topic_, sizeof(availability_topic_), "iot/v1/%s/%s/availability",
                  config_.site_id, config_.device_id);
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
    if (std::strncmp(config_.mqtt_uri, "mqtts://", 8) == 0) {
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, "pool.ntp.org");
        esp_sntp_init();
    }
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&init) != ESP_OK) {
        return false;
    }
    // Credentials stay in application-owned NVS configuration, not Wi-Fi's flash store.
    if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK) {
        return false;
    }
    if (esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event, this,
                                            &wifi_handler_) != ESP_OK ||
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event, this,
                                            &ip_handler_) != ESP_OK) {
        return false;
    }
    wifi_config_t wifi{};
    std::memcpy(wifi.sta.ssid, config_.wifi_ssid, std::strlen(config_.wifi_ssid));
    std::memcpy(wifi.sta.password, config_.wifi_password, std::strlen(config_.wifi_password));
    wifi.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    return esp_wifi_set_mode(WIFI_MODE_STA) == ESP_OK &&
           esp_wifi_set_config(WIFI_IF_STA, &wifi) == ESP_OK && esp_wifi_start() == ESP_OK;
#else
    return false;
#endif
}

void EspNetwork::wifi_event(void* context, esp_event_base_t base, std::int32_t id, void*) {
    auto& self = *static_cast<EspNetwork*>(context);
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        self.wifi_started_.store(true);
    }
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        self.wifi_up_.store(false);
        self.connecting_.store(false);
        self.mqtt_up_.store(false);
    }
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        self.wifi_up_.store(true);
        self.connecting_.store(false);
    }
}

bool EspNetwork::start_mqtt() noexcept {
#if CONFIG_ESPKIT_NETWORK_ENABLED
    esp_mqtt_client_config_t mqtt{};
    mqtt.broker.address.uri = config_.mqtt_uri;
    mqtt.broker.verification.crt_bundle_attach = esp_crt_bundle_attach;
    mqtt.credentials.client_id = config_.device_id;
    mqtt.credentials.username = config_.mqtt_username[0] ? config_.mqtt_username : nullptr;
    mqtt.credentials.authentication.password =
        config_.mqtt_password[0] ? config_.mqtt_password : nullptr;
    mqtt.session.last_will.topic = availability_topic_;
    mqtt.session.last_will.msg = "offline";
    mqtt.session.last_will.qos = 1;
    mqtt.session.last_will.retain = 1;
    mqtt.network.reconnect_timeout_ms = mqtt_retry_ms + esp_random() % 1000;
    mqtt.network.timeout_ms = network_timeout_ms;
    mqtt.outbox.limit = outbox_limit_bytes;
    auto client = esp_mqtt_client_init(&mqtt);
    if (!client) {
        return false;
    }
    client_.store(client);
    if (esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event, this) != ESP_OK ||
        esp_mqtt_client_start(client) != ESP_OK) {
        client_.store(nullptr);
        esp_mqtt_client_destroy(client);
        return false;
    }
    return true;
#else
    return false;
#endif
}

void EspNetwork::mqtt_event(void* context, esp_event_base_t, std::int32_t id, void*) {
    auto& self = *static_cast<EspNetwork*>(context);
    if (id == MQTT_EVENT_CONNECTED) {
        self.mqtt_up_.store(true);
        esp_mqtt_client_enqueue(self.client_.load(), self.availability_topic_, "online", 0, 1, 1,
                                true);
        ESP_LOGI(tag, "MQTT connected");
    } else if (id == MQTT_EVENT_DISCONNECTED || id == MQTT_EVENT_ERROR) {
        self.mqtt_up_.store(false);
    }
}

void EspNetwork::step() noexcept {
#if CONFIG_ESPKIT_NETWORK_ENABLED
    if (!attempted_start_) {
        attempted_start_ = true;
        initialized_ = start();
        if (!initialized_) {
            ESP_LOGW(tag, "Network startup unavailable; local control continues");
        }
    }
    if (!initialized_) {
        return;
    }
    if (wifi_up_.load()) {
        backoff_.reset();
        // Wait for wall-clock synchronization before certificate validity checks.
        // Safety deadlines continue using the independent monotonic Clock port.
        constexpr std::time_t minimum_tls_time = 1704067200; // 2024-01-01 UTC
        if (std::strncmp(config_.mqtt_uri, "mqtts://", 8) == 0 &&
            std::time(nullptr) < minimum_tls_time) {
            return;
        }
        if (!attempted_mqtt_) {
            attempted_mqtt_ = true;
            if (!start_mqtt()) {
                ESP_LOGW(tag, "MQTT startup unavailable; local control continues");
            }
        }
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

void EspNetwork::report(const Telemetry& status) noexcept {
#if CONFIG_ESPKIT_NETWORK_ENABLED
    const auto client = client_.load();
    if (!client || !wifi_up_.load() || !mqtt_up_.load()) {
        ++dropped_;
        return;
    }
    std::array<char, payload_capacity> payload{};
    const auto length = encode_telemetry(status, payload);
    // QoS 0 + store=false: no offline telemetry backlog or replay after reconnect.
    if (!length || esp_mqtt_client_get_outbox_size(client) >= outbox_limit_bytes ||
        esp_mqtt_client_enqueue(client, telemetry_topic_, payload.data(), static_cast<int>(length),
                                0, 0, false) < 0) {
        ++dropped_;
    }
#else
    (void)status;
#endif
}

} // namespace espkit
