# Installation

## Firmware toolchain

Install ESP-IDF **5.4.2** using Espressif's instructions for your operating system:
[ESP32-S3 setup guide](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32s3/get-started/index.html).
Activate its environment in each development shell and check:

```sh
idf.py --version
```

The component manifest requires this exact SDK baseline. Upgrade it deliberately,
including CI's container and tests. ESP-IDF uses CMake internally; you do not need
Arduino or PlatformIO.

## Hardware

The root project expects an ESP32-S3 board with at least 4 MiB flash. Examples
use the SDK default layout. No external wiring is needed for the simulation.
Hardware execution requires a suitable USB cable and serial-port access.

## Host tests

Install Git, CMake 3.20+, and a C++20 compiler. GoogleTest is fetched at a pinned
commit on the first configure. These tests do not require an ESP32.

## Documentation

Python 3.10+ is a practical supported baseline for the documentation commands:

```sh
python3 -m venv .venv-docs
. .venv-docs/bin/activate
python -m pip install -r requirements-docs.txt
mkdocs serve
```

Open the local URL printed by MkDocs. Build the deployable site with
`mkdocs build --strict`. See [create your project](create-project.md) next.
