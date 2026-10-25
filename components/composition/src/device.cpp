#include "espkit/composition/device.hpp"

#include "esp_log.h"
#include "espkit/actuators/simulated_output.hpp"
#include "espkit/application/workers.hpp"
#include "espkit/board/profile.hpp"
#include "espkit/composition/constants.hpp"
#include "espkit/hal/i2c.hpp"
#include "espkit/networking/esp_network.hpp"
#include "espkit/runtime/adapters.hpp"
#include "espkit/sensors/sht3x.hpp"
#include "espkit/sensors/synthetic_sensor.hpp"

#include <cstdlib>

namespace espkit {

namespace {

class DisabledRequests final : public RequestSource {
    public:
    bool enable_requested(Milliseconds) noexcept override {
        return false;
    }
};

class CombinedReporter final : public TelemetrySink {
    public:
    CombinedReporter(TelemetrySink& log, TelemetrySink& network) : log_(log), network_(network) {
    }

    void report(const Telemetry& status) noexcept override {
        log_.report(status);
        network_.report(status);
    }

    private:
    TelemetrySink& log_;
    TelemetrySink& network_;
};

class Device {
    public:
    Device()
        : sensor_worker_(sensor_, clock_, readings_),
          control_worker_(policy_, clock_, readings_, requests_, output_, telemetry_),
          reporter_worker_(telemetry_, reporter_),
          sensor_task_(sensor_worker_,
                       {"sensor", runtime_constants::sensor_stack_bytes,
                        runtime_constants::sensor_priority, device_defaults::sensor_period_ms}),
          control_task_(control_worker_,
                        {"control", runtime_constants::control_stack_bytes,
                         runtime_constants::control_priority, device_defaults::control_period_ms}),
          reporter_task_(reporter_worker_, {"reporter", runtime_constants::reporter_stack_bytes,
                                            runtime_constants::reporter_priority,
                                            device_defaults::reporter_period_ms}) {
    }

    void start() {
        if (started_ || !readings_.ready() || !telemetry_.ready()) {
            std::abort();
        }
        started_ = true;
#if CONFIG_ESPKIT_SENSOR_SHT3X
        if (!bus_.begin(board::sda_gpio, board::scl_gpio, board::sensor_address,
                        board::i2c_clock_hz, board::i2c_timeout_ms)) {
            ESP_LOGW("espkit_board",
                     "I2C unavailable: check explicitly configured pins and wiring");
        }
#endif
        // Seed snapshots and apply safe OFF before any concurrent execution.
        sensor_worker_.step();
        control_worker_.step();
        if (!sensor_task_.start() || !control_task_.start() || !reporter_task_.start()) {
            std::abort();
        }
#if CONFIG_ESPKIT_NETWORK_ENABLED
        if (!network_task_.start()) {
            std::abort();
        }
#endif
    }

    private:
    EspClock clock_;
    FreeRtosSnapshot<Reading> readings_;
    FreeRtosSnapshot<Telemetry> telemetry_;
#if CONFIG_ESPKIT_SENSOR_SHT3X
    EspI2cDevice bus_;
    FreeRtosDelay delay_;
    Sht3xSensor sensor_{bus_, delay_};
#else
    SyntheticSensor sensor_{device_defaults::synthetic_reading};
#endif
    SimulatedOutput output_;
    DisabledRequests requests_;
    LogTelemetry log_;
    EspNetwork network_{clock_};
    CombinedReporter reporter_{log_, network_};
    TimedOutput policy_{OutputPolicy{}};
    SensorWorker sensor_worker_;
    ControlWorker control_worker_;
    ReporterWorker reporter_worker_;
    PeriodicTask sensor_task_;
    PeriodicTask control_task_;
    PeriodicTask reporter_task_;
#if CONFIG_ESPKIT_NETWORK_ENABLED
    PeriodicTask network_task_{network_,
                               {"network", runtime_constants::network_stack_bytes,
                                runtime_constants::network_priority,
                                device_defaults::network_period_ms}};
#endif
    bool started_{false};
};

} // namespace

void start_device() {
    // Construct after ESP-IDF startup, never from global hardware constructors.
    static Device device;
    device.start();
}

} // namespace espkit
