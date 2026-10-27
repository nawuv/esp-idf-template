#include "device.hpp"

#include "application/command_path.hpp"
#include "application/workers.hpp"
#include "boards/esp32s3/board_config.hpp"
#include "configuration/nvs_config_store.hpp"
#include "device_config.hpp"
#include "drivers/sht3x.hpp"
#include "drivers/simulated_output.hpp"
#include "drivers/synthetic_sensor.hpp"
#include "esp_log.h"
#include "networking/mqtt_telemetry.hpp"
#include "networking/network_service.hpp"
#include "networking/sntp_time.hpp"
#include "networking/wifi_station.hpp"
#include "platform/clock.hpp"
#include "platform/command_queue.hpp"
#include "platform/i2c.hpp"
#include "platform/log_telemetry.hpp"
#include "platform/periodic_task.hpp"
#include "platform/snapshot.hpp"

#include <cstdlib>

namespace device {

namespace app = application;

namespace {

#if CONFIG_ESPKIT_REMOTE_COMMANDS
constexpr bool remote_commands_enabled = true;
#else
constexpr bool remote_commands_enabled = false;
#endif

class CombinedReporter final : public app::TelemetrySink {
    public:
    CombinedReporter(app::TelemetrySink& log, app::TelemetrySink& network,
                     app::BoundedFifo<domain::CommandResult>& results)
        : log_(log), network_(network), results_(results) {
    }

    void report(const domain::Telemetry& status) noexcept override {
        log_.report(status);
        network_.report(status);
        domain::CommandResult result{};
        while (results_.try_pop(result)) {
            ESP_LOGI("commands", "id=%llu sequence=%u source=%d type=%d status=%d",
                     static_cast<unsigned long long>(result.id),
                     static_cast<unsigned>(result.sequence), static_cast<int>(result.source),
                     static_cast<int>(result.type), static_cast<int>(result.status));
        }
    }

    private:
    app::TelemetrySink& log_;
    app::TelemetrySink& network_;
    app::BoundedFifo<domain::CommandResult>& results_;
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
        if (started_ || !readings_.ready() || !telemetry_.ready() || !commands_.ready() ||
            !command_results_.ready() || !stops_.ready() ||
            ingress_.remote_commands_enabled() != remote_commands_enabled) {
            std::abort();
        }
        started_ = true;
#if CONFIG_ESPKIT_NETWORK_ENABLED
        configuration::NetworkConfig config{};
        configuration::NvsConfigStore store;
#if CONFIG_ESPKIT_ALLOW_PLAINTEXT_MQTT
        constexpr bool allow_plaintext = true;
#else
        constexpr bool allow_plaintext = false;
#endif
        if (store.load(config, allow_plaintext)) {
            network_.configure(config);
        } else {
            ESP_LOGW("network", "Network configuration unavailable; local control continues");
        }
#endif
#if CONFIG_ESPKIT_SENSOR_SHT3X
        if (!bus_.begin(board::sda_gpio, board::scl_gpio, board::sensor_address,
                        board::i2c_clock_hz, board::i2c_timeout_ms)) {
            ESP_LOGW("board", "I2C unavailable: check explicitly configured pins and wiring");
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
    platform::FreeRtosFifo<app::QueuedCommand> commands_{defaults::command_queue_capacity};
    platform::FreeRtosFifo<domain::CommandResult> command_results_{
        defaults::command_result_capacity};
    platform::FreeRtosSnapshot<app::StopNotice> stops_;
    app::CommandIngress ingress_{commands_, command_results_, stops_, defaults::command_limits,
                                 remote_commands_enabled};
#if CONFIG_ESPKIT_SENSOR_SHT3X
    platform::EspI2cDevice bus_;
    platform::FreeRtosDelay delay_;
    drivers::Sht3xSensor sensor_{bus_, delay_};
#else
    drivers::SyntheticSensor sensor_{defaults::synthetic_reading};
#endif
    drivers::SimulatedOutput output_;
    app::CommandRequests requests_{commands_, command_results_, stops_,
                                   defaults::output_policy.max_run_ms};
    platform::LogTelemetry log_{"simulated"};
    networking::WifiStation wifi_{clock_};
    networking::MqttTelemetry mqtt_;
    networking::SntpTime network_time_;
    networking::NetworkService network_{wifi_, mqtt_, network_time_, clock_};
    CombinedReporter reporter_{log_, network_, command_results_};
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
