#include "configuration/network_config.hpp"

#include <charconv>
#include <cstring>
#include <string_view>

namespace configuration {

namespace {

template <std::size_t N> bool terminated(const char (&value)[N]) {
    return std::memchr(value, '\0', N) != nullptr;
}

bool identifier(std::string_view value) {
    if (value.empty()) {
        return false;
    }
    for (const unsigned char c : value) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == '-' || c == '_')) {
            return false;
        }
    }
    return true;
}

bool valid_uri(std::string_view uri, bool allow_plaintext) {
    std::size_t scheme = 0;
    if (uri.starts_with("mqtts://")) {
        scheme = 8;
    } else if (allow_plaintext && uri.starts_with("mqtt://")) {
        scheme = 7;
    } else {
        return false;
    }
    auto authority = uri.substr(scheme);
    if (authority.empty() || authority.find_first_of("/@ \r\n\t?#") != authority.npos) {
        return false;
    }
    std::string_view host;
    std::string_view port;
    bool has_port = false;
    if (authority.front() == '[') {
        const auto close = authority.find(']');
        if (close == authority.npos || close <= 1) {
            return false;
        }
        host = authority.substr(1, close - 1);
        const auto suffix = authority.substr(close + 1);
        if (!suffix.empty()) {
            if (suffix.front() != ':') {
                return false;
            }
            has_port = true;
            port = suffix.substr(1);
        }
    } else {
        const auto colon = authority.find(':');
        host = authority.substr(0, colon);
        if (colon != authority.npos) {
            has_port = true;
            port = authority.substr(colon + 1);
        }
    }
    if (host.empty()) {
        return false;
    }
    for (const unsigned char c : host) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == '.' || c == '-' || c == ':')) {
            return false;
        }
    }
    if (has_port) {
        unsigned number = 0;
        const auto result = std::from_chars(port.data(), port.data() + port.size(), number);
        if (result.ec != std::errc{} || result.ptr != port.data() + port.size() || number == 0 ||
            number > 65535) {
            return false;
        }
    }
    return true;
}

} // namespace

bool valid_network_config(const NetworkConfig& c, bool allow_plaintext) noexcept {
    if (!terminated(c.wifi_ssid) || !terminated(c.wifi_password) || !terminated(c.mqtt_uri) ||
        !terminated(c.mqtt_username) || !terminated(c.mqtt_password) || !terminated(c.device_id) ||
        !terminated(c.site_id)) {
        return false;
    }
    return c.wifi_ssid[0] && std::strlen(c.wifi_password) >= 8 &&
           valid_uri(c.mqtt_uri, allow_plaintext) && identifier(c.device_id) &&
           identifier(c.site_id);
}

} // namespace configuration
