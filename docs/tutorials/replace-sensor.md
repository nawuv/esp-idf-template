# Replace the synthetic sensor

## Understand the boundary

`SensorWorker` receives a `SensorSource&`, `Clock&`, and reading writer. It does not
construct a particular sensor or call ESP-IDF. See `components/application/include/application/ports/device.hpp`.

## Implement a source

```cpp
class MySensor final : public application::SensorSource {
    public:
    domain::Reading read(domain::Milliseconds now) noexcept override {
        // Replace with bounded I/O and actual conversion.
        return {25.0F, now, true};
    }
};
```

This skeleton still returns synthetic data. A real driver must report failures
with `valid=false`, bound bus waits, and define units/ranges. `sampled_at` must
represent measurement freshness in the same local monotonic clock domain.

## Wire the implementation

In your composition, construct `MySensor`, then pass it into `SensorWorker` in
place of `SyntheticSensor`. The source and its dependencies must outlive the worker
and running task. Add the component's CMake dependencies explicitly.

## Verify

Inject a fake source and clock into host tests. Test successful reads, failures,
invalid values, and stale data. Build firmware, then validate bus and timing on
the board. The [sensor-node example](../examples.md#sensor-node) is a small starting point.
