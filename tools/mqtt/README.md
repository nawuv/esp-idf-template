# Local development broker

```sh
docker compose -f tools/mqtt/compose.yml up -d
```

Default binding is localhost only. For an ESP32 on an isolated LAN, explicitly set
`MQTT_BIND_ADDRESS` to the development machine's LAN address before starting the
broker. Configure the device with `mqtt://<LAN-address>:1883`, enable the plaintext
Kconfig option, and generate NVS with `--allow-plaintext`.

This broker allows anonymous access and has no TLS. It is a local testing tool,
not a production broker. A physical device connection is still needed to validate
the ESP-IDF Wi-Fi/MQTT adapter. Inspect messages using:

```sh
docker compose -f tools/mqtt/compose.yml exec broker \
  mosquitto_sub -h localhost -t 'iot/v1/+/+/telemetry' -v
```

Stop it with `docker compose -f tools/mqtt/compose.yml down`.
