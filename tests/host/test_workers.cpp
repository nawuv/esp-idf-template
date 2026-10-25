#include "espkit/actuators/simulated_output.hpp"
#include "espkit/application/workers.hpp"
#include "espkit/sensors/synthetic_sensor.hpp"

#include <gtest/gtest.h>
#include <vector>

namespace {

class FakeClock final : public espkit::Clock {
    public:
    espkit::Milliseconds time{0};

    espkit::Milliseconds now() const noexcept override {
        return time;
    }
};

template <typename T>
class MemorySnapshot final : public espkit::SnapshotReader<T>, public espkit::SnapshotWriter<T> {
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

class AlternateSensor final : public espkit::SensorSource {
    public:
    espkit::Reading read(espkit::Milliseconds now) noexcept override {
        return {17.5F, now, true};
    }
};

class Requests final : public espkit::RequestSource {
    public:
    bool enabled{false};

    bool enable_requested(espkit::Milliseconds) noexcept override {
        return enabled;
    }
};

class FailingOutput final : public espkit::OutputSink {
    public:
    bool fail_stop{false};
    std::vector<bool> calls;

    espkit::OutputError set_enabled(bool enabled) noexcept override {
        calls.push_back(enabled);
        return enabled || fail_stop ? espkit::OutputError::hardware_fault
                                    : espkit::OutputError::none;
    }
};

class CaptureTelemetry final : public espkit::TelemetrySink {
    public:
    unsigned calls{0};
    espkit::Telemetry value{};

    void report(const espkit::Telemetry& telemetry) noexcept override {
        ++calls;
        value = telemetry;
    }
};

TEST(SensorWorker, UsesInjectedSensorAndClock) {
    FakeClock clock;
    clock.time = 321;
    AlternateSensor sensor;
    MemorySnapshot<espkit::Reading> readings;
    espkit::SensorWorker worker(sensor, clock, readings);
    worker.step();
    ASSERT_TRUE(readings.present);
    EXPECT_FLOAT_EQ(readings.value.value, 17.5F);
    EXPECT_EQ(readings.value.sampled_at, 321);
}

TEST(SensorWorker, SensorImplementationCanBeReplacedWithoutChangingWorker) {
    FakeClock clock;
    espkit::SyntheticSensor sensor(12.0F);
    MemorySnapshot<espkit::Reading> readings;
    espkit::SensorWorker worker(sensor, clock, readings);
    worker.step();
    EXPECT_FLOAT_EQ(readings.value.value, 12.0F);
}

TEST(ControlWorker, MissingReadingFailsClosed) {
    FakeClock clock;
    MemorySnapshot<espkit::Reading> readings;
    MemorySnapshot<espkit::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    espkit::SimulatedOutput output;
    espkit::TimedOutput policy({1000, 200});
    espkit::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    worker.step();
    EXPECT_FALSE(output.enabled());
    EXPECT_TRUE(policy.fault());
    EXPECT_EQ(telemetry.value.applied_output, espkit::AppliedOutput::off);
}

TEST(ControlWorker, RunsLocallyAndStopsWhenSampleExpires) {
    FakeClock clock;
    MemorySnapshot<espkit::Reading> readings;
    readings.publish({42.0F, 0, true});
    MemorySnapshot<espkit::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    espkit::SimulatedOutput output;
    espkit::TimedOutput policy({1000, 200});
    espkit::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    worker.step();
    ASSERT_TRUE(output.enabled());
    clock.time = 201;
    worker.step();
    EXPECT_FALSE(output.enabled());
    EXPECT_TRUE(telemetry.value.fault);
}

TEST(ControlWorker, OutputFailureLatchesFaultAndAttemptsStop) {
    FakeClock clock;
    MemorySnapshot<espkit::Reading> readings;
    readings.publish({42.0F, 0, true});
    MemorySnapshot<espkit::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    FailingOutput output;
    espkit::TimedOutput policy({1000, 200});
    espkit::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    worker.step();
    EXPECT_EQ(output.calls, (std::vector<bool>{true, false}));
    EXPECT_TRUE(policy.fault());
    EXPECT_EQ(telemetry.value.output_error, espkit::OutputError::hardware_fault);
    EXPECT_EQ(telemetry.value.applied_output, espkit::AppliedOutput::off);
    worker.step();
    EXPECT_FALSE(output.calls.back());
    EXPECT_TRUE(telemetry.value.fault);
}

TEST(ControlWorker, FailedStopIsReportedAsUnknown) {
    FakeClock clock;
    MemorySnapshot<espkit::Reading> readings;
    readings.publish({42.0F, 0, true});
    MemorySnapshot<espkit::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    FailingOutput output;
    output.fail_stop = true;
    espkit::TimedOutput policy({1000, 200});
    espkit::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    worker.step();
    EXPECT_TRUE(policy.fault());
    EXPECT_EQ(telemetry.value.applied_output, espkit::AppliedOutput::unknown);
}

TEST(ReporterWorker, ReportsLatestSnapshotAndSkipsMissingData) {
    MemorySnapshot<espkit::Telemetry> telemetry;
    CaptureTelemetry sink;
    espkit::ReporterWorker worker(telemetry, sink);
    worker.step();
    EXPECT_EQ(sink.calls, 0U);
    telemetry.publish({123, false, espkit::AppliedOutput::off, false, espkit::OutputError::none});
    worker.step();
    EXPECT_EQ(sink.calls, 1U);
    EXPECT_EQ(sink.value.uptime, 123);
}

TEST(TimedOutput, StateTransitionsAreExplicit) {
    espkit::TimedOutput policy({1000, 200});
    EXPECT_EQ(policy.state(), espkit::OutputState::off);
    ASSERT_TRUE(policy.update(true, {42, 0, true}, 0));
    EXPECT_EQ(policy.state(), espkit::OutputState::running);
    policy.trip_fault();
    EXPECT_EQ(policy.state(), espkit::OutputState::fault);
    EXPECT_FALSE(policy.update(true, {42, 1, true}, 1));
    ASSERT_TRUE(policy.reset_fault());
    EXPECT_EQ(policy.state(), espkit::OutputState::off);
}

} // namespace
