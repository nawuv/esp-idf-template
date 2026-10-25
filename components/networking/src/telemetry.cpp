#include "networking/telemetry.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace networking {

std::size_t encode_telemetry(const domain::Telemetry& s, std::span<char> buffer) noexcept {
    if (buffer.empty() || s.uptime < 0) {
        return 0;
    }
    const char* applied = s.applied_output == domain::AppliedOutput::on    ? "on"
                          : s.applied_output == domain::AppliedOutput::off ? "off"
                                                                           : "unknown";
    const char* unit = s.reading.unit == domain::MeasurementUnit::celsius ? "celsius"
                       : s.reading.unit == domain::MeasurementUnit::relative_humidity_percent
                           ? "percent_rh"
                           : "unspecified";
    const char* origin = s.reading.origin == domain::DataOrigin::synthetic  ? "synthetic"
                         : s.reading.origin == domain::DataOrigin::physical ? "physical"
                                                                            : "unspecified";
    const bool valid = s.reading.valid && std::isfinite(s.reading.value) &&
                       s.reading.sampled_at >= 0 && s.reading.sampled_at <= s.uptime;
    char value[48] = "null";
    if (valid) {
        std::snprintf(value, sizeof(value), "%.3f", static_cast<double>(s.reading.value));
    }
    const auto count = std::snprintf(
        buffer.data(), buffer.size(),
        "{\"schema_version\":1,\"uptime_ms\":%lld,"
        "\"requested\":%s,\"applied\":\"%s\","
        "\"fault\":%s,\"output_error\":%d,"
        "\"sensor\":{\"value\":%s,\"unit\":\"%s\","
        "\"origin\":\"%s\",\"valid\":%s,\"sampled_at_ms\":%lld}}",
        static_cast<long long>(s.uptime), s.output_requested ? "true" : "false", applied,
        s.fault ? "true" : "false", static_cast<int>(s.output_error), value, unit, origin,
        valid ? "true" : "false", static_cast<long long>(s.reading.sampled_at));
    return count > 0 && static_cast<std::size_t>(count) < buffer.size()
               ? static_cast<std::size_t>(count)
               : 0;
}

std::uint32_t ReconnectBackoff::next_delay(std::uint32_t random) noexcept {
    constexpr std::uint32_t base_ms = 500;
    constexpr std::uint32_t maximum_ms = 30000;
    const auto base = std::min(maximum_ms, base_ms << std::min(attempt_, 6U));
    attempt_ = std::min(attempt_ + 1, 6U);
    return std::min(maximum_ms, base + random % (base / 4 + 1));
}

} // namespace networking
