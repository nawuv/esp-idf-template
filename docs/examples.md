# Examples

All three projects are standalone ESP-IDF applications referencing the repository's
shared components through `EXTRA_COMPONENT_DIRS`. They require no external wiring.
Build each with `idf.py -C examples/<name> set-target esp32s3` followed by
`idf.py -C examples/<name> build`. Each directory includes a README with flashing commands.

## Simulation

`examples/simulation` starts the common device composition: separate sensor, control,
and reporting tasks, with synthetic input and output permanently requested OFF.
The command ingress is constructed and left idle. Expected fields:
`requested=0 applied=1 fault=0`.

## Sensor node

`examples/sensor-node` injects a synthetic source with value 23.5 into a sensor
worker. A periodic task samples every 500 ms; the main task logs the latest sample
every second. Expected fields: `value=23.5 sampled_at_ms=<uptime> valid=1`.

## Actuator node

`examples/actuator-node` continuously requests ON in memory. It refreshes synthetic
input and steps control every 100 ms. After about 1 second, the runtime limit
latches a fault and subsequent requests remain OFF:

```text
output=1 fault=0
output=0 fault=1
```

This example steps workers on the main task for a compact demonstration. The root
simulation demonstrates independent scheduling. Restart resets the example state.

## Validation and future examples

CI compiles every example; compilation does not execute it on hardware. No example
uses real GPIO, Wi-Fi, MQTT, provisioning, or OTA. MQTT and smart-farm examples will
be added alongside those implementations, not as nonfunctional placeholders.
