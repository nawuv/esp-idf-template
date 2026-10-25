#include "esp_log.h"
#include "espkit/actuators/simulated_output.hpp"
#include "espkit/application/workers.hpp"
#include "espkit/runtime/adapters.hpp"
#include "espkit/sensors/synthetic_sensor.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdlib>

namespace {

constexpr float demo_sensor_value = 42.0F;
constexpr espkit::Milliseconds max_run_ms = 1000;
constexpr espkit::Milliseconds max_sample_age_ms = 500;
constexpr unsigned step_period_ms = 100;

class LocalEnableRequest final : public espkit::RequestSource {
    public:
    // Intentionally keeps requesting ON to demonstrate the latched runtime limit.
    bool enable_requested(espkit::Milliseconds) noexcept override {
        return true;
    }
};

} // namespace

extern "C" void app_main() {
    static espkit::EspClock clock;
    static espkit::SyntheticSensor sensor(demo_sensor_value);
    static espkit::SimulatedOutput output;
    static LocalEnableRequest requests;
    static espkit::FreeRtosSnapshot<espkit::Reading> readings;
    static espkit::FreeRtosSnapshot<espkit::Telemetry> telemetry;
    static espkit::TimedOutput policy({max_run_ms, max_sample_age_ms});
    static espkit::SensorWorker sampler(sensor, clock, readings);
    static espkit::ControlWorker controller(policy, clock, readings, requests, output, telemetry);
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
        espkit::Telemetry status{};
        if (telemetry.latest(status)) {
            ESP_LOGI("actuator_node", "output=%d fault=%d", output.enabled(), status.fault);
        }
        vTaskDelay(pdMS_TO_TICKS(step_period_ms));
    }
}
