#include "iot/composition/device.hpp"

#include "iot/actuators/simulated_output.hpp"
#include "iot/application/workers.hpp"
#include "iot/composition/constants.hpp"
#include "iot/runtime/adapters.hpp"
#include "iot/sensors/synthetic_sensor.hpp"

#include <cstdlib>

namespace iot {

namespace {

class DisabledRequests final : public RequestSource {
    public:
    bool enable_requested(Milliseconds) noexcept override {
        return false;
    }
};

class Device {
    public:
    Device()
        : sensor_(device_defaults::synthetic_reading), sensor_worker_(sensor_, clock_, readings_),
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
        // Seed snapshots and apply safe OFF before any concurrent execution.
        sensor_worker_.step();
        control_worker_.step();
        if (!sensor_task_.start() || !control_task_.start() || !reporter_task_.start()) {
            std::abort();
        }
    }

    private:
    EspClock clock_;
    FreeRtosSnapshot<Reading> readings_;
    FreeRtosSnapshot<Telemetry> telemetry_;
    SyntheticSensor sensor_;
    SimulatedOutput output_;
    DisabledRequests requests_;
    LogTelemetry reporter_;
    TimedOutput policy_{OutputPolicy{}};
    SensorWorker sensor_worker_;
    ControlWorker control_worker_;
    ReporterWorker reporter_worker_;
    PeriodicTask sensor_task_;
    PeriodicTask control_task_;
    PeriodicTask reporter_task_;
    bool started_{false};
};

} // namespace

void start_device() {
    // Construct after ESP-IDF startup, never from global hardware constructors.
    static Device device;
    device.start();
}

} // namespace iot
