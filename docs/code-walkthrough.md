# Code walkthrough

Read these files in order. The default firmware is a simulation; it never drives
physical GPIO outputs. Enabling the SHT3x option changes input, not output behavior.

## 1. Entry point: main/app_main.cpp

ESP-IDF calls `app_main()`. It calls `device::start_device()`; it does not contain
sensor, control, or network logic.

## 2. Wiring: main/device.cpp

`DeviceRuntime` owns concrete objects and passes references to their consumers.
For example, `SensorWorker` receives a sensor, clock, and reading writer. It never
constructs a driver itself. Creating concrete objects here is the composition-root
part of dependency injection, not a violation of it.

The selected sensor is synthetic by default. The SHT3x build option selects the
physical driver and initializes its injected I2C adapter. Output is always simulated
and the root device always requests OFF. Settings live in `device_config.hpp` and
`boards/esp32s3/board_config.hpp`.

## 3. Single-step work: application/workers.hpp and workers.cpp

`SensorWorker::step()` reads one sample and publishes it. `ControlWorker::step()`
reads the latest sample, evaluates `domain::TimedOutput`, applies the result, and
publishes status. `ReporterWorker::step()` forwards the latest status to its sink.

A snapshot is a one-slot mailbox: a newer value replaces the old value. It is not
a command queue or a history. Host tests supply in-memory snapshots and fake clocks.

## 4. Policy: domain/timed_output.hpp and timed_output.cpp

The state machine owns off/running/fault transitions. It checks measurement age
and maximum continuous runtime using the injected monotonic time. It knows nothing
about GPIO, Wi-Fi, MQTT, or FreeRTOS. Workers own the calls to external ports.

## 5. Scheduling: platform/adapters.hpp and runtime.cpp

`PeriodicTask` repeatedly calls a worker's `step()` and yields until its next period.
Sensor reads can wait without stopping the separately scheduled local controller.
`FreeRtosSnapshot` implements the mailbox using a bounded queue.

## 6. Optional networking

The composition root injects `WifiStation` and `MqttTelemetry` into `NetworkService`.
The network task loads NVS settings, connects Wi-Fi, waits for TLS time readiness,
and starts MQTT. The reporter logs status and attempts bounded telemetry enqueue.
Offline reports are dropped; reconnection does not replay old measurements.

These adapters live for the entire boot. Initialization failure leaves networking
unavailable while local workers continue. Real board timing, live broker outages,
and physical safety still require the [hardware acceptance checks](hardware-mqtt.md).

## Where should I make a change?

| Change | Location |
| --- | --- |
| Select a different sensor/output | `main/device.cpp` |
| Change periods, policy, task budgets | `main/device_config.hpp` |
| Change wiring | `main/boards/esp32s3/board_config.hpp` and Kconfig |
| Add a portable driver | `components/espkit_drivers/` |
| Change local safety rules | `components/espkit_domain/` and host tests |
| Change MQTT encoding/connection | `components/espkit_networking/` |

Read [Architecture](architecture.md) for dependency rules before adding cross-component includes.
