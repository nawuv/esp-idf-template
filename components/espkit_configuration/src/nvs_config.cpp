#include "espkit/configuration/network_config.hpp"
#include "nvs.h"
#include "nvs_flash.h"

#include <cstdint>

namespace espkit {

bool load_network_config(NetworkConfig& config, bool allow_plaintext) noexcept {
    // Preserve credentials and unrelated settings if NVS initialization fails.
    if (nvs_flash_init() != ESP_OK) {
        return false;
    }
    nvs_handle_t handle = 0;
    if (nvs_open("espkit", NVS_READONLY, &handle) != ESP_OK) {
        return false;
    }
    std::uint8_t schema = 0;
    if (nvs_get_u8(handle, "schema", &schema) != ESP_OK || schema != 1) {
        nvs_close(handle);
        return false;
    }
    NetworkConfig candidate{};
    auto read = [handle](const char* key, auto& field, bool optional = false) {
        std::size_t size = sizeof(field);
        const auto result = nvs_get_str(handle, key, field, &size);
        return result == ESP_OK || (optional && result == ESP_ERR_NVS_NOT_FOUND);
    };
    const bool loaded =
        read("wifi_ssid", candidate.wifi_ssid) && read("wifi_pass", candidate.wifi_password) &&
        read("mqtt_uri", candidate.mqtt_uri) && read("device_id", candidate.device_id) &&
        read("site_id", candidate.site_id) && read("mqtt_user", candidate.mqtt_username, true) &&
        read("mqtt_pass", candidate.mqtt_password, true);
    nvs_close(handle);
    if (!loaded || !valid_network_config(candidate, allow_plaintext)) {
        return false;
    }
    config = candidate;
    return true;
}

} // namespace espkit
