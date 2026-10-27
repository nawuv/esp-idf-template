#pragma once

#include "configuration/network_config.hpp"

#include <cstdint>

namespace configuration {

// How one stored key was observed. Buffers are meaningful only when the key is present.
enum class StoredKey : std::uint8_t {
    missing,
    present,
    corrupt
};

// Raw projection of the espkit namespace. Command admission is not part of this record.
struct StoredSettings {
    StoredKey schema{StoredKey::missing};
    std::uint8_t schema_version{0};
    StoredKey wifi_ssid{StoredKey::missing};
    StoredKey wifi_password{StoredKey::missing};
    StoredKey mqtt_uri{StoredKey::missing};
    StoredKey mqtt_username{StoredKey::missing};
    StoredKey mqtt_password{StoredKey::missing};
    StoredKey device_id{StoredKey::missing};
    StoredKey site_id{StoredKey::missing};
    NetworkConfig values{};
};

// Numeric values are stable; the NVS adapter logs them.
enum class SettingsOutcome : std::uint8_t {
    current = 0,
    migrated = 1,
    compatible_newer = 2,
    rejected = 3
};

enum class SettingsProblem : std::uint8_t {
    none = 0,
    corrupt = 1,
    partial = 2,
    out_of_range = 3,
    incompatible = 4
};

// In-memory migration result. `config` is zeroed unless the record was accepted.
struct SettingsMigration {
    SettingsOutcome outcome{SettingsOutcome::rejected};
    SettingsProblem problem{SettingsProblem::corrupt};
    std::uint8_t observed_schema{0};
    bool schema_key_present{false};
    std::uint8_t effective_schema{0};
    NetworkConfig config{};
};

// Upgrades unversioned storage and reads a newer schema through the v1 fields.
// Does not write, erase, or clamp. A rejected result carries a zeroed config.
SettingsMigration migrate_settings(const StoredSettings& stored,
                                   bool allow_plaintext = false) noexcept;

inline bool settings_accepted(const SettingsMigration& migration) noexcept {
    return migration.outcome != SettingsOutcome::rejected;
}

} // namespace configuration
