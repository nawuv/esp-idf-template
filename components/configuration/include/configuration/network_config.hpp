#pragma once

#include <cstddef>
#include <cstdint>

namespace configuration {

// Capacities include the terminating NUL. Provisioning byte limits are one less.
inline constexpr std::size_t wifi_ssid_capacity = 33;
inline constexpr std::size_t wifi_password_capacity = 64;
inline constexpr std::size_t mqtt_uri_capacity = 192;
inline constexpr std::size_t mqtt_username_capacity = 64;
inline constexpr std::size_t mqtt_password_capacity = 128;
inline constexpr std::size_t device_id_capacity = 64;
inline constexpr std::size_t site_id_capacity = 32;

// Durable on-disk schema in the NVS namespace "espkit".
// A missing schema key is the unversioned layout; it migrates to v1 in memory.
inline constexpr std::uint8_t settings_schema_v1 = 1;
inline constexpr std::uint8_t settings_schema_current = settings_schema_v1;

struct NetworkConfig {
    char wifi_ssid[wifi_ssid_capacity]{};
    char wifi_password[wifi_password_capacity]{};
    char mqtt_uri[mqtt_uri_capacity]{};
    char mqtt_username[mqtt_username_capacity]{};
    char mqtt_password[mqtt_password_capacity]{};
    char device_id[device_id_capacity]{};
    char site_id[site_id_capacity]{};
};

bool valid_network_config(const NetworkConfig& config, bool allow_plaintext = false) noexcept;

} // namespace configuration
