#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iot/application/workers.hpp"
#include "iot/runtime/adapters.hpp"
#include "iot/sensors/synthetic_sensor.hpp"

#include <cstdlib>

namespace {

constexpr float demo_sensor_value = 23.5F;
constexpr iot::Milliseconds sampling_period_ms = 500;
constexpr unsigned reporting_period_ms = 1000;

} // namespace

extern "C" void app_main() {
    static iot::EspClock clock;
    static iot::SyntheticSensor sensor(demo_sensor_value);
    static iot::FreeRtosSnapshot<iot::Reading> readings;
    static iot::SensorWorker worker(sensor, clock, readings);
    static iot::PeriodicTask task(worker,
                                  {"sensor", iot::runtime_constants::sensor_stack_bytes,
                                   iot::runtime_constants::sensor_priority, sampling_period_ms});
    if (!readings.ready()) {
        std::abort();
    }
    worker.step();
    if (!task.start()) {
        std::abort();
    }
    for (;;) {
        iot::Reading reading{};
        if (readings.latest(reading)) {
            ESP_LOGI("sensor_node", "value=%.1f sampled_at_ms=%lld valid=%d",
                     static_cast<double>(reading.value), static_cast<long long>(reading.sampled_at),
                     reading.valid);
        }
        vTaskDelay(pdMS_TO_TICKS(reporting_period_ms));
    }
}
