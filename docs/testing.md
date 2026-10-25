# Testing

## Host

Run `bash tools/test-host.sh`. GoogleTest builds the production policy source with
C++20 and strict warnings, without linking ESP-IDF. Tests cover timeout boundaries,
non-extending requests, sensor validity/freshness, clock regression, invalid policy,
and fault reset. Worker tests inject alternate sensors, a fake clock, in-memory
snapshots, request sources, and failed outputs. They verify missing/stale input,
fault propagation, failed-stop unknown state, and reporting without ESP-IDF.
Add domain-specific boundary tests when control rules grow.

## Target

A Unity policy smoke test lives in `components/application/test/`, with a dedicated
application in `tests/target/`. Build and flash it using:

```sh
idf.py -C tests/target set-target esp32s3
idf.py -C tests/target build
idf.py -C tests/target -p /dev/ttyACM0 flash monitor
```

Select the application test from the serial Unity menu. These tests require a
physical board to execute. Validate GPIO polarity, NVS, bus timeouts, RTOS timing,
stack high-water marks, and driver behavior on physical hardware. Use a protected
self-hosted runner for HIL. Never expose connected actuators to untrusted PR jobs.

## Integration

When networking is added, run a local test broker and inject disconnects, invalid
credentials, duplicate and expired commands, fragmented payloads, queue pressure,
and simultaneous reconnects. Observe control deadlines while the network is busy.

CI compiles firmware and executes host tests. It does not claim on-device validation.
