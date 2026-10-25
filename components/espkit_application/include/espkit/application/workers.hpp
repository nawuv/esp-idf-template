#pragma once

#include "espkit/application/ports/device.hpp"
#include "espkit/domain/timed_output.hpp"

namespace espkit::application {

// Dependencies are borrowed and must outlive the worker. Each worker has one owner.
class SensorWorker final : public Work {
    public:
    SensorWorker(SensorSource& sensor, Clock& clock,
                 SnapshotWriter<espkit::domain::Reading>& readings)
        : sensor_(sensor), clock_(clock), readings_(readings) {
    }

    void step() noexcept override;

    private:
    SensorSource& sensor_;
    Clock& clock_;
    SnapshotWriter<espkit::domain::Reading>& readings_;
};

class ControlWorker final : public Work {
    public:
    ControlWorker(espkit::domain::TimedOutput& policy, Clock& clock,
                  const SnapshotReader<espkit::domain::Reading>& readings, RequestSource& requests,
                  OutputSink& output, SnapshotWriter<espkit::domain::Telemetry>& telemetry)
        : policy_(policy), clock_(clock), readings_(readings), requests_(requests), output_(output),
          telemetry_(telemetry) {
    }

    void step() noexcept override;

    private:
    espkit::domain::TimedOutput& policy_;
    Clock& clock_;
    const SnapshotReader<espkit::domain::Reading>& readings_;
    RequestSource& requests_;
    OutputSink& output_;
    SnapshotWriter<espkit::domain::Telemetry>& telemetry_;
};

class ReporterWorker final : public Work {
    public:
    ReporterWorker(const SnapshotReader<espkit::domain::Telemetry>& telemetry, TelemetrySink& sink)
        : telemetry_(telemetry), sink_(sink) {
    }

    void step() noexcept override;

    private:
    const SnapshotReader<espkit::domain::Telemetry>& telemetry_;
    TelemetrySink& sink_;
};

} // namespace espkit::application
