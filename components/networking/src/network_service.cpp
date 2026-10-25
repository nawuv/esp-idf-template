#include "networking/network_service.hpp"

#include <cstring>

namespace networking {

void NetworkService::configure(const configuration::NetworkConfig& config) noexcept {
    if (state_ != NetworkState::disabled) {
        return;
    }
    config_ = config;
    needs_time_ = std::strncmp(config_.mqtt_uri, "mqtts://", 8) == 0;
    state_ = NetworkState::starting;
}

void NetworkService::step() noexcept {
    if (state_ == NetworkState::disabled || state_ == NetworkState::failed) {
        return;
    }
    if (state_ == NetworkState::starting) {
        if (!wifi_.start(config_)) {
            state_ = NetworkState::failed;
            return;
        }
        if (needs_time_) {
            time_.begin();
        }
        state_ = NetworkState::waiting_for_wifi;
    }
    wifi_.step();
    if (!wifi_.connected() || state_ == NetworkState::running) {
        return;
    }
    if (needs_time_ && !time_.ready()) {
        state_ = NetworkState::waiting_for_time;
        return;
    }
    state_ = mqtt_.start(config_) ? NetworkState::running : NetworkState::failed;
}

void NetworkService::report(const domain::Telemetry& status) noexcept {
    if (wifi_.connected()) {
        mqtt_.report(status);
    } else {
        mqtt_.drop_report();
    }
}

} // namespace networking
