# ADR 0006: Versioned network settings with in-memory migration

Status: accepted.

## Context

Devices already store Wi-Fi and MQTT settings in the NVS namespace `espkit`.
NVS is the ESP-IDF key-value store in flash. The provisioned layout is schema 1:
a `schema` byte plus `wifi_ssid`, `wifi_pass`, `mqtt_uri`, `device_id`,
`site_id`, and optional `mqtt_user` and `mqtt_pass`. Firmware accepted that
record only when `schema` was exactly 1. A missing version, a newer version, a
short password, and a truncated string all failed the same way.

A field device can boot older firmware after a bad update. That older firmware
reads whatever the newer firmware left in flash. Rewriting the namespace on
boot would wear flash and could erase keys the older image still needs.
[ADR 0001](0001-portable-control-and-simulation.md) defers OTA until migrations
are rollback-safe. [ADR 0004](0004-portable-component-api.md) keeps this
namespace and schema compatible with devices that are already provisioned.
[ADR 0005](0005-command-ingress.md) keeps command admission in RAM and Kconfig.
Those values are not per-device credentials.

`ESPKIT_NETWORK_ENABLED`, `ESPKIT_ALLOW_PLAINTEXT_MQTT`, and
`ESPKIT_REMOTE_COMMANDS` stay compile-time options. Plaintext MQTT is a firmware
policy, not a stored secret.

## Decision

Keep the on-disk layout at schema 1. Validate and migrate in portable
`migrate_settings`. `NvsConfigStore::load` only reads the namespace into a
`StoredSettings` value and calls that function. The adapter does not truncate
an oversized string, enumerate keys, erase keys, or write keys back.
`nvs_flash_init` failure still does not erase the partition.

| Observed storage | Result |
| --- | --- |
| No `schema` key, and the schema 1 fields validate | `migrated`. The in-memory result is schema 1. Missing broker credentials become empty. |
| `schema == 1` and the fields validate | `current`. |
| `schema > 1` and the schema 1 fields still validate | `compatible_newer`. Unknown keys stay stored and are not read. |
| Unreadable `schema`, `schema == 0`, a field with no terminating NUL, or a value that does not fit its buffer | `rejected`, problem `corrupt`. |
| A required schema 1 key is absent | `rejected`, problem `partial`. When `schema > 1`, the problem is `incompatible`: a newer layout may have replaced keys, so this is not a damaged v1 record. |
| Fields are well formed but fail `valid_network_config` | `rejected`, problem `out_of_range`. Values are not clamped. |
| More than one of the above | `corrupt` wins over `partial` and `out_of_range`. |

A rejected result carries a zeroed `NetworkConfig`. `load` leaves the caller's
struct unchanged. The composition root calls `NetworkService::configure` only
when `load` returns true, so local control continues with networking off.

The only upgrade step today is unversioned storage to v1, and v1 is the current
schema. `settings_schema_current` is statically asserted equal to
`settings_schema_v1`. Raising the current schema has to add a named step in
`settings_migration.cpp` and remove that assert. Key names keep their schema 1
meaning. A new meaning needs a new key.

A new optional key should keep the stored schema at 1 and default when absent.
Firmware that still checks `schema == 1` then keeps reading the old keys. Bump
the stored schema only when older firmware must not apply the record. After a
bump, leave the v1 keys in place if a rollback should keep networking. Do not
bump it until the oldest firmware you will roll back to understands a newer
schema. Firmware from before this change rejects any schema other than 1.

Command queue depth, command limits, duplicate history, and
`ESPKIT_REMOTE_COMMANDS` are not NVS settings.

`tools/provision_device.py` still writes schema 1 and the same keys. It rejects
unknown JSON keys. Credentials stay out of git.

## Alternatives rejected

- **Rewrite NVS when the in-memory schema changes.** A rollback would then read
  the rewritten blob. Load stays read-only, so bytes written by an older image
  remain in place.
- **Reject any schema other than 1.** That was the previous check. It treats a
  missing version and a newer version as the same failure, and it drops
  networking on rollback even when the credentials are unchanged.
- **Accept a newer schema while ignoring validation failures.** A damaged
  password must not become an empty password and an anonymous broker session.
- **Clamp out-of-range values.** Shortening a password or a URI changes which
  network the device joins. Reject the record instead.
- **Default a missing SSID or broker.** A default network would ship in every
  device. Optional MQTT username and password default to empty because empty is
  already valid schema 1. Required fields do not.
- **Persist command-ingress settings.** ADR 0005 rejected persisted idempotency.
  Queue sizes and remote admission are product and Kconfig choices, not
  per-device credentials.
- **A migration registry, versioned document object, or code generator.** One
  function per published step is enough for a single settings struct. A table
  of callbacks would be a service locator for two versions.
- **Store a second copy under a new namespace.** Already-provisioned devices
  would look unconfigured, and the tool would have two images to flash.
- **Bump to schema 2 without a new field.** Firmware already in the field
  rejects `schema != 1`. A version bump with the same keys only breaks that
  rollback.

## Consequences

Images from `tools/provision_device.py`, and devices already programmed with
schema 1, load as `current`. Kconfig option names are unchanged.
`ESPKIT_ALLOW_PLAINTEXT_MQTT` still decides whether `mqtt://` validates.
Unversioned records that contain a valid schema 1 field set start networking
on this firmware. Older firmware still ignores those records. Neither path
erases NVS.

This firmware reads a newer schema through the schema 1 keys when those keys
still validate. Firmware from before this change leaves networking off if the
stored schema is not 1. Host tests cover the migration function. They do not
open NVS. Hardware validation was not performed. OTA still waits for image
checks, output quiescing, and local health confirmation.
