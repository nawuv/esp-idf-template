#pragma once

#include "espkit/application/timed_output.hpp"
#include "espkit/core/ports.hpp"

namespace espkit {

// Dependencies are borrowed and must outlive the worker. Each worker has one owner.
class SensorWorker final : public Work {
    public:
    SensorWorker(SensorSource& sensor, Clock& clock, SnapshotWriter<Reading>& readings)
        : sensor_(sensor), clock_(clock), readings_(readings) {
    }

    void step() noexcept override;

    private:
    SensorSource& sensor_;
    Clock& clock_;
    SnapshotWriter<Reading>& readings_;
};

class ControlWorker final : public Work {
    public:
    ControlWorker(TimedOutput& policy, Clock& clock, const SnapshotReader<Reading>& readings,
                  RequestSource& requests, OutputSink& output, SnapshotWriter<Telemetry>& telemetry)
        : policy_(policy), clock_(clock), readings_(readings), requests_(requests), output_(output),
          telemetry_(telemetry) {
    }

    void step() noexcept override;

    private:
    TimedOutput& policy_;
    Clock& clock_;
    const SnapshotReader<Reading>& readings_;
    RequestSource& requests_;
    OutputSink& output_;
    SnapshotWriter<Telemetry>& telemetry_;
};

class ReporterWorker final : public Work {
    public:
    ReporterWorker(const SnapshotReader<Telemetry>& telemetry, TelemetrySink& sink)
        : telemetry_(telemetry), sink_(sink) {
    }

    void step() noexcept override;

    private:
    const SnapshotReader<Telemetry>& telemetry_;
    TelemetrySink& sink_;
};

} // namespace espkit
