#include "networking/network_service.hpp"

#include "networking/lifecycle.hpp"

#include <cstring>

namespace networking {

void NetworkService::configure(const configuration::NetworkConfig& config) noexcept {
    if (state_ != NetworkState::disabled) {
        return;
    }
    config_ = config;
    needs_time_ = std::strncmp(config_.mqtt_uri, "mqtts://", 8) == 0;
    retry_not_before_ = 0;
    state_ = NetworkState::starting;
}

void NetworkService::start() noexcept {
    if (state_ != NetworkState::stopped && state_ != NetworkState::failed) {
        return;
    }
    retry_not_before_ = 0;
    state_ = NetworkState::starting;
}

void NetworkService::stop() noexcept {
    if (state_ == NetworkState::disabled) {
        return;
    }
    release();
    retry_not_before_ = 0;
    state_ = NetworkState::stopped;
}

void NetworkService::release() noexcept {
    for (const auto role : connection_stop_order) {
        switch (role) {
        case ConnectionRole::mqtt:
            mqtt_.stop();
            break;
        case ConnectionRole::time:
            time_.stop();
            break;
        case ConnectionRole::wifi:
            wifi_.stop();
            break;
        }
    }
}

void NetworkService::fail_bringup() noexcept {
    release();
    retry_not_before_ = clock_.now() + network_init_retry_ms;
    state_ = NetworkState::failed;
}

void NetworkService::step() noexcept {
    if (state_ == NetworkState::disabled || state_ == NetworkState::stopped) {
        return;
    }
    if (state_ == NetworkState::failed) {
        if (clock_.now() < retry_not_before_) {
            return;
        }
        state_ = NetworkState::starting;
    }
    if (state_ == NetworkState::starting) {
        if (!wifi_.start(config_)) {
            fail_bringup();
            return;
        }
        if (needs_time_) {
            time_.begin();
        }
        state_ = NetworkState::waiting_for_wifi;
    }
    wifi_.step();
    if (state_ == NetworkState::running) {
        return;
    }
    if (!wifi_.connected()) {
        state_ = NetworkState::waiting_for_wifi;
        return;
    }
    if (needs_time_ && !time_.ready()) {
        state_ = NetworkState::waiting_for_time;
        return;
    }
    state_ = NetworkState::waiting_for_mqtt;
    if (clock_.now() < retry_not_before_) {
        return;
    }
    if (!mqtt_.start(config_)) {
        // The station and SNTP stay up. Only the failed client is released.
        mqtt_.stop();
        retry_not_before_ = clock_.now() + network_init_retry_ms;
        return;
    }
    retry_not_before_ = 0;
    state_ = NetworkState::running;
}

void NetworkService::report(const domain::Telemetry& status) noexcept {
    if (wifi_.connected()) {
        mqtt_.report(status);
    } else {
        mqtt_.drop_report();
    }
}

} // namespace networking
