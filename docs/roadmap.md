# Roadmap

## Foundation (implemented)

Portable types, injected workers, output state machine/error handling, component-owned
constants, a dedicated composition root, simulation adapters, host tests, firmware
build CI, English MkDocs documentation, standalone examples, opt-in Pages/release
automation, packaged firmware provenance, and reserved OTA partitions.

## First hardware slice (code available; hardware validation pending)

Optional SHT3x, I2C adapters, NVS runtime settings, and MQTT telemetry now compile.
Confirm the actual ESP32-S3 board, sensor, and actuator wiring. Add board/HAL/driver components,
output error handling, interlocks, irrigation hysteresis, cooldown, and Unity tests.
Keep smart-farm rules out of reusable networking and hardware components.

## Networking

Wi-Fi and MQTT telemetry are implemented as opt-in services. Next add a bounded
command queue, local command
validation, TLS, telemetry, and integration tests. Do not add dependencies until
used; then commit the managed dependency lockfile. Pin versions compatible with
the chosen SDK baseline.

## Configuration and provisioning

Add typed configuration, range validation, schema version, NVS persistence, and
provisioning through supported ESP-IDF mechanisms. Design migrations for both
upgrade and rollback. Never commit per-device credentials.

## OTA and fleet operation

Add HTTPS OTA, image verification, board/version compatibility checks, safe output
quiescing, and local health confirmation/rollback. Do not require broker availability
to mark a healthy device valid. The current partition table reserves two app slots;
it does not implement updates. Budget image size against each slot.

Add release manifests, device identity, diagnostics, staged rollout, certificate
rotation, and backend fleet inventory. A firmware template alone is not a fleet
management backend. Keep credentials out of CI artifacts.

## Reuse and publishing

Simulation, sensor-node, and actuator-node examples now build independently.
Enable GitHub's template repository setting, review the MIT
license, verify a new repository can build without secrets, then tag the first release.
Keep CI action commits and the build image digest pinned when upgrading dependencies.

## Cluster and swarm products

Follow [distributed-system boundaries](distributed-systems.md). Add transport, identity,
leases, peer handling, and product-specific coordination only with a concrete node
scenario and timing budget. Local interlocks remain independent of coordination.
