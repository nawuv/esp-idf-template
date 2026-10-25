# Build, flash, and monitor

From the repository root in an activated ESP-IDF 5.4.2 environment:

```sh
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Replace the port with your board's actual port. ESP-IDF monitor exits with `Ctrl+]`.
The root firmware uses the custom 4 MiB layout with two reserved OTA app slots.

## Expected output

Illustrative fields; timing and prefixes vary:

```text
mode=simulation uptime_ms=2000 requested=0 applied=1 fault=0 error=0
```

`requested=0` means no enable request. `applied=1` is the enum value for acknowledged
OFF (`unknown=0`, `off=1`, `on=2`). This is software acknowledgement, not physical
feedback. `fault=0` means no latched control fault, and `error=0` means no current
output-adapter error. No sensor values are sent over a network.

## Docker build

```sh
docker run --rm -v "$PWD:/project" -w /project espressif/idf:v5.4.2 \
  bash -c 'idf.py set-target esp32s3 && idf.py build'
```

The container command only builds. USB passthrough and flashing depend on the host.

## Try an example

```sh
idf.py -C examples/sensor-node set-target esp32s3
idf.py -C examples/sensor-node build
```

Each [example](../examples.md) has its own entry point and generated configuration.
