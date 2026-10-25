# ADR 0001: Portable control and safe simulation default

Status: accepted

## Context

The template must support multiple products without assuming a pin map, sensor,
broker, or provisioning backend. A first clone must build without credentials.
Network availability must not determine whether local safety rules execute.

## Decision

Use explicit C++20 and pin ESP-IDF 5.4.2 as a reproducible initial baseline.
Keep control policy independent of ESP-IDF. Compose bounded sensor, control,
and reporting flows using runtime scheduling and composition-owned wiring
(as refined in ADR 0002). Use GoogleTest on the host and
Unity on target. Default to synthetic input and in-memory output only.

Reserve OTA storage early but add its implementation only alongside local health
checks, output quiescing, and rollback-compatible configuration migrations.
Use MIT licensing for broad reuse. Track additional hardware and networking work
in the roadmap rather than adding empty framework components.

## Consequences

The first build demonstrates boundaries and policy tests, not a deployed smart
farm. A concrete board profile and hardware fault feedback are needed before
physical actuation. The SDK baseline must receive deliberate compatibility and
security maintenance; pinning does not replace updates.
