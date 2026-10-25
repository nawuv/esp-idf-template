# Constants and configuration

| Location | Owns |
| --- | --- |
| `domain/policy_defaults.hpp` | Default maximum runtime and sample age |
| `main/device_config.hpp` | Device periods, task budgets, selected policy, and synthetic value |
| `platform/constants.hpp` | Queue capacity and time conversions |

Use named `inline constexpr` values with units in their names. Keep test scenario
values local to each test. Do not create a global miscellaneous constants file.

`OutputPolicy` is a value passed to the controller, so another product can inject
its own limits. Validate configuration before use. The controller rejects nonpositive
safety durations; this is not a replacement for product-specific range validation.

Runtime network settings can now be provisioned into NVS schema 1; see
[Hardware and MQTT](../hardware-mqtt.md). Product settings, schema migrations,
secure provisioning, and remote updates are not implemented yet. When introduced, invalid configuration must not overwrite the
last valid configuration or bypass local limits. Consider rollback compatibility.
