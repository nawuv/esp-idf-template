#include "esp_log.h"
#include "esp_timer.h"
#include "espkit/runtime/adapters.hpp"

#include <limits>

namespace espkit {

Milliseconds EspClock::now() const noexcept {
    return esp_timer_get_time() / runtime_constants::microseconds_per_millisecond;
}

void LogTelemetry::report(const Telemetry& value) noexcept {
    ESP_LOGI("espkit_runtime",
             "mode=simulation uptime_ms=%lld requested=%d applied=%d fault=%d error=%d",
             static_cast<long long>(value.uptime), value.output_requested,
             static_cast<int>(value.applied_output), value.fault,
             static_cast<int>(value.output_error));
}

bool PeriodicTask::start() noexcept {
    if (handle_ || !options_.name || !*options_.name || options_.period_ms <= 0 ||
        options_.stack_bytes == 0 || options_.priority >= configMAX_PRIORITIES) {
        return false;
    }
    // Calculate before narrowing; reject sub-tick periods and conversion overflow.
    const auto max_ms =
        (std::numeric_limits<TickType_t>::max() * runtime_constants::milliseconds_per_second) /
        configTICK_RATE_HZ;
    if (static_cast<std::uint64_t>(options_.period_ms) > max_ms) {
        return false;
    }
    period_ticks_ = static_cast<TickType_t>(
        (static_cast<std::uint64_t>(options_.period_ms) * configTICK_RATE_HZ) /
        runtime_constants::milliseconds_per_second);
    if (!period_ticks_) {
        return false;
    }
    return xTaskCreate(entry, options_.name, options_.stack_bytes, this, options_.priority,
                       &handle_) == pdPASS;
}

void PeriodicTask::entry(void* context) {
    auto& task = *static_cast<PeriodicTask*>(context);
    auto wake = xTaskGetTickCount();
    for (;;) {
        task.work_.step();
        vTaskDelayUntil(&wake, task.period_ticks_);
    }
}

} // namespace espkit
