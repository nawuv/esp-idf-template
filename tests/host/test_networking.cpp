#include "espkit/configuration/network_config.hpp"
#include "espkit/networking/telemetry.hpp"

#include <array>
#include <cstring>
#include <gtest/gtest.h>
#include <limits>
#include <string_view>

namespace {

espkit::NetworkConfig config() {
    espkit::NetworkConfig value{};
    std::strcpy(value.wifi_ssid, "lab");
    std::strcpy(value.wifi_password, "test-password");
    std::strcpy(value.mqtt_uri, "mqtts://broker.example.com:8883");
    std::strcpy(value.device_id, "node-01");
    std::strcpy(value.site_id, "farm");
    return value;
}

TEST(NetworkConfig, RequiresTlsUnlessDevelopmentModeIsExplicit) {
    auto c = config();
    EXPECT_TRUE(espkit::valid_network_config(c));
    std::strcpy(c.mqtt_uri, "mqtt://localhost:1883");
    EXPECT_FALSE(espkit::valid_network_config(c));
    EXPECT_TRUE(espkit::valid_network_config(c, true));
}

TEST(NetworkConfig, RejectsTopicInjectionCredentialsInUriAndUnterminatedFields) {
    auto c = config();
    std::strcpy(c.device_id, "node/commands");
    EXPECT_FALSE(espkit::valid_network_config(c));
    c = config();
    std::strcpy(c.mqtt_uri, "mqtts://user:secret@host");
    EXPECT_FALSE(espkit::valid_network_config(c));
    c = config();
    std::memset(c.wifi_ssid, 'x', sizeof(c.wifi_ssid));
    EXPECT_FALSE(espkit::valid_network_config(c));
    c = config();
    c.wifi_password[0] = '\0';
    EXPECT_FALSE(espkit::valid_network_config(c));
}

TEST(NetworkConfig, RejectsMissingHostBadPortAndPaths) {
    for (const auto* uri : {"mqtts:///", "mqtts://:8883", "mqtts://host:0", "mqtts://host:99999",
                            "mqtts://host/path"}) {
        auto c = config();
        std::strcpy(c.mqtt_uri, uri);
        EXPECT_FALSE(espkit::valid_network_config(c)) << uri;
    }
}

TEST(TelemetryEncoding, CarriesMeasurementUnitTimestampAndFault) {
    espkit::Telemetry status{1000,
                             false,
                             espkit::AppliedOutput::off,
                             false,
                             espkit::OutputError::none,
                             {25.5F, 900, true, espkit::MeasurementUnit::celsius}};
    std::array<char, 512> buffer{};
    const auto size = espkit::encode_telemetry(status, buffer);
    ASSERT_GT(size, 0U);
    const std::string_view json(buffer.data(), size);
    EXPECT_NE(json.find("\"value\":25.500"), json.npos);
    EXPECT_NE(json.find("\"unit\":\"celsius\""), json.npos);
    EXPECT_NE(json.find("\"sampled_at_ms\":900"), json.npos);
    EXPECT_NE(json.find("\"applied\":\"off\""), json.npos);
}

TEST(TelemetryEncoding, InvalidNumbersBecomeNullAndSmallBuffersFail) {
    espkit::Telemetry status{1000,
                             false,
                             espkit::AppliedOutput::unknown,
                             true,
                             espkit::OutputError::hardware_fault,
                             {std::numeric_limits<float>::quiet_NaN(), 900, true}};
    std::array<char, 512> buffer{};
    ASSERT_GT(espkit::encode_telemetry(status, buffer), 0U);
    EXPECT_NE(std::string_view(buffer.data()).find("\"value\":null"), std::string_view::npos);
    EXPECT_NE(std::string_view(buffer.data()).find("\"valid\":false"), std::string_view::npos);
    std::array<char, 8> small{};
    EXPECT_EQ(espkit::encode_telemetry(status, small), 0U);
}

TEST(ReconnectBackoff, IsBoundedAddsJitterAndResetsAfterRecovery) {
    espkit::ReconnectBackoff retry;
    EXPECT_EQ(retry.next_delay(0), 500U);
    EXPECT_EQ(retry.next_delay(0), 1000U);
    for (unsigned i = 0; i < 30; ++i) {
        EXPECT_LE(retry.next_delay(12345), 30000U);
    }
    retry.reset();
    EXPECT_GT(retry.next_delay(10), 500U);
}

} // namespace
