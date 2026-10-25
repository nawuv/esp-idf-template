#include "espkit/networking/mqtt_telemetry.hpp"

#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_random.h"
#include "espkit/networking/telemetry.hpp"
#include "sdkconfig.h"

#include <array>
#include <cstdio>

namespace espkit::networking {

namespace {

constexpr char tag[] = "espkit_network";
constexpr unsigned payload_capacity = 512;
constexpr unsigned outbox_limit_bytes = 4096;
constexpr unsigned mqtt_retry_ms = 5000;
constexpr unsigned network_timeout_ms = 3000;

} // namespace

bool MqttTelemetry::start(const configuration::NetworkConfig& config) noexcept {
#if CONFIG_ESPKIT_NETWORK_ENABLED
    std::snprintf(telemetry_topic_, sizeof(telemetry_topic_), "iot/v1/%s/%s/telemetry",
                  config.site_id, config.device_id);
    std::snprintf(availability_topic_, sizeof(availability_topic_), "iot/v1/%s/%s/availability",
                  config.site_id, config.device_id);
    esp_mqtt_client_config_t mqtt{};
    mqtt.broker.address.uri = config.mqtt_uri;
    mqtt.broker.verification.crt_bundle_attach = esp_crt_bundle_attach;
    mqtt.credentials.client_id = config.device_id;
    mqtt.credentials.username = config.mqtt_username[0] ? config.mqtt_username : nullptr;
    mqtt.credentials.authentication.password =
        config.mqtt_password[0] ? config.mqtt_password : nullptr;
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
    if (esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, event, this) != ESP_OK ||
        esp_mqtt_client_start(client) != ESP_OK) {
        client_.store(nullptr);
        esp_mqtt_client_destroy(client);
        return false;
    }
    return true;
#else
    (void)config;
    return false;
#endif
}

void MqttTelemetry::event(void* context, esp_event_base_t, std::int32_t id, void*) {
    auto& self = *static_cast<MqttTelemetry*>(context);
    if (id == MQTT_EVENT_CONNECTED) {
        self.mqtt_up_.store(true);
        esp_mqtt_client_enqueue(self.client_.load(), self.availability_topic_, "online", 0, 1, 1,
                                true);
        ESP_LOGI(tag, "MQTT connected");
    } else if (id == MQTT_EVENT_DISCONNECTED || id == MQTT_EVENT_ERROR) {
        self.mqtt_up_.store(false);
    }
}

void MqttTelemetry::report(const espkit::domain::Telemetry& status) noexcept {
#if CONFIG_ESPKIT_NETWORK_ENABLED
    const auto client = client_.load();
    if (!client || !mqtt_up_.load()) {
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

} // namespace espkit::networking
