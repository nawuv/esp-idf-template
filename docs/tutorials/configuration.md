# Constants and configuration

| Location | Owns |
| --- | --- |
| `domain/policy_defaults.hpp` | Default maximum runtime and sample age |
| `main/device_config.hpp` | Device periods, task budgets, selected policy, and synthetic value |
| `platform/constants.hpp` | Queue capacity and time conversions |
| `configuration/network_config.hpp` | Runtime network and identity fields, and their storage capacities |
| `configuration/settings_migration.hpp` | Schema version, upgrade, and rollback rules for those fields |

Use named `inline constexpr` values with units in their names. Keep test scenario
values local to each test. Do not create a global miscellaneous constants file.

`OutputPolicy` is a value passed to the controller, so another product can inject
its own limits. Validate configuration before use. The controller rejects nonpositive
safety durations; this is not a replacement for product-specific range validation.

Runtime network settings are provisioned into NVS schema 1. See
[Hardware and MQTT](../hardware-mqtt.md) and
[ADR 0006](../decisions/0006-versioned-settings.md). The durable record lives in
the `espkit` namespace. Loading it never erases or rewrites flash. A missing
`schema` key is the unversioned layout: when the schema 1 fields validate, this
firmware uses them as schema 1 in memory. A schema newer than this firmware is
used only through those same fields. Corrupt, partial, and out-of-range records
are not applied, and they are not clamped. Optional MQTT username and password
default to empty. SSID, password, broker URI, device id, and site id do not get
invented defaults.

Command queue depth, command limits, and `ESPKIT_REMOTE_COMMANDS` are not stored
in NVS. They stay in `device_config.hpp` and Kconfig. Secure provisioning,
encrypted NVS, and remote configuration updates are not implemented yet.

## Add a runtime setting later

1. Add the field to `NetworkConfig` with a named capacity. Add the range check to
   `valid_network_config` in the portable configuration component.
2. Decide the stored shape. An optional key keeps `schema` at 1, defaults when
   the key is absent, and is not required. Firmware that still checks
   `schema == 1` then keeps reading the existing keys. Do not reuse a key name
   for a new meaning.
3. Read the key in `NvsConfigStore` without truncating a value that does not fit.
   Do not add a write-back. Add the same key and limit to `tools/provision_device.py`.
4. Extend `migrate_settings` and add a host test for upgrade, a newer schema,
   a missing key, and an out-of-range value. If the new field cannot be defaulted,
   add a named migration step and raise `settings_schema_current`. The static
   assert in `settings_migration.cpp` fails until that step exists. Leave the
   schema 1 keys valid if a rollback should still join the network.
5. Do not store per-device credentials in git, Kconfig defaults, or CI artifacts.
