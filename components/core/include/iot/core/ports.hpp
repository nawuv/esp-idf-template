#pragma once

#include "iot/core/types.hpp"

namespace iot {

class Clock {
    public:
    virtual ~Clock() = default;
    virtual Milliseconds now() const noexcept = 0;
};

class SensorSource {
    public:
    virtual ~SensorSource() = default;
    virtual Reading read(Milliseconds now) noexcept = 0;
};

class OutputSink {
    public:
    virtual ~OutputSink() = default;
    virtual OutputError set_enabled(bool enabled) noexcept = 0;
};

// Implementations must be non-blocking and must not bypass local safety rules.
class RequestSource {
    public:
    virtual ~RequestSource() = default;
    virtual bool enable_requested(Milliseconds now) noexcept = 0;
};

template <typename T> class SnapshotReader {
    public:
    virtual ~SnapshotReader() = default;
    virtual bool latest(T& value) const noexcept = 0;
};

template <typename T> class SnapshotWriter {
    public:
    virtual ~SnapshotWriter() = default;
    virtual void publish(const T& value) noexcept = 0;
};

class TelemetrySink {
    public:
    virtual ~TelemetrySink() = default;
    virtual void report(const Telemetry& value) noexcept = 0;
};

class Work {
    public:
    virtual ~Work() = default;
    virtual void step() noexcept = 0;
};

} // namespace iot
