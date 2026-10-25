#include "esp_log.h"
#include "espkit/application/workers.hpp"
#include "espkit/runtime/adapters.hpp"
#include "espkit/sensors/synthetic_sensor.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdlib>

namespace {

constexpr float demo_sensor_value = 23.5F;
constexpr espkit::Milliseconds sampling_period_ms = 500;
constexpr unsigned reporting_period_ms = 1000;

} // namespace

extern "C" void app_main() {
    static espkit::EspClock clock;
    static espkit::SyntheticSensor sensor(demo_sensor_value);
    static espkit::FreeRtosSnapshot<espkit::Reading> readings;
    static espkit::SensorWorker worker(sensor, clock, readings);
    static espkit::PeriodicTask task(
        worker, {"sensor", espkit::runtime_constants::sensor_stack_bytes,
                 espkit::runtime_constants::sensor_priority, sampling_period_ms});
    if (!readings.ready()) {
        std::abort();
    }
    worker.step();
    if (!task.start()) {
        std::abort();
    }
    for (;;) {
        espkit::Reading reading{};
        if (readings.latest(reading)) {
            ESP_LOGI("sensor_node", "value=%.1f sampled_at_ms=%lld valid=%d",
                     static_cast<double>(reading.value), static_cast<long long>(reading.sampled_at),
                     reading.valid);
        }
        vTaskDelay(pdMS_TO_TICKS(reporting_period_ms));
    }
}
