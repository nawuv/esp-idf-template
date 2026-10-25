# ESPKit

A small C++20 foundation for ESP32 devices, sensor networks, and actuator nodes.
The first target is ESP32-S3 with ESP-IDF **5.4.2**. This is an explicit baseline,
not a claim that this SDK release is the latest.

## Status

Implemented:
- Constructor-injected workers with no ESP-IDF dependencies.
- Explicit output state machine and hardware-error feedback with GoogleTest coverage.
- Separate FreeRTOS sensor, control, and reporting tasks with bounded queues.
- Simulation-only firmware: no GPIO output, credentials, or broker required.
- ESP32-S3 build workflow and host test workflow.
- Unity target-test application (execution requires hardware).
- A 4 MiB flash layout reserving two OTA application slots.
- MkDocs documentation, tutorials, and standalone simulation/sensor/actuator examples.
- Same-run firmware bundles with checksums and source manifests.
- Opt-in GitHub Pages deployment and semantic-release on validated main pushes.

Planned: board drivers, Wi-Fi/MQTT, provisioning, persistent configuration,
OTA download/health confirmation, hardware-driver tests, and networking/smart-farm examples.
These features are not implemented by placeholder folders.

**Do not connect this starter firmware to a live pump.** Hardware output control
and an application-specific safety review are required first. See [Safety](docs/safety.md).

## Start a project

1. Select **Use this template** on GitHub once the repository is marked as a template.
2. Clone your new repository and install/export ESP-IDF 5.4.2.
3. Run:

```sh
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

The example samples a synthetic reading and logs a disabled output state. The actual serial
port varies. The default layout requires at least 4 MiB of flash.
For a different board, review flash size, console settings, and partition capacity.

Alternatively, build with Docker:

```sh
docker run --rm -v "$PWD:/project" -w /project espressif/idf:v5.4.2   bash -c 'idf.py set-target esp32s3 && idf.py build'
```

## Host tests

Requires Git, CMake 3.20+, and a C++20 compiler. The first configure downloads
GoogleTest at a fixed commit; subsequent builds can use the populated cache.

```sh
bash tools/test-host.sh
```

## Structure

| Path | Responsibility |
| --- | --- |
| `main/` | Firmware entry point |
| `components/core/` | Portable time and message types |
| `components/application/` | Portable workers and output policy |
| `components/sensors/` | Synthetic sensor adapter |
| `components/actuators/` | In-memory output adapter |
| `components/runtime/` | FreeRTOS scheduling, snapshots, clock, and log adapters |
| `components/composition/` | Concrete object construction, defaults, and dependency wiring |
| `tests/host/` | GoogleTest tests built without ESP-IDF |
| `docs/` | Architecture, protocols, safety, and roadmap |
| `examples/` | Three standalone simulation projects |
| `.github/workflows/` | Tests, example builds, docs, and opt-in deployment/release |

Start with [Architecture](docs/architecture.md), [Porting](docs/board-porting.md),
[MQTT contract](docs/mqtt-contract.md), [Distributed evolution](docs/distributed-systems.md),
and [Roadmap](docs/roadmap.md).

## Documentation and examples

Read [Getting Started](docs/getting-started/installation.md), [Examples](docs/examples.md),
and [Releases and Branches](docs/releases.md). Preview the English site locally:

```sh
python3 -m venv .venv-docs
. .venv-docs/bin/activate
python -m pip install -r requirements-docs.txt
mkdocs serve
```

Standalone projects live in `examples/simulation`, `examples/sensor-node`, and
`examples/actuator-node`. Each has its own README and is compiled in CI.

Development flows through `feature/* -> develop -> main`. Keep main as the stable
template default. CI builds and tests without secrets. Set `ENABLE_PAGES=true` and/or
`ENABLE_RELEASES=true` as **repository variables** after completing
[repository setup](docs/repository-setup.md). Services remain disabled in new copies
until configured. Main pushes evaluate releases after all checks; docs-only changes
normally do not create a new version.

## Customize

- Rename `espkit_device` in the root CMake project.
- Select sensor/output/reporting implementations in `components/composition/src/device.cpp`.
- Adjust sampling periods in `composition/constants.hpp` and safety defaults in
  `application/constants.hpp`; task memory and priority defaults live in `runtime/constants.hpp`.
- Add board wiring before implementing a physical output.
- Keep business rules independent of ESP-IDF and test them on the host.
- Keep credentials and signing keys out of source control and build artifacts.
- Review the included MIT license and update attribution for your project.

## Reproducibility

The SDK and GoogleTest baseline are pinned. GitHub Actions and GoogleTest use commit pins; CI also pins the SDK container
digest. Update these pins deliberately when upgrading the baseline.
Do not commit generated `sdkconfig` or build directories. Review and commit
changes to `dependencies.lock` when dependencies change.

## License

[MIT](LICENSE).
