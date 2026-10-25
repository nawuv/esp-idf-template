# MQTT contract proposal

Status: design only; no MQTT client or Wi-Fi implementation is included yet.

## Topics

```text
iot/v1/{site_id}/{device_id}/telemetry
iot/v1/{site_id}/{device_id}/state
iot/v1/{site_id}/{device_id}/availability
iot/v1/{site_id}/{device_id}/commands
iot/v1/{site_id}/{device_id}/command-results
```

Use per-device credentials and ACLs, TLS server verification, bounded reconnect
backoff with jitter, and a last will for availability. Broker-reported presence
is not a safety input. Telemetry may be lossy; safety faults need a separate
observable record. Define broker limits and offline storage limits explicitly.

## Commands

```json
{
  "schema_version": 1,
  "command_id": "8f36a7b1",
  "type": "set_output",
  "enabled": true,
  "issued_at": "2026-10-25T03:00:00Z",
  "expires_at": "2026-10-25T03:00:15Z",
  "max_duration_ms": 5000
}
```

Never retain one-shot actuator commands. Define maximum topic, payload, and queue
sizes. Reassemble fragmented MQTT payloads into bounded owned storage before
parsing. Reject malformed, expired, unsupported, or unauthorized requests.
Reject time-dependent enable commands before trusted time is available.
Deduplicate IDs with a defined lifetime and reboot policy. MQTT QoS does not
replace application-level idempotency for physical operations.

Acknowledge acceptance separately from completion. Queue saturation produces an
explicit rejection when transport is available; it must not silently accept work.
A reconnect must not replay expired commands. A device may shorten requested
runtime, and all remote requests remain subject to local interlocks.
