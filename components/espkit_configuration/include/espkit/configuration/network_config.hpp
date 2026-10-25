#pragma once

#include <cstddef>

namespace espkit {

struct NetworkConfig {
    char wifi_ssid[33]{};
    char wifi_password[64]{};
    char mqtt_uri[192]{};
    char mqtt_username[64]{};
    char mqtt_password[128]{};
    char device_id[64]{};
    char site_id[32]{};
};

bool valid_network_config(const NetworkConfig& config, bool allow_plaintext = false) noexcept;
// Loads the default NVS partition without erasing or migrating existing storage.
bool load_network_config(NetworkConfig& config, bool allow_plaintext) noexcept;

} // namespace espkit
