#include "configuration/settings_migration.hpp"

#include <cstring>

namespace configuration {

namespace {

enum class FieldRequirement : std::uint8_t {
    required,
    optional
};

int severity(SettingsProblem problem) noexcept {
    switch (problem) {
    case SettingsProblem::none:
        return 0;
    case SettingsProblem::out_of_range:
        return 1;
    case SettingsProblem::partial:
    case SettingsProblem::incompatible:
        return 2;
    case SettingsProblem::corrupt:
        return 3;
    }
    return 3;
}

SettingsProblem worse(SettingsProblem left, SettingsProblem right) noexcept {
    return severity(left) >= severity(right) ? left : right;
}

template <std::size_t N>
SettingsProblem classify_field(StoredKey state, const char (&source)[N],
                               FieldRequirement requirement) noexcept {
    switch (state) {
    case StoredKey::missing:
        return requirement == FieldRequirement::required ? SettingsProblem::partial
                                                         : SettingsProblem::none;
    case StoredKey::present:
        break;
    case StoredKey::corrupt:
    default:
        return SettingsProblem::corrupt;
    }
    if (std::memchr(source, '\0', N) == nullptr) {
        return SettingsProblem::corrupt;
    }
    return SettingsProblem::none;
}

template <std::size_t N>
void copy_field(StoredKey state, const char (&source)[N], char (&destination)[N]) noexcept {
    if (state == StoredKey::present) {
        std::memcpy(destination, source, N);
    }
}

// Schema 1 field set. Optional broker credentials default to empty when absent.
// A future version adds a named step after this projection; it does not replace it.
SettingsProblem read_v1_fields(const StoredSettings& stored, NetworkConfig& out,
                               bool allow_plaintext) noexcept {
    SettingsProblem problem = SettingsProblem::none;
    problem = worse(problem, classify_field(stored.wifi_ssid, stored.values.wifi_ssid,
                                            FieldRequirement::required));
    problem = worse(problem, classify_field(stored.wifi_password, stored.values.wifi_password,
                                            FieldRequirement::required));
    problem = worse(problem, classify_field(stored.mqtt_uri, stored.values.mqtt_uri,
                                            FieldRequirement::required));
    problem = worse(problem, classify_field(stored.device_id, stored.values.device_id,
                                            FieldRequirement::required));
    problem = worse(
        problem, classify_field(stored.site_id, stored.values.site_id, FieldRequirement::required));
    problem = worse(problem, classify_field(stored.mqtt_username, stored.values.mqtt_username,
                                            FieldRequirement::optional));
    problem = worse(problem, classify_field(stored.mqtt_password, stored.values.mqtt_password,
                                            FieldRequirement::optional));
    if (problem != SettingsProblem::none) {
        return problem;
    }

    NetworkConfig config{};
    copy_field(stored.wifi_ssid, stored.values.wifi_ssid, config.wifi_ssid);
    copy_field(stored.wifi_password, stored.values.wifi_password, config.wifi_password);
    copy_field(stored.mqtt_uri, stored.values.mqtt_uri, config.mqtt_uri);
    copy_field(stored.device_id, stored.values.device_id, config.device_id);
    copy_field(stored.site_id, stored.values.site_id, config.site_id);
    copy_field(stored.mqtt_username, stored.values.mqtt_username, config.mqtt_username);
    copy_field(stored.mqtt_password, stored.values.mqtt_password, config.mqtt_password);
    if (!valid_network_config(config, allow_plaintext)) {
        return SettingsProblem::out_of_range;
    }
    out = config;
    return SettingsProblem::none;
}

} // namespace

SettingsMigration migrate_settings(const StoredSettings& stored, bool allow_plaintext) noexcept {
    // Published steps: a missing schema key upgrades to v1, and v1 is current.
    // A schema above current is projected through the v1 fields, not upgraded.
    static_assert(settings_schema_current == settings_schema_v1,
                  "add a migration step before changing settings_schema_current");

    SettingsMigration result{};
    result.schema_key_present = stored.schema != StoredKey::missing;
    if (stored.schema == StoredKey::corrupt) {
        result.problem = SettingsProblem::corrupt;
        return result;
    }
    if (stored.schema == StoredKey::present) {
        result.observed_schema = stored.schema_version;
        if (stored.schema_version == 0) {
            result.problem = SettingsProblem::corrupt;
            return result;
        }
    }

    const bool newer =
        stored.schema == StoredKey::present && stored.schema_version > settings_schema_current;
    const bool unversioned = stored.schema == StoredKey::missing;

    const auto problem = read_v1_fields(stored, result.config, allow_plaintext);
    if (problem != SettingsProblem::none) {
        result.config = {};
        result.problem =
            newer && problem == SettingsProblem::partial ? SettingsProblem::incompatible : problem;
        return result;
    }

    result.problem = SettingsProblem::none;
    result.effective_schema = settings_schema_current;
    if (unversioned) {
        result.outcome = SettingsOutcome::migrated;
    } else if (newer) {
        result.outcome = SettingsOutcome::compatible_newer;
    } else {
        result.outcome = SettingsOutcome::current;
    }
    return result;
}

} // namespace configuration
