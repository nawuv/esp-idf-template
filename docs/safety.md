# Safety and failure behavior

This repository currently simulates all I/O. It does not control a pump.

The portable policy disables output for invalid, non-finite, stale, or future-dated
sensor values, invalid durations, a regressing clock, and maximum runtime expiry.
Faults latch until an explicit reset while output is off. Physical value ranges
and units must be validated by a product-specific policy.

Before enabling real hardware:

1. Set a safe output latch before enabling its GPIO direction. Verify boot/reset
   pin behavior, relay polarity, and a default-OFF circuit.
2. Have one control task own each output, with bounded driver calls.
3. Enforce maximum on-time, minimum off-time, sample freshness, and duty limits.
4. Keep safe local control independent of MQTT connectivity.
5. Give remote manual commands a deadline and a bounded runtime. A retransmission
   must never restart the runtime deadline. Do not expose unrestricted fault reset.
6. Provide a stop/interlock path that cannot be delayed by a full command queue.
7. Validate watchdog behavior, brownouts, sensor disconnects, bus stalls, reboot,
   output write failure, and storage corruption on the actual board.
8. Use independent hardware cutoff where the consequences require it. A stalled
   task, stuck relay, or failed transistor cannot be handled by policy code alone.

A network outage does not itself force every product off: verified local automatic
operation may continue. Missing or invalid safety inputs do force a safe state.
Select these rules explicitly for each product. The starter defaults to OFF.

OTA must quiesce outputs and confirm this before restarting. The current partition
layout only reserves storage; OTA download and health confirmation are not present.
