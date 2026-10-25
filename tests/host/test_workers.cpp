#include "application/workers.hpp"
#include "drivers/simulated_output.hpp"
#include "drivers/synthetic_sensor.hpp"

#include <gtest/gtest.h>
#include <vector>

namespace {

class FakeClock final : public application::Clock {
    public:
    domain::Milliseconds time{0};

    domain::Milliseconds now() const noexcept override {
        return time;
    }
};

template <typename T>
class MemorySnapshot final : public application::SnapshotReader<T>,
                             public application::SnapshotWriter<T> {
    public:
    bool present{false};
    T value{};

    bool latest(T& destination) const noexcept override {
        if (!present) {
            return false;
        }
        destination = value;
        return true;
    }

    void publish(const T& source) noexcept override {
        value = source;
        present = true;
    }
};

class AlternateSensor final : public application::SensorSource {
    public:
    domain::Reading read(domain::Milliseconds now) noexcept override {
        return {17.5F, now, true};
    }
};

class Requests final : public application::RequestSource {
    public:
    bool enabled{false};

    bool enable_requested(domain::Milliseconds) noexcept override {
        return enabled;
    }
};

class FailingOutput final : public application::OutputSink {
    public:
    bool fail_stop{false};
    std::vector<bool> calls;

    domain::OutputError set_enabled(bool enabled) noexcept override {
        calls.push_back(enabled);
        return enabled || fail_stop ? domain::OutputError::hardware_fault
                                    : domain::OutputError::none;
    }
};

class CaptureTelemetry final : public application::TelemetrySink {
    public:
    unsigned calls{0};
    domain::Telemetry value{};

    void report(const domain::Telemetry& telemetry) noexcept override {
        ++calls;
        value = telemetry;
    }
};

TEST(SensorWorker, UsesInjectedSensorAndClock) {
    FakeClock clock;
    clock.time = 321;
    AlternateSensor sensor;
    MemorySnapshot<domain::Reading> readings;
    application::SensorWorker worker(sensor, clock, readings);
    worker.step();
    ASSERT_TRUE(readings.present);
    EXPECT_FLOAT_EQ(readings.value.value, 17.5F);
    EXPECT_EQ(readings.value.sampled_at, 321);
}

TEST(SensorWorker, SensorImplementationCanBeReplacedWithoutChangingWorker) {
    FakeClock clock;
    drivers::SyntheticSensor sensor(12.0F);
    MemorySnapshot<domain::Reading> readings;
    application::SensorWorker worker(sensor, clock, readings);
    worker.step();
    EXPECT_FLOAT_EQ(readings.value.value, 12.0F);
}

TEST(ControlWorker, MissingReadingFailsClosed) {
    FakeClock clock;
    MemorySnapshot<domain::Reading> readings;
    MemorySnapshot<domain::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    drivers::SimulatedOutput output;
    domain::TimedOutput policy({1000, 200});
    application::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    worker.step();
    EXPECT_FALSE(output.enabled());
    EXPECT_TRUE(policy.fault());
    EXPECT_EQ(telemetry.value.applied_output, domain::AppliedOutput::off);
}

TEST(ControlWorker, RunsLocallyAndStopsWhenSampleExpires) {
    FakeClock clock;
    MemorySnapshot<domain::Reading> readings;
    readings.publish({42.0F, 0, true});
    MemorySnapshot<domain::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    drivers::SimulatedOutput output;
    domain::TimedOutput policy({1000, 200});
    application::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    worker.step();
    ASSERT_TRUE(output.enabled());
    clock.time = 201;
    worker.step();
    EXPECT_FALSE(output.enabled());
    EXPECT_TRUE(telemetry.value.fault);
}

TEST(ControlWorker, OutputFailureLatchesFaultAndAttemptsStop) {
    FakeClock clock;
    MemorySnapshot<domain::Reading> readings;
    readings.publish({42.0F, 0, true});
    MemorySnapshot<domain::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    FailingOutput output;
    domain::TimedOutput policy({1000, 200});
    application::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    worker.step();
    EXPECT_EQ(output.calls, (std::vector<bool>{true, false}));
    EXPECT_TRUE(policy.fault());
    EXPECT_EQ(telemetry.value.output_error, domain::OutputError::hardware_fault);
    EXPECT_EQ(telemetry.value.applied_output, domain::AppliedOutput::off);
    worker.step();
    EXPECT_FALSE(output.calls.back());
    EXPECT_TRUE(telemetry.value.fault);
}

TEST(ControlWorker, FailedStopIsReportedAsUnknown) {
    FakeClock clock;
    MemorySnapshot<domain::Reading> readings;
    readings.publish({42.0F, 0, true});
    MemorySnapshot<domain::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    FailingOutput output;
    output.fail_stop = true;
    domain::TimedOutput policy({1000, 200});
    application::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    worker.step();
    EXPECT_TRUE(policy.fault());
    EXPECT_EQ(telemetry.value.applied_output, domain::AppliedOutput::unknown);
}

TEST(ReporterWorker, ReportsLatestSnapshotAndSkipsMissingData) {
    MemorySnapshot<domain::Telemetry> telemetry;
    CaptureTelemetry sink;
    application::ReporterWorker worker(telemetry, sink);
    worker.step();
    EXPECT_EQ(sink.calls, 0U);
    telemetry.publish({123, false, domain::AppliedOutput::off, false, domain::OutputError::none});
    worker.step();
    EXPECT_EQ(sink.calls, 1U);
    EXPECT_EQ(sink.value.uptime, 123);
}

TEST(TimedOutput, StateTransitionsAreExplicit) {
    domain::TimedOutput policy({1000, 200});
    EXPECT_EQ(policy.state(), domain::OutputState::off);
    ASSERT_TRUE(policy.update(true, {42, 0, true}, 0));
    EXPECT_EQ(policy.state(), domain::OutputState::running);
    policy.trip_fault();
    EXPECT_EQ(policy.state(), domain::OutputState::fault);
    EXPECT_FALSE(policy.update(true, {42, 1, true}, 1));
    ASSERT_TRUE(policy.reset_fault());
    EXPECT_EQ(policy.state(), domain::OutputState::off);
}

TEST(ControlWorker, EnforcesRuntimeEvenWhenReporterIsNeverScheduled) {
    FakeClock clock;
    MemorySnapshot<domain::Reading> readings;
    MemorySnapshot<domain::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    drivers::SimulatedOutput output;
    domain::TimedOutput policy({1000, 200});
    application::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    for (domain::Milliseconds now = 0; now <= 1000; now += 100) {
        clock.time = now;
        readings.publish({42.0F, now, true});
        worker.step();
        EXPECT_EQ(output.enabled(), now < 1000);
    }
    EXPECT_TRUE(telemetry.value.fault);
}

} // namespace
