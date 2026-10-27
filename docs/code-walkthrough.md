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
and the root device always requests OFF. The command ingress is wired and idle.
Settings live in `device_config.hpp` and
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

## 5. Commands: domain/command.hpp and application/command_path.hpp

A command is a small plain value: id, sequence, identity, type, source, deadline,
and, for an enable, a requested duration. `validate_command` rejects a command
that is malformed, too long, already expired, or remote while remote admission
is off. `CommandIngress::submit` then rejects duplicates and stale sequences.
A full FIFO returns `rejected_full`. It does not block and it does not drop some
older command to make room.

`CommandRequests` is the `RequestSource` the control worker already polls. After
`TimedOutput` decides, the worker calls `note_outcome`. That is the
acknowledgement that the command was applied or refused. The command code never
calls `OutputSink` itself.

`stop` is the exception to FIFO order. It publishes a one-slot notice, bumps a
generation counter, and the next control step throws away queued commands that
were admitted before that generation. A later, new command can still be accepted.
Retransmitting an id that was already accepted cannot start the output again.

The root firmware constructs this path and does not call `submit`. MQTT does not
subscribe. A future network adapter should parse a bounded payload, convert a
trusted deadline into monotonic time, and call `submit` from one task.

### Add a command type later

1. Add a `CommandType` enumerator.
2. Update `validate_command`. Host builds treat a missing switch case as an error.
3. Decide whether it is queued or immediate, like `stop`. Handle it in
   `CommandRequests` so an unknown queued value is not silently popped.
4. Keep the output behind `TimedOutput`. Do not reset a fault from a remote command.
5. Add host tests for acceptance, rejection, and the safety interaction.

## 6. Scheduling: platform/periodic_task.hpp, snapshot.hpp, command_queue.hpp, and runtime.cpp

`PeriodicTask` repeatedly calls a worker's `step()` and yields until its next period.
Sensor reads can wait without stopping the separately scheduled local controller.
`FreeRtosSnapshot` implements the mailbox using a bounded queue. `FreeRtosFifo`
is the command queue: a zero-wait send fails when the queue is full.

## 7. Optional networking

The composition root loads NVS settings with `NvsConfigStore` before starting tasks.
The store reads the `espkit` namespace and calls portable `migrate_settings`. A
schema 1 record is used as stored. A record with no schema key is upgraded in
memory when its fields validate. A newer schema is used only through those schema
1 fields. The load does not write flash. Invalid records leave networking off.
The composition root injects `WifiStation`, `MqttTelemetry`, and `SntpTime` through
portable connection ports into `NetworkService`. The network task connects Wi-Fi, waits for TLS time
readiness, and starts MQTT. The reporter logs status and attempts bounded telemetry enqueue.
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
| Add a portable driver | `components/drivers/` |
| Change local safety rules | `components/domain/` and host tests |
| Change command admission or add a command type | `domain/command.hpp`, `application/command_path.hpp`, and host tests |
| Subscribe to remote commands later | `components/networking/`, calling `CommandIngress::submit` |
| Change MQTT encoding/connection | `components/networking/` |
| Add a runtime network or identity setting | `configuration/network_config.hpp`, `settings_migration.hpp`, `NvsConfigStore`, and `tools/provision_device.py` |

Read [Architecture](architecture.md) for dependency rules before adding cross-component includes.
