#include "espkit/networking/network_service.hpp"

#include "esp_log.h"
#include "esp_sntp.h"
#include "sdkconfig.h"

#include <cstring>
#include <ctime>

namespace espkit::networking {

void NetworkService::step() noexcept {
#if CONFIG_ESPKIT_NETWORK_ENABLED
    if (!attempted_start_) {
        attempted_start_ = true;
#if CONFIG_ESPKIT_ALLOW_PLAINTEXT_MQTT
        constexpr bool allow_plaintext = true;
#else
        constexpr bool allow_plaintext = false;
#endif
        initialized_ =
            configuration::load_network_config(config_, allow_plaintext) && wifi_.start(config_);
        if (!initialized_) {
            ESP_LOGW("espkit_network", "Network startup unavailable; local control continues");
            return;
        }
        if (std::strncmp(config_.mqtt_uri, "mqtts://", 8) == 0) {
            esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
            esp_sntp_setservername(0, "pool.ntp.org");
            esp_sntp_init();
        }
    }
    if (!initialized_) {
        return;
    }
    wifi_.step();
    if (!wifi_.connected()) {
        return;
    }
    // Certificate validity uses wall time; safety deadlines use the monotonic clock.
    constexpr std::time_t minimum_tls_time = 1704067200; // 2024-01-01 UTC
    if (std::strncmp(config_.mqtt_uri, "mqtts://", 8) == 0 &&
        std::time(nullptr) < minimum_tls_time) {
        return;
    }
    if (!attempted_mqtt_) {
        attempted_mqtt_ = true;
        if (!mqtt_.start(config_)) {
            ESP_LOGW("espkit_network", "MQTT startup unavailable; local control continues");
        }
    }
#endif
}

void NetworkService::report(const domain::Telemetry& status) noexcept {
#if CONFIG_ESPKIT_NETWORK_ENABLED
    if (wifi_.connected()) {
        mqtt_.report(status);
    } else {
        mqtt_.drop_report();
    }
#else
    (void)status;
#endif
}

} // namespace espkit::networking
