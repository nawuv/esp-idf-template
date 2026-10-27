#include "configuration/nvs_config_store.hpp"
#include "configuration/settings_migration.hpp"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include <cstring>

namespace configuration {

namespace {

constexpr char nvs_namespace[] = "espkit";

// A value that does not fit is corrupt. Do not keep a truncated credential.
template <std::size_t N>
StoredKey read_string(nvs_handle_t handle, const char* key, char (&field)[N]) noexcept {
    std::size_t size = N;
    const auto result = nvs_get_str(handle, key, field, &size);
    if (result == ESP_OK) {
        return StoredKey::present;
    }
    std::memset(field, 0, N);
    return result == ESP_ERR_NVS_NOT_FOUND ? StoredKey::missing : StoredKey::corrupt;
}

} // namespace

bool NvsConfigStore::load(NetworkConfig& config, bool allow_plaintext) const noexcept {
    // Preserve credentials and unrelated settings if NVS initialization fails.
    if (nvs_flash_init() != ESP_OK) {
        return false;
    }
    nvs_handle_t handle = 0;
    if (nvs_open(nvs_namespace, NVS_READONLY, &handle) != ESP_OK) {
        return false;
    }

    StoredSettings stored{};
    std::uint8_t schema = 0;
    const auto schema_result = nvs_get_u8(handle, "schema", &schema);
    if (schema_result == ESP_OK) {
        stored.schema = StoredKey::present;
        stored.schema_version = schema;
    } else if (schema_result == ESP_ERR_NVS_NOT_FOUND) {
        stored.schema = StoredKey::missing;
    } else {
        stored.schema = StoredKey::corrupt;
    }
    stored.wifi_ssid = read_string(handle, "wifi_ssid", stored.values.wifi_ssid);
    stored.wifi_password = read_string(handle, "wifi_pass", stored.values.wifi_password);
    stored.mqtt_uri = read_string(handle, "mqtt_uri", stored.values.mqtt_uri);
    stored.device_id = read_string(handle, "device_id", stored.values.device_id);
    stored.site_id = read_string(handle, "site_id", stored.values.site_id);
    stored.mqtt_username = read_string(handle, "mqtt_user", stored.values.mqtt_username);
    stored.mqtt_password = read_string(handle, "mqtt_pass", stored.values.mqtt_password);
    nvs_close(handle);

    const auto migration = migrate_settings(stored, allow_plaintext);
    if (!settings_accepted(migration)) {
        ESP_LOGW("config", "rejected NVS settings problem=%u schema=%u",
                 static_cast<unsigned>(migration.problem),
                 static_cast<unsigned>(migration.observed_schema));
        return false;
    }
    if (migration.outcome != SettingsOutcome::current) {
        ESP_LOGI("config", "loaded NVS settings outcome=%u schema=%u",
                 static_cast<unsigned>(migration.outcome),
                 static_cast<unsigned>(migration.observed_schema));
    }
    config = migration.config;
    return true;
}

} // namespace configuration
