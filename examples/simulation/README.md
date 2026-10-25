# simulation

Runs the shared sensor, control, and reporter composition. All outputs stay disabled.

## Requirements

ESP-IDF 5.4.2 and an ESP32-S3 development board for execution. No external sensor,
actuator, Wi-Fi credentials, or broker is required. GPIO is never accessed.
These examples use the SDK default flash layout; the root firmware reserves OTA slots.

## Build and run

From the repository root:

```sh
idf.py -C examples/simulation set-target esp32s3
idf.py -C examples/simulation build
idf.py -C examples/simulation -p /dev/ttyACM0 flash monitor
```

## Expected output

Illustrative log fields; timestamps and prefixes vary:

```text
requested=0 applied=1 fault=0
```

## Validation

CI compiles this project. On-device execution requires a physical board and is
not implied by a successful build. The actuator example only changes memory.

## Next step

Read the repository's `docs/architecture.md` and tutorials. Replace concrete
adapters in composition without adding SDK calls to application workers.
