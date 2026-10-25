#include "device.hpp"

#include "boards/esp32s3/board_config.hpp"
#include "device_config.hpp"
#include "esp_log.h"
#include "espkit/application/workers.hpp"
#include "espkit/drivers/sht3x.hpp"
#include "espkit/drivers/simulated_output.hpp"
#include "espkit/drivers/synthetic_sensor.hpp"
#include "espkit/networking/network_service.hpp"
#include "espkit/platform/adapters.hpp"
#include "espkit/platform/i2c.hpp"

#include <cstdlib>

namespace device {

namespace app = espkit::application;
namespace domain = espkit::domain;
namespace drivers = espkit::drivers;
namespace platform = espkit::platform;
namespace networking = espkit::networking;

namespace {

class DisabledRequests final : public app::RequestSource {
    public:
    bool enable_requested(domain::Milliseconds) noexcept override {
        return false;
    }
};

class CombinedReporter final : public app::TelemetrySink {
    public:
    CombinedReporter(app::TelemetrySink& log, app::TelemetrySink& network)
        : log_(log), network_(network) {
    }

    void report(const domain::Telemetry& status) noexcept override {
        log_.report(status);
        network_.report(status);
    }

    private:
    app::TelemetrySink& log_;
    app::TelemetrySink& network_;
};

class DeviceRuntime {
    public:
    DeviceRuntime()
        : sensor_worker_(sensor_, clock_, readings_),
          control_worker_(policy_, clock_, readings_, requests_, output_, telemetry_),
          reporter_worker_(telemetry_, reporter_),
          sensor_task_(sensor_worker_,
                       {"sensor", device::defaults::sensor_stack_bytes,
                        device::defaults::sensor_priority, defaults::sensor_period_ms}),
          control_task_(control_worker_,
                        {"control", device::defaults::control_stack_bytes,
                         device::defaults::control_priority, defaults::control_period_ms}),
          reporter_task_(reporter_worker_,
                         {"reporter", device::defaults::reporter_stack_bytes,
                          device::defaults::reporter_priority, defaults::reporter_period_ms}) {
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
    platform::EspClock clock_;
    platform::FreeRtosSnapshot<domain::Reading> readings_;
    platform::FreeRtosSnapshot<domain::Telemetry> telemetry_;
#if CONFIG_ESPKIT_SENSOR_SHT3X
    platform::EspI2cDevice bus_;
    platform::FreeRtosDelay delay_;
    drivers::Sht3xSensor sensor_{bus_, delay_};
#else
    drivers::SyntheticSensor sensor_{defaults::synthetic_reading};
#endif
    drivers::SimulatedOutput output_;
    DisabledRequests requests_;
    platform::LogTelemetry log_{"simulated"};
    networking::WifiStation wifi_{clock_};
    networking::MqttTelemetry mqtt_;
    networking::NetworkService network_{wifi_, mqtt_};
    CombinedReporter reporter_{log_, network_};
    domain::TimedOutput policy_{defaults::output_policy};
    app::SensorWorker sensor_worker_;
    app::ControlWorker control_worker_;
    app::ReporterWorker reporter_worker_;
    platform::PeriodicTask sensor_task_;
    platform::PeriodicTask control_task_;
    platform::PeriodicTask reporter_task_;
#if CONFIG_ESPKIT_NETWORK_ENABLED
    platform::PeriodicTask network_task_{network_,
                                         {"network", device::defaults::network_stack_bytes,
                                          device::defaults::network_priority,
                                          defaults::network_period_ms}};
#endif
    bool started_{false};
};

} // namespace

void start_device() {
    // Construct after ESP-IDF startup, never from global hardware constructors.
    static DeviceRuntime device;
    device.start();
}

} // namespace device
