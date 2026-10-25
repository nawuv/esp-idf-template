#include "networking/network_service.hpp"

#include <cstring>
#include <gtest/gtest.h>

namespace {

class FakeWifi final : public networking::WifiConnection {
    public:
    bool start(const configuration::NetworkConfig& config) noexcept override {
        ++starts;
        ssid = config.wifi_ssid;
        return succeeds;
    }

    void step() noexcept override {
        ++steps;
    }

    bool connected() const noexcept override {
        return online;
    }

    int starts{0};
    int steps{0};
    bool succeeds{true};
    bool online{false};
    std::string ssid;
};

class FakeMqtt final : public networking::MqttConnection {
    public:
    bool start(const configuration::NetworkConfig&) noexcept override {
        ++starts;
        return succeeds;
    }

    void report(const domain::Telemetry&) noexcept override {
        ++reports;
    }

    void drop_report() noexcept override {
        ++drops;
    }

    int starts{0};
    int reports{0};
    int drops{0};
    bool succeeds{true};
};

class FakeTime final : public networking::NetworkTime {
    public:
    void begin() noexcept override {
        ++begins;
    }

    bool ready() const noexcept override {
        return synchronized;
    }

    int begins{0};
    bool synchronized{false};
};

class NetworkServiceTest : public ::testing::Test {
    protected:
    FakeWifi wifi;
    FakeMqtt mqtt;
    FakeTime time;
    networking::NetworkService service{wifi, mqtt, time};
    configuration::NetworkConfig config{};

    void SetUp() override {
        std::strcpy(config.wifi_ssid, "test-network");
        std::strcpy(config.mqtt_uri, "mqtts://broker.example");
    }
};

TEST_F(NetworkServiceTest, UnconfiguredServiceDoesNotStartAdapters) {
    service.step();
    EXPECT_EQ(service.state(), networking::NetworkState::disabled);
    EXPECT_EQ(wifi.starts, 0);
    EXPECT_EQ(mqtt.starts, 0);
}

TEST_F(NetworkServiceTest, TlsWaitsForWifiAndTimeAndKeepsServicingReconnects) {
    service.configure(config);
    config.wifi_ssid[0] = 'X';
    service.step();
    EXPECT_EQ(wifi.ssid, "test-network");
    EXPECT_EQ(time.begins, 1);
    EXPECT_EQ(mqtt.starts, 0);
    wifi.online = true;
    service.step();
    EXPECT_EQ(service.state(), networking::NetworkState::waiting_for_time);
    EXPECT_EQ(mqtt.starts, 0);
    time.synchronized = true;
    service.step();
    EXPECT_EQ(service.state(), networking::NetworkState::running);
    EXPECT_EQ(mqtt.starts, 1);
    wifi.online = false;
    service.step();
    wifi.online = true;
    service.step();
    EXPECT_EQ(wifi.steps, 5);
    EXPECT_EQ(wifi.starts, 1);
    EXPECT_EQ(mqtt.starts, 1);
}

TEST_F(NetworkServiceTest, PlaintextDoesNotRequireTimeSynchronization) {
    std::strcpy(config.mqtt_uri, "mqtt://broker.example");
    wifi.online = true;
    service.configure(config);
    service.step();
    EXPECT_EQ(service.state(), networking::NetworkState::running);
    EXPECT_EQ(time.begins, 0);
    EXPECT_EQ(mqtt.starts, 1);
}

TEST_F(NetworkServiceTest, WifiInitializationFailureIsTerminal) {
    wifi.succeeds = false;
    service.configure(config);
    service.step();
    service.step();
    EXPECT_EQ(service.state(), networking::NetworkState::failed);
    EXPECT_EQ(wifi.starts, 1);
    EXPECT_EQ(mqtt.starts, 0);
}

TEST_F(NetworkServiceTest, MqttInitializationFailureIsTerminal) {
    wifi.online = true;
    time.synchronized = true;
    mqtt.succeeds = false;
    service.configure(config);
    service.step();
    service.step();
    EXPECT_EQ(service.state(), networking::NetworkState::failed);
    EXPECT_EQ(mqtt.starts, 1);
}

TEST_F(NetworkServiceTest, ReportsAreDroppedOfflineAndForwardedOnline) {
    service.report({});
    EXPECT_EQ(mqtt.drops, 1);
    wifi.online = true;
    service.report({});
    EXPECT_EQ(mqtt.reports, 1);
}

} // namespace
