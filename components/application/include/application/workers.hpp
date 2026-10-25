#pragma once

#include "application/ports/device.hpp"
#include "domain/timed_output.hpp"

namespace application {

// Dependencies are borrowed and must outlive the worker. Each worker has one owner.
class SensorWorker final : public Work {
    public:
    SensorWorker(SensorSource& sensor, Clock& clock, SnapshotWriter<domain::Reading>& readings)
        : sensor_(sensor), clock_(clock), readings_(readings) {
    }

    void step() noexcept override;

    private:
    SensorSource& sensor_;
    Clock& clock_;
    SnapshotWriter<domain::Reading>& readings_;
};

class ControlWorker final : public Work {
    public:
    ControlWorker(domain::TimedOutput& policy, Clock& clock,
                  const SnapshotReader<domain::Reading>& readings, RequestSource& requests,
                  OutputSink& output, SnapshotWriter<domain::Telemetry>& telemetry)
        : policy_(policy), clock_(clock), readings_(readings), requests_(requests), output_(output),
          telemetry_(telemetry) {
    }

    void step() noexcept override;

    private:
    domain::TimedOutput& policy_;
    Clock& clock_;
    const SnapshotReader<domain::Reading>& readings_;
    RequestSource& requests_;
    OutputSink& output_;
    SnapshotWriter<domain::Telemetry>& telemetry_;
};

class ReporterWorker final : public Work {
    public:
    ReporterWorker(const SnapshotReader<domain::Telemetry>& telemetry, TelemetrySink& sink)
        : telemetry_(telemetry), sink_(sink) {
    }

    void step() noexcept override;

    private:
    const SnapshotReader<domain::Telemetry>& telemetry_;
    TelemetrySink& sink_;
};

} // namespace application
