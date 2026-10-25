#include "esp_log.h"
#include "espkit/application/workers.hpp"
#include "espkit/drivers/simulated_output.hpp"
#include "espkit/drivers/synthetic_sensor.hpp"
#include "espkit/platform/adapters.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdlib>

namespace {

constexpr float demo_sensor_value = 42.0F;
constexpr espkit::domain::Milliseconds max_run_ms = 1000;
constexpr espkit::domain::Milliseconds max_sample_age_ms = 500;
constexpr unsigned step_period_ms = 100;

class LocalEnableRequest final : public espkit::application::RequestSource {
    public:
    // Intentionally keeps requesting ON to demonstrate the latched runtime limit.
    bool enable_requested(espkit::domain::Milliseconds) noexcept override {
        return true;
    }
};

} // namespace

extern "C" void app_main() {
    static espkit::platform::EspClock clock;
    static espkit::drivers::SyntheticSensor sensor(demo_sensor_value);
    static espkit::drivers::SimulatedOutput output;
    static LocalEnableRequest requests;
    static espkit::platform::FreeRtosSnapshot<espkit::domain::Reading> readings;
    static espkit::platform::FreeRtosSnapshot<espkit::domain::Telemetry> telemetry;
    static espkit::domain::TimedOutput policy({max_run_ms, max_sample_age_ms});
    static espkit::application::SensorWorker sampler(sensor, clock, readings);
    static espkit::application::ControlWorker controller(policy, clock, readings, requests, output,
                                                         telemetry);
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
        espkit::domain::Telemetry status{};
        if (telemetry.latest(status)) {
            ESP_LOGI("actuator_node", "output=%d fault=%d", output.enabled(), status.fault);
        }
        vTaskDelay(pdMS_TO_TICKS(step_period_ms));
    }
}
