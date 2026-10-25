#pragma once

#include "espkit/domain/types.hpp"

namespace espkit::application {

class Clock {
    public:
    virtual ~Clock() = default;
    virtual espkit::domain::Milliseconds now() const noexcept = 0;
};

class SensorSource {
    public:
    virtual ~SensorSource() = default;
    virtual espkit::domain::Reading read(espkit::domain::Milliseconds now) noexcept = 0;
};

class OutputSink {
    public:
    virtual ~OutputSink() = default;
    virtual espkit::domain::OutputError set_enabled(bool enabled) noexcept = 0;
};

// Implementations must be non-blocking and must not bypass local safety rules.
class RequestSource {
    public:
    virtual ~RequestSource() = default;
    virtual bool enable_requested(espkit::domain::Milliseconds now) noexcept = 0;
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
    virtual void report(const espkit::domain::Telemetry& value) noexcept = 0;
};

class Work {
    public:
    virtual ~Work() = default;
    virtual void step() noexcept = 0;
};

} // namespace espkit::application
