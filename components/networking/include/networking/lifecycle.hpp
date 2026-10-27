#pragma once

#include <cstdint>

namespace networking {

// Resources acquired by WifiStation. idle and process_ready are not released:
// the TCP/IP stack and the default event loop outlive one station restart.
enum class WifiPhase : std::uint8_t {
    idle,
    process_ready,
    netif,
    driver,
    handlers,
    started
};

enum class MqttPhase : std::uint8_t {
    idle,
    created,
    started
};

enum class SntpPhase : std::uint8_t {
    idle,
    started
};

// Stop walks this list. Start is not the reverse loop: Wi-Fi and SNTP have to
// become ready before MQTT, so NetworkService orders those waits itself.
enum class ConnectionRole : std::uint8_t {
    wifi,
    time,
    mqtt
};

struct MqttCleanup {
    bool stop_client;
    bool destroy_client;
};

inline constexpr ConnectionRole connection_stop_order[] = {
    ConnectionRole::mqtt,
    ConnectionRole::time,
    ConnectionRole::wifi,
};

constexpr bool service_owned(WifiPhase phase) noexcept {
    return phase == WifiPhase::netif || phase == WifiPhase::driver ||
           phase == WifiPhase::handlers || phase == WifiPhase::started;
}

constexpr bool wifi_phase_advances(WifiPhase current, WifiPhase next) noexcept {
    return (current == WifiPhase::idle && next == WifiPhase::process_ready) ||
           (current == WifiPhase::process_ready && next == WifiPhase::netif) ||
           (current == WifiPhase::netif && next == WifiPhase::driver) ||
           (current == WifiPhase::driver && next == WifiPhase::handlers) ||
           (current == WifiPhase::handlers && next == WifiPhase::started);
}

// One step of a reverse teardown. idle and process_ready stay where they are.
constexpr WifiPhase release_wifi_phase(WifiPhase phase) noexcept {
    switch (phase) {
    case WifiPhase::started:
        return WifiPhase::handlers;
    case WifiPhase::handlers:
        return WifiPhase::driver;
    case WifiPhase::driver:
        return WifiPhase::netif;
    case WifiPhase::netif:
        return WifiPhase::process_ready;
    case WifiPhase::process_ready:
    case WifiPhase::idle:
        return phase;
    }
    return WifiPhase::idle;
}

// A started client must be stopped before destroy. A client that was only
// created must be destroyed without stop: the SDK rejects stop in that state.
constexpr MqttCleanup mqtt_cleanup(MqttPhase phase) noexcept {
    switch (phase) {
    case MqttPhase::started:
        return MqttCleanup{true, true};
    case MqttPhase::created:
        return MqttCleanup{false, true};
    case MqttPhase::idle:
        return MqttCleanup{false, false};
    }
    return MqttCleanup{false, false};
}

constexpr bool sntp_is_started(SntpPhase phase) noexcept {
    return phase == SntpPhase::started;
}

} // namespace networking
