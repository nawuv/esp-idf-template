# ESPKit configurations

`hardware.defaults` enables SHT3x and MQTT for compile coverage, with GPIOs left
unset. It contains no credentials and never enables physical outputs.

`device.example.json` is the runtime credential example for NVS schema 1. Copy it
under ignored `local/`, edit privately, and use `tools/provision_device.py` in an
ESP-IDF shell. The tool stamps `schema` as 1. Do not put actual credentials in
this directory or sdkconfig defaults.

Configuration uses the root firmware's 4 MiB partition layout. Refer to
`docs/hardware-mqtt.md` before flashing an NVS image or configuring real wiring.
