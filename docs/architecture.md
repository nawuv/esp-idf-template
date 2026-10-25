# Architecture

## Public naming

The project is named ESPKit. Public C++ symbols use the `espkit` root namespace,
and headers use `espkit/<component>/...` paths. The GitHub repository slug is
independent of this API name. Example: `espkit/application/timed_output.hpp`.

## Dependency direction

```text
main -> composition -> application -> core
                    -> sensors / actuators -> core
                    -> runtime -> core + ESP-IDF / FreeRTOS
```

`composition` is the only production location that selects concrete dependencies.
`application` and `core` have no ESP-IDF or FreeRTOS headers. `runtime` knows how to
run a `Work` object but does not know which sensor, controller, or product it runs.
Public dependencies belong in `REQUIRES`; private ones in `PRIV_REQUIRES`.

## Construction and dependency injection

`app_main()` calls `start_device()`. Its function-local static `Device` constructs
queues, adapters, policies, workers, and task runners after ESP-IDF startup. Member
order follows dependency lifetime: dependencies are constructed before borrowers.
All objects remain alive while tasks execute. Startup is one-shot; a second call
aborts. Shutdown/restart of individual task runners is not supported yet.

Workers receive borrowed references through constructors:

- `SensorWorker`: `SensorSource`, `Clock`, and a reading writer.
- `ControlWorker`: policy, clock, reading reader, requests, output, and status writer.
- `ReporterWorker`: status reader and reporting sink.

No worker constructs a driver or looks up global services. The template does not
need a DI container, singleton service registry, or dynamic plugin framework.

## Patterns and boundaries

| Pattern | Application |
| --- | --- |
| Constructor injection / DIP | Workers depend on portable ports |
| Composition root | `composition/src/device.cpp` selects and owns implementations |
| Adapter | ESP clock, FreeRTOS snapshots, serial log sink, simulation drivers |
| Producer-consumer | Sensor/control/reporter exchange bounded snapshots |
| State machine | `TimedOutput` transitions between off, running, and fault |

`Work::step()` performs one iteration. `PeriodicTask` supplies scheduling. This
separates work from FreeRTOS and lets host tests run deterministic single steps.
A second control algorithm may justify a Strategy interface; do not introduce one
until a concrete alternate policy exists.

## Execution

Sensor sampling defaults to 500 ms, control to 100 ms, and reporting to 2000 ms.
Two length-one FreeRTOS queues transfer trivially copyable latest-value snapshots.
Readers do not remove data. Overwrite is intentional: this is not an event history
or a command FIFO. Split reader and writer ports limit what each consumer can do.

Composition seeds a sensor reading and runs one safe control step before starting
tasks. The default request source always returns false. No GPIO or networking is
used. Task stack sizes/priorities need hardware measurement; no core affinity is
required. Allocation happens during startup, not in periodic production workers.
Clock, sensor, request, output, and queue operations used by control must be bounded.

## Output state and errors

`TimedOutput` validates input and deadlines. OFF transitions to running only for
a valid enable request. Invalid input or maximum runtime expiry latches fault.
Repeated ON requests do not reset the deadline. OFF/ON cycles start a new interval;
product rules still need cooldown and duty-cycle limits where required.

`ControlWorker` applies the decision. On output failure it latches a policy fault
and attempts OFF. If OFF fails, status is `AppliedOutput::unknown`, not confirmed
OFF. `AppliedOutput` represents adapter acknowledgement, not physical feedback.
The original enable request is reported separately from applied output.

Fault reset is an internal trusted operation, not an exposed network command.
The next step revalidates input. Nothing here stops hardware if the control task
stalls: independent interlocks/watchdogs are a separate hardware requirement.

## Constants and configuration

Keep named `inline constexpr` values close to the component that owns their meaning:

- `application/constants.hpp`: default runtime and sample-age safety limits.
- `composition/constants.hpp`: device sampling/reporting periods and synthetic value.
- `runtime/constants.hpp`: task stacks/priorities, queue capacity, unit conversions.

Use units in names. Policy values remain constructor parameters so another device
can inject different validated settings. Do not move every zero, boolean, test
fixture value, or protocol example into a global constants file. Future persisted
configuration will be validated and bounded before replacing defaults.

## Coding conventions

Use C++20, `PascalCase` types, `snake_case` functions/files, trailing underscores for
private fields, and English comments/docs. Prefer RAII and explicit ownership.
Exceptions/RTTI remain disabled. Use monotonic local time for safety deadlines.
Never use `volatile` for task synchronization. Run `python3 tools/format.py` with
clang-format 18; CI checks the same formatting rules.

For distributed extensions, see [Cluster and swarm evolution](distributed-systems.md).

## Hardware and networking adapters

Optional SHT3x and Wi-Fi/MQTT implementations are wired only in composition.
SHT3x bus waits stay on the sensor worker; MQTT reporting and reconnect work stay
outside the control task. Runtime configuration loads from NVS schema 1 without
automatic erasure. Physical outputs and remote commands remain disabled.

New registered IDF components use an `espkit_` prefix to avoid SDK/component name
collisions: ESP-IDF already owns `hal`, so our hardware adapters are `espkit_hal`.
Public headers retain the `espkit/hal/...` naming. Follow this rule for new components.
