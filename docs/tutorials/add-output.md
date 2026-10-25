# Add an output adapter

Implement `OutputSink::set_enabled(bool)` and return an `OutputError`. Select the
implementation in composition; never write GPIO in an MQTT callback.

```cpp
class MyOutput final : public iot::OutputSink {
    public:
    iot::OutputError set_enabled(bool enabled) noexcept override {
        // Apply the request through a bounded driver operation.
        // Return hardware_fault or unavailable on failure.
        return iot::OutputError::unavailable;
    }
};
```

This skeleton intentionally reports unavailable and cannot drive hardware.
The control worker latches a fault on failure and attempts OFF. If that attempt
also fails, it reports `AppliedOutput::unknown`. Success is an adapter
acknowledgement, not proof that a motor stopped or a relay opened.

Before wiring real hardware, implement safe startup, polarity selection, driver
error feedback, and required independent interlocks. See [Safety](../safety.md).
Use [actuator-node](../examples.md#actuator-node) to study the timing policy without GPIO.
