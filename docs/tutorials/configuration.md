# Constants and configuration

| Location | Owns |
| --- | --- |
| `application/constants.hpp` | Default maximum runtime and sample age |
| `composition/constants.hpp` | Device execution periods and synthetic value |
| `runtime/constants.hpp` | Task memory, priorities, queue capacity, conversions |

Use named `inline constexpr` values with units in their names. Keep test scenario
values local to each test. Do not create a global miscellaneous constants file.

`OutputPolicy` is a value passed to the controller, so another product can inject
its own limits. Validate configuration before use. The controller rejects nonpositive
safety durations; this is not a replacement for product-specific range validation.

Runtime network settings can now be provisioned into NVS schema 1; see
[Hardware and MQTT](../hardware-mqtt.md). Product settings, schema migrations,
secure provisioning, and remote updates are not implemented yet. When introduced, invalid configuration must not overwrite the
last valid configuration or bypass local limits. Consider rollback compatibility.
