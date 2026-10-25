#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iot/actuators/simulated_output.hpp"
#include "iot/application/workers.hpp"
#include "iot/runtime/adapters.hpp"
#include "iot/sensors/synthetic_sensor.hpp"

#include <cstdlib>

namespace {

constexpr float demo_sensor_value = 42.0F;
constexpr iot::Milliseconds max_run_ms = 1000;
constexpr iot::Milliseconds max_sample_age_ms = 500;
constexpr unsigned step_period_ms = 100;

class LocalEnableRequest final : public iot::RequestSource {
    public:
    // Intentionally keeps requesting ON to demonstrate the latched runtime limit.
    bool enable_requested(iot::Milliseconds) noexcept override {
        return true;
    }
};

} // namespace

extern "C" void app_main() {
    static iot::EspClock clock;
    static iot::SyntheticSensor sensor(demo_sensor_value);
    static iot::SimulatedOutput output;
    static LocalEnableRequest requests;
    static iot::FreeRtosSnapshot<iot::Reading> readings;
    static iot::FreeRtosSnapshot<iot::Telemetry> telemetry;
    static iot::TimedOutput policy({max_run_ms, max_sample_age_ms});
    static iot::SensorWorker sampler(sensor, clock, readings);
    static iot::ControlWorker controller(policy, clock, readings, requests, output, telemetry);
    if (!readings.ready() || !telemetry.ready()) {
        std::abort();
    }
    ESP_LOGI("actuator_node",
             "Simulation only: no GPIO; expected ON then latched OFF after 1 second");
    // This minimal example steps both workers on the main task for easy inspection.
    // The root simulation demonstrates independently scheduled workers.
    for (;;) {
        sampler.step();
        controller.step();
        iot::Telemetry status{};
        if (telemetry.latest(status)) {
            ESP_LOGI("actuator_node", "output=%d fault=%d", output.enabled(), status.fault);
        }
        vTaskDelay(pdMS_TO_TICKS(step_period_ms));
    }
}
