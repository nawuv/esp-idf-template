#include "espkit/application/workers.hpp"
#include "espkit/drivers/simulated_output.hpp"
#include "espkit/drivers/synthetic_sensor.hpp"

#include <gtest/gtest.h>
#include <vector>

namespace {

class FakeClock final : public espkit::application::Clock {
    public:
    espkit::domain::Milliseconds time{0};

    espkit::domain::Milliseconds now() const noexcept override {
        return time;
    }
};

template <typename T>
class MemorySnapshot final : public espkit::application::SnapshotReader<T>,
                             public espkit::application::SnapshotWriter<T> {
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

class AlternateSensor final : public espkit::application::SensorSource {
    public:
    espkit::domain::Reading read(espkit::domain::Milliseconds now) noexcept override {
        return {17.5F, now, true};
    }
};

class Requests final : public espkit::application::RequestSource {
    public:
    bool enabled{false};

    bool enable_requested(espkit::domain::Milliseconds) noexcept override {
        return enabled;
    }
};

class FailingOutput final : public espkit::application::OutputSink {
    public:
    bool fail_stop{false};
    std::vector<bool> calls;

    espkit::domain::OutputError set_enabled(bool enabled) noexcept override {
        calls.push_back(enabled);
        return enabled || fail_stop ? espkit::domain::OutputError::hardware_fault
                                    : espkit::domain::OutputError::none;
    }
};

class CaptureTelemetry final : public espkit::application::TelemetrySink {
    public:
    unsigned calls{0};
    espkit::domain::Telemetry value{};

    void report(const espkit::domain::Telemetry& telemetry) noexcept override {
        ++calls;
        value = telemetry;
    }
};

TEST(SensorWorker, UsesInjectedSensorAndClock) {
    FakeClock clock;
    clock.time = 321;
    AlternateSensor sensor;
    MemorySnapshot<espkit::domain::Reading> readings;
    espkit::application::SensorWorker worker(sensor, clock, readings);
    worker.step();
    ASSERT_TRUE(readings.present);
    EXPECT_FLOAT_EQ(readings.value.value, 17.5F);
    EXPECT_EQ(readings.value.sampled_at, 321);
}

TEST(SensorWorker, SensorImplementationCanBeReplacedWithoutChangingWorker) {
    FakeClock clock;
    espkit::drivers::SyntheticSensor sensor(12.0F);
    MemorySnapshot<espkit::domain::Reading> readings;
    espkit::application::SensorWorker worker(sensor, clock, readings);
    worker.step();
    EXPECT_FLOAT_EQ(readings.value.value, 12.0F);
}

TEST(ControlWorker, MissingReadingFailsClosed) {
    FakeClock clock;
    MemorySnapshot<espkit::domain::Reading> readings;
    MemorySnapshot<espkit::domain::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    espkit::drivers::SimulatedOutput output;
    espkit::domain::TimedOutput policy({1000, 200});
    espkit::application::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    worker.step();
    EXPECT_FALSE(output.enabled());
    EXPECT_TRUE(policy.fault());
    EXPECT_EQ(telemetry.value.applied_output, espkit::domain::AppliedOutput::off);
}

TEST(ControlWorker, RunsLocallyAndStopsWhenSampleExpires) {
    FakeClock clock;
    MemorySnapshot<espkit::domain::Reading> readings;
    readings.publish({42.0F, 0, true});
    MemorySnapshot<espkit::domain::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    espkit::drivers::SimulatedOutput output;
    espkit::domain::TimedOutput policy({1000, 200});
    espkit::application::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    worker.step();
    ASSERT_TRUE(output.enabled());
    clock.time = 201;
    worker.step();
    EXPECT_FALSE(output.enabled());
    EXPECT_TRUE(telemetry.value.fault);
}

TEST(ControlWorker, OutputFailureLatchesFaultAndAttemptsStop) {
    FakeClock clock;
    MemorySnapshot<espkit::domain::Reading> readings;
    readings.publish({42.0F, 0, true});
    MemorySnapshot<espkit::domain::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    FailingOutput output;
    espkit::domain::TimedOutput policy({1000, 200});
    espkit::application::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    worker.step();
    EXPECT_EQ(output.calls, (std::vector<bool>{true, false}));
    EXPECT_TRUE(policy.fault());
    EXPECT_EQ(telemetry.value.output_error, espkit::domain::OutputError::hardware_fault);
    EXPECT_EQ(telemetry.value.applied_output, espkit::domain::AppliedOutput::off);
    worker.step();
    EXPECT_FALSE(output.calls.back());
    EXPECT_TRUE(telemetry.value.fault);
}

TEST(ControlWorker, FailedStopIsReportedAsUnknown) {
    FakeClock clock;
    MemorySnapshot<espkit::domain::Reading> readings;
    readings.publish({42.0F, 0, true});
    MemorySnapshot<espkit::domain::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    FailingOutput output;
    output.fail_stop = true;
    espkit::domain::TimedOutput policy({1000, 200});
    espkit::application::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    worker.step();
    EXPECT_TRUE(policy.fault());
    EXPECT_EQ(telemetry.value.applied_output, espkit::domain::AppliedOutput::unknown);
}

TEST(ReporterWorker, ReportsLatestSnapshotAndSkipsMissingData) {
    MemorySnapshot<espkit::domain::Telemetry> telemetry;
    CaptureTelemetry sink;
    espkit::application::ReporterWorker worker(telemetry, sink);
    worker.step();
    EXPECT_EQ(sink.calls, 0U);
    telemetry.publish(
        {123, false, espkit::domain::AppliedOutput::off, false, espkit::domain::OutputError::none});
    worker.step();
    EXPECT_EQ(sink.calls, 1U);
    EXPECT_EQ(sink.value.uptime, 123);
}

TEST(TimedOutput, StateTransitionsAreExplicit) {
    espkit::domain::TimedOutput policy({1000, 200});
    EXPECT_EQ(policy.state(), espkit::domain::OutputState::off);
    ASSERT_TRUE(policy.update(true, {42, 0, true}, 0));
    EXPECT_EQ(policy.state(), espkit::domain::OutputState::running);
    policy.trip_fault();
    EXPECT_EQ(policy.state(), espkit::domain::OutputState::fault);
    EXPECT_FALSE(policy.update(true, {42, 1, true}, 1));
    ASSERT_TRUE(policy.reset_fault());
    EXPECT_EQ(policy.state(), espkit::domain::OutputState::off);
}

TEST(ControlWorker, EnforcesRuntimeEvenWhenReporterIsNeverScheduled) {
    FakeClock clock;
    MemorySnapshot<espkit::domain::Reading> readings;
    MemorySnapshot<espkit::domain::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    espkit::drivers::SimulatedOutput output;
    espkit::domain::TimedOutput policy({1000, 200});
    espkit::application::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    for (espkit::domain::Milliseconds now = 0; now <= 1000; now += 100) {
        clock.time = now;
        readings.publish({42.0F, now, true});
        worker.step();
        EXPECT_EQ(output.enabled(), now < 1000);
    }
    EXPECT_TRUE(telemetry.value.fault);
}

} // namespace
