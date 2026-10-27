#include "configuration/settings_migration.hpp"

#include <cstring>
#include <gtest/gtest.h>

namespace {

template <std::size_t N> void put(char (&destination)[N], const char* text) {
    std::memset(destination, 0, N);
    std::strcpy(destination, text);
}

configuration::StoredSettings valid_v1() {
    configuration::StoredSettings stored{};
    stored.schema = configuration::StoredKey::present;
    stored.schema_version = configuration::settings_schema_v1;
    stored.wifi_ssid = configuration::StoredKey::present;
    stored.wifi_password = configuration::StoredKey::present;
    stored.mqtt_uri = configuration::StoredKey::present;
    stored.device_id = configuration::StoredKey::present;
    stored.site_id = configuration::StoredKey::present;
    put(stored.values.wifi_ssid, "lab");
    put(stored.values.wifi_password, "test-password");
    put(stored.values.mqtt_uri, "mqtts://broker.example.com:8883");
    put(stored.values.device_id, "node-01");
    put(stored.values.site_id, "farm");
    return stored;
}

bool blank(const configuration::NetworkConfig& config) {
    const configuration::NetworkConfig zero{};
    return std::memcmp(&config, &zero, sizeof(config)) == 0;
}

} // namespace

TEST(SettingsMigration, MigratesUnversionedRecordToCurrentSchema) {
    auto stored = valid_v1();
    stored.schema = configuration::StoredKey::missing;
    stored.schema_version = 9;

    const auto result = configuration::migrate_settings(stored);

    EXPECT_TRUE(configuration::settings_accepted(result));
    EXPECT_EQ(result.outcome, configuration::SettingsOutcome::migrated);
    EXPECT_EQ(result.problem, configuration::SettingsProblem::none);
    EXPECT_FALSE(result.schema_key_present);
    EXPECT_EQ(result.observed_schema, 0);
    EXPECT_EQ(result.effective_schema, configuration::settings_schema_current);
    EXPECT_STREQ(result.config.wifi_ssid, "lab");
    EXPECT_STREQ(result.config.mqtt_uri, "mqtts://broker.example.com:8883");
    EXPECT_STREQ(result.config.device_id, "node-01");
}

TEST(SettingsMigration, AcceptsSchemaV1AsCurrent) {
    const auto result = configuration::migrate_settings(valid_v1());

    EXPECT_EQ(result.outcome, configuration::SettingsOutcome::current);
    EXPECT_EQ(result.observed_schema, configuration::settings_schema_v1);
    EXPECT_EQ(result.effective_schema, configuration::settings_schema_current);
    EXPECT_STREQ(result.config.wifi_password, "test-password");
    EXPECT_STREQ(result.config.site_id, "farm");
}

TEST(SettingsMigration, DefaultsMissingOptionalCredentialsToEmpty) {
    auto stored = valid_v1();
    stored.mqtt_username = configuration::StoredKey::missing;
    stored.mqtt_password = configuration::StoredKey::missing;
    std::strcpy(stored.values.mqtt_username, "ignored-user");
    std::strcpy(stored.values.mqtt_password, "ignored-secret");

    const auto result = configuration::migrate_settings(stored);

    EXPECT_EQ(result.outcome, configuration::SettingsOutcome::current);
    EXPECT_STREQ(result.config.mqtt_username, "");
    EXPECT_STREQ(result.config.mqtt_password, "");
}

TEST(SettingsMigration, KeepsPresentOptionalCredentials) {
    auto stored = valid_v1();
    stored.mqtt_username = configuration::StoredKey::present;
    stored.mqtt_password = configuration::StoredKey::present;
    put(stored.values.mqtt_username, "device");
    put(stored.values.mqtt_password, "broker-secret");

    const auto result = configuration::migrate_settings(stored);

    EXPECT_EQ(result.outcome, configuration::SettingsOutcome::current);
    EXPECT_STREQ(result.config.mqtt_username, "device");
    EXPECT_STREQ(result.config.mqtt_password, "broker-secret");
}

TEST(SettingsMigration, UsesNewerSchemaWhenV1FieldsRemainValid) {
    auto stored = valid_v1();
    stored.schema_version = 255;

    const auto result = configuration::migrate_settings(stored);

    EXPECT_EQ(result.outcome, configuration::SettingsOutcome::compatible_newer);
    EXPECT_EQ(result.problem, configuration::SettingsProblem::none);
    EXPECT_TRUE(result.schema_key_present);
    EXPECT_EQ(result.observed_schema, 255);
    EXPECT_EQ(result.effective_schema, configuration::settings_schema_v1);
    EXPECT_STREQ(result.config.wifi_ssid, "lab");
    EXPECT_STREQ(result.config.device_id, "node-01");
    EXPECT_STREQ(result.config.mqtt_username, "");
    EXPECT_STREQ(result.config.mqtt_password, "");
}

TEST(SettingsMigration, RejectsNewerSchemaWhenRequiredKeyIsMissing) {
    auto stored = valid_v1();
    stored.schema_version = 2;
    stored.device_id = configuration::StoredKey::missing;

    const auto result = configuration::migrate_settings(stored);

    EXPECT_EQ(result.outcome, configuration::SettingsOutcome::rejected);
    EXPECT_EQ(result.problem, configuration::SettingsProblem::incompatible);
    EXPECT_EQ(result.observed_schema, 2);
    EXPECT_EQ(result.effective_schema, 0);
    EXPECT_TRUE(blank(result.config));
}

TEST(SettingsMigration, RejectsCorruptSchemaAndDamagedFields) {
    auto schema_zero = valid_v1();
    schema_zero.schema_version = 0;
    const auto zero = configuration::migrate_settings(schema_zero);
    EXPECT_EQ(zero.outcome, configuration::SettingsOutcome::rejected);
    EXPECT_EQ(zero.problem, configuration::SettingsProblem::corrupt);
    EXPECT_TRUE(blank(zero.config));

    auto unreadable = valid_v1();
    unreadable.schema = configuration::StoredKey::corrupt;
    const auto broken_schema = configuration::migrate_settings(unreadable);
    EXPECT_EQ(broken_schema.problem, configuration::SettingsProblem::corrupt);
    EXPECT_TRUE(broken_schema.schema_key_present);
    EXPECT_EQ(broken_schema.observed_schema, 0);

    auto unterminated = valid_v1();
    std::memset(unterminated.values.wifi_ssid, 'a', sizeof(unterminated.values.wifi_ssid));
    const auto damaged = configuration::migrate_settings(unterminated);
    EXPECT_EQ(damaged.problem, configuration::SettingsProblem::corrupt);
    EXPECT_TRUE(blank(damaged.config));

    auto bad_password = valid_v1();
    bad_password.mqtt_password = configuration::StoredKey::corrupt;
    const auto credential = configuration::migrate_settings(bad_password);
    EXPECT_EQ(credential.problem, configuration::SettingsProblem::corrupt);
    EXPECT_TRUE(blank(credential.config));
}

TEST(SettingsMigration, PrefersCorruptOverAMissingRequiredKey) {
    auto stored = valid_v1();
    stored.wifi_ssid = configuration::StoredKey::corrupt;
    stored.site_id = configuration::StoredKey::missing;

    const auto result = configuration::migrate_settings(stored);

    EXPECT_EQ(result.problem, configuration::SettingsProblem::corrupt);
    EXPECT_TRUE(blank(result.config));
}

TEST(SettingsMigration, RejectsPartialAndEmptyRecords) {
    auto missing_site = valid_v1();
    missing_site.site_id = configuration::StoredKey::missing;
    const auto partial = configuration::migrate_settings(missing_site);
    EXPECT_EQ(partial.outcome, configuration::SettingsOutcome::rejected);
    EXPECT_EQ(partial.problem, configuration::SettingsProblem::partial);
    EXPECT_TRUE(blank(partial.config));

    const auto empty = configuration::migrate_settings(configuration::StoredSettings{});
    EXPECT_EQ(empty.outcome, configuration::SettingsOutcome::rejected);
    EXPECT_EQ(empty.problem, configuration::SettingsProblem::partial);
    EXPECT_FALSE(empty.schema_key_present);
    EXPECT_TRUE(blank(empty.config));
}

TEST(SettingsMigration, RejectsOutOfRangeValuesWithoutClamping) {
    auto short_password = valid_v1();
    put(short_password.values.wifi_password, "short");
    const auto password = configuration::migrate_settings(short_password);
    EXPECT_EQ(password.problem, configuration::SettingsProblem::out_of_range);
    EXPECT_TRUE(blank(password.config));

    auto injected = valid_v1();
    put(injected.values.device_id, "node/commands");
    EXPECT_EQ(configuration::migrate_settings(injected).problem,
              configuration::SettingsProblem::out_of_range);

    auto plaintext = valid_v1();
    put(plaintext.values.mqtt_uri, "mqtt://localhost:1883");
    EXPECT_EQ(configuration::migrate_settings(plaintext).problem,
              configuration::SettingsProblem::out_of_range);
    const auto allowed = configuration::migrate_settings(plaintext, true);
    EXPECT_EQ(allowed.outcome, configuration::SettingsOutcome::current);
    EXPECT_STREQ(allowed.config.mqtt_uri, "mqtt://localhost:1883");
}

TEST(SettingsMigration, RejectsNewerSchemaWhenV1FieldsFailValidation) {
    auto stored = valid_v1();
    stored.schema_version = 4;
    put(stored.values.wifi_password, "short");

    const auto result = configuration::migrate_settings(stored);

    EXPECT_EQ(result.outcome, configuration::SettingsOutcome::rejected);
    EXPECT_EQ(result.problem, configuration::SettingsProblem::out_of_range);
    EXPECT_EQ(result.observed_schema, 4);
    EXPECT_TRUE(blank(result.config));
}

TEST(SettingsMigration, MigratesUnversionedPlaintextOnlyWhenAllowed) {
    auto stored = valid_v1();
    stored.schema = configuration::StoredKey::missing;
    put(stored.values.mqtt_uri, "mqtt://localhost:1883");

    EXPECT_EQ(configuration::migrate_settings(stored).problem,
              configuration::SettingsProblem::out_of_range);
    const auto allowed = configuration::migrate_settings(stored, true);
    EXPECT_EQ(allowed.outcome, configuration::SettingsOutcome::migrated);
    EXPECT_STREQ(allowed.config.mqtt_uri, "mqtt://localhost:1883");
}
