# Architecture

ESPKit separates portable device behavior from SDK adapters and product wiring.
Start with [Code walkthrough](code-walkthrough.md) for the execution order.

## Names and directories

Project components use their responsibility as their name. A component is a
build/dependency boundary; a namespace is a C++ name boundary. Public headers
mirror their owning namespace. Check for SDK component collisions when adding names.

| Component | Namespace | Responsibility |
| --- | --- | --- |
| `domain` | `domain` | Measurements, status, timed-output policy, and command checks |
| `application` | `application` | Portable ports, command ingress, and single-step workers |
| `drivers` | `drivers` | Synthetic/SHT3x inputs and simulated output |
| `platform` | `platform` | ESP-IDF I2C, monotonic clock, FreeRTOS queues/tasks, log sink |
| `configuration` | `configuration` | Network settings validation and NVS loading |
| `networking` | `networking` | Wi-Fi lifecycle, MQTT telemetry, and connection coordination |
| `main/` | `device` | Product selection, board settings, task budgets, and object wiring |

For example, `drivers::Sht3xSensor` is declared in
`components/drivers/include/drivers/sht3x.hpp`.
Header paths use `application/...`. Kconfig symbols retain `ESPKIT_` because
they share a global configuration namespace; the stored NVS namespace remains
`espkit` to preserve provisioned settings.
Avoid generic `core`, `utils`, and `common` buckets. Do not add a component or a
namespace for each class. Split components when independent reuse or dependencies
justify a new build boundary.

## Dependency direction

```text
main (composition root)
  ├── drivers ──────────┐
  ├── platform ─────────┤
  ├── networking ───────┤──> application ──> domain
  │      └── configuration (independent settings adapter)
  └── domain / application
```

Domain, application, and drivers contain no ESP-IDF/FreeRTOS headers. Domain
has no dependency on application. Ports belong to the application that consumes
them, not to the SDK implementation. Platform, networking, and configuration
must not depend on each other except networking's explicit configuration dependency.
Public dependencies use CMake `REQUIRES`; implementation-only ones use `PRIV_REQUIRES`.
Run `python3 tools/check_architecture.py`; CI checks owned include and component edges.
This is a lightweight boundary check, not a general C++ dependency analyzer.

## Patterns

| Pattern | Concrete use |
| --- | --- |
| Constructor injection | Workers borrow sensor, output, clock, and snapshot ports |
| Composition root | `main/device.cpp` selects and owns concrete implementations |
| Adapter | ESP-IDF I2C/clock and FreeRTOS implement portable application ports |
| Strategy | Sensor/output ports allow synthetic and physical implementations |
| State machine | `domain::TimedOutput` explicitly tracks off/running/fault |
| Producer-consumer | Bounded latest-value queues connect workers |
| Bounded FIFO with rejection | Command ingress returns a status instead of overwriting or blocking |
| Separate stop mailbox | A full command FIFO cannot delay `CommandType::stop` |

No DI container, service locator, Singleton, generic event bus, or factory hierarchy
is required. A replaceable control-policy interface should wait until a second
policy needs it. Network coordination uses an explicit state enum rather than
one dynamically allocated object per state.

## Lifetime and concurrency

`app_main()` calls `device::start_device()`. A function-local static `DeviceRuntime`
constructs after SDK startup. Members are declared before objects borrowing them.
The runtime is boot-lifetime and startup is one-shot. Individual service shutdown,
restart, and resource recovery after partial SDK initialization are not supported.
This constraint is explicit; these adapters are not general-purpose restartable services.

Sensor (500 ms), control (100 ms), and reporter (2000 ms) workers run independently.
Two length-one FreeRTOS queues carry latest-value snapshots; reads do not consume
them. Overwrite is intentional. Commands use a separate bounded FIFO that rejects
when full, plus a one-slot stop notice. `CommandIngress::submit` is single-threaded;
the FIFO and stop mailbox are the cross-task boundary. The default firmware does
not call `submit`, so the output request stays off. Startup seeds a reading and
applies safe OFF before tasks start.

Before any tasks start, the composition root loads validated settings through
`configuration::NvsConfigStore` and passes a copy to `NetworkService::configure()`.
Only the network task calls `NetworkService::step()`. It steps `WifiStation`, waits
for injected `NetworkTime` readiness, then starts `MqttTelemetry`. The service
uses portable connection ports; `SntpTime` owns the SDK time-sync calls.
The reporter calls `NetworkService::report()`, which gates transmission on Wi-Fi
state and delegates to MQTT. Wi-Fi and MQTT callbacks update atomic adapter state;
retry timing and initialization state belong to the network worker. MQTT's callback
also enqueues retained online availability. MQTT config references remain valid
because `NetworkService` owns its settings for the boot lifetime.

Control never calls networking. Telemetry uses bounded nonblocking enqueue with
QoS 0 and no offline backlog. Sensor bus waits remain on the sensor task. Task
budgets require hardware measurement; compilation is not timing validation.

## Safety and status

`TimedOutput` rejects invalid/stale input and maximum runtime expiry, latching a
fault. Repeated ON requests cannot extend an interval. The worker applies the
policy decision; output errors latch fault and trigger a best-effort OFF. Failed
OFF is reported as unknown, not confirmed OFF. Applied status is software
acknowledgement, not physical feedback. Cooldowns, duty limits, independent
interlocks, and hardware watchdog behavior are product responsibilities.

Physical outputs remain disabled. Remote command sources stay rejected unless
`ESPKIT_REMOTE_COMMANDS` is enabled, and that option does not subscribe to MQTT.
Log status identifies sensor origin separately from the simulated output mode.
See [Safety](safety.md) and [ADR 0005](decisions/0005-command-ingress.md).

## Constants and settings

- `main/device_config.hpp`: sampling periods, task stacks/priorities, command queue depths, and selected policy.
- `main/boards/esp32s3/board_config.hpp`: GPIO and bus settings, exposed by `main/Kconfig.projbuild`.
- `domain/policy_defaults.hpp`: reusable safe-policy defaults, overridable through `OutputPolicy`.
- `platform/constants.hpp`: queue capacity and time conversions, not deployment task budgets.
- Driver `.cpp` files: fixed protocol commands, CRC values, and timing requirements.
- Runtime NVS: validated network credentials and identity; see [Hardware and MQTT](hardware-mqtt.md).

Use units in names. Configuration objects express values that vary by device;
constants express fixed protocol/library rules. Do not collect every literal into
one global constants file. NVS storage is not automatically encrypted.

## Conventions and evolution

Use C++20, `PascalCase` types, `snake_case` functions/files, trailing underscores
for fields, and English comments/docs. Public headers have no `using namespace`
directives; local aliases are acceptable in composition `.cpp` files. Use RAII
where lifetime is supported, borrowed references for injected dependencies, and
monotonic time for safety. Run `python3 tools/format.py` with clang-format 18.

See [ADR 0004](decisions/0004-portable-component-api.md) for migration details and
[Cluster and swarm evolution](distributed-systems.md) for future boundaries.

## Host build

Host tests add the component directories with `add_subdirectory()` and link their
library targets. Each component declares its portable sources once for both builds;
ESP-IDF adds SDK adapter sources. Configuration validation and network coordination
are included in the portable host targets.
