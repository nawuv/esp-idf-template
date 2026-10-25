#include "iot/actuators/simulated_output.hpp"
#include "iot/application/workers.hpp"
#include "iot/sensors/synthetic_sensor.hpp"

#include <gtest/gtest.h>
#include <vector>

namespace {

class FakeClock final : public iot::Clock {
    public:
    iot::Milliseconds time{0};

    iot::Milliseconds now() const noexcept override {
        return time;
    }
};

template <typename T>
class MemorySnapshot final : public iot::SnapshotReader<T>, public iot::SnapshotWriter<T> {
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

class AlternateSensor final : public iot::SensorSource {
    public:
    iot::Reading read(iot::Milliseconds now) noexcept override {
        return {17.5F, now, true};
    }
};

class Requests final : public iot::RequestSource {
    public:
    bool enabled{false};

    bool enable_requested(iot::Milliseconds) noexcept override {
        return enabled;
    }
};

class FailingOutput final : public iot::OutputSink {
    public:
    bool fail_stop{false};
    std::vector<bool> calls;

    iot::OutputError set_enabled(bool enabled) noexcept override {
        calls.push_back(enabled);
        return enabled || fail_stop ? iot::OutputError::hardware_fault : iot::OutputError::none;
    }
};

class CaptureTelemetry final : public iot::TelemetrySink {
    public:
    unsigned calls{0};
    iot::Telemetry value{};

    void report(const iot::Telemetry& telemetry) noexcept override {
        ++calls;
        value = telemetry;
    }
};

TEST(SensorWorker, UsesInjectedSensorAndClock) {
    FakeClock clock;
    clock.time = 321;
    AlternateSensor sensor;
    MemorySnapshot<iot::Reading> readings;
    iot::SensorWorker worker(sensor, clock, readings);
    worker.step();
    ASSERT_TRUE(readings.present);
    EXPECT_FLOAT_EQ(readings.value.value, 17.5F);
    EXPECT_EQ(readings.value.sampled_at, 321);
}

TEST(SensorWorker, SensorImplementationCanBeReplacedWithoutChangingWorker) {
    FakeClock clock;
    iot::SyntheticSensor sensor(12.0F);
    MemorySnapshot<iot::Reading> readings;
    iot::SensorWorker worker(sensor, clock, readings);
    worker.step();
    EXPECT_FLOAT_EQ(readings.value.value, 12.0F);
}

TEST(ControlWorker, MissingReadingFailsClosed) {
    FakeClock clock;
    MemorySnapshot<iot::Reading> readings;
    MemorySnapshot<iot::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    iot::SimulatedOutput output;
    iot::TimedOutput policy({1000, 200});
    iot::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    worker.step();
    EXPECT_FALSE(output.enabled());
    EXPECT_TRUE(policy.fault());
    EXPECT_EQ(telemetry.value.applied_output, iot::AppliedOutput::off);
}

TEST(ControlWorker, RunsLocallyAndStopsWhenSampleExpires) {
    FakeClock clock;
    MemorySnapshot<iot::Reading> readings;
    readings.publish({42.0F, 0, true});
    MemorySnapshot<iot::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    iot::SimulatedOutput output;
    iot::TimedOutput policy({1000, 200});
    iot::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    worker.step();
    ASSERT_TRUE(output.enabled());
    clock.time = 201;
    worker.step();
    EXPECT_FALSE(output.enabled());
    EXPECT_TRUE(telemetry.value.fault);
}

TEST(ControlWorker, OutputFailureLatchesFaultAndAttemptsStop) {
    FakeClock clock;
    MemorySnapshot<iot::Reading> readings;
    readings.publish({42.0F, 0, true});
    MemorySnapshot<iot::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    FailingOutput output;
    iot::TimedOutput policy({1000, 200});
    iot::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    worker.step();
    EXPECT_EQ(output.calls, (std::vector<bool>{true, false}));
    EXPECT_TRUE(policy.fault());
    EXPECT_EQ(telemetry.value.output_error, iot::OutputError::hardware_fault);
    EXPECT_EQ(telemetry.value.applied_output, iot::AppliedOutput::off);
    worker.step();
    EXPECT_FALSE(output.calls.back());
    EXPECT_TRUE(telemetry.value.fault);
}

TEST(ControlWorker, FailedStopIsReportedAsUnknown) {
    FakeClock clock;
    MemorySnapshot<iot::Reading> readings;
    readings.publish({42.0F, 0, true});
    MemorySnapshot<iot::Telemetry> telemetry;
    Requests requests;
    requests.enabled = true;
    FailingOutput output;
    output.fail_stop = true;
    iot::TimedOutput policy({1000, 200});
    iot::ControlWorker worker(policy, clock, readings, requests, output, telemetry);
    worker.step();
    EXPECT_TRUE(policy.fault());
    EXPECT_EQ(telemetry.value.applied_output, iot::AppliedOutput::unknown);
}

TEST(ReporterWorker, ReportsLatestSnapshotAndSkipsMissingData) {
    MemorySnapshot<iot::Telemetry> telemetry;
    CaptureTelemetry sink;
    iot::ReporterWorker worker(telemetry, sink);
    worker.step();
    EXPECT_EQ(sink.calls, 0U);
    telemetry.publish({123, false, iot::AppliedOutput::off, false, iot::OutputError::none});
    worker.step();
    EXPECT_EQ(sink.calls, 1U);
    EXPECT_EQ(sink.value.uptime, 123);
}

TEST(TimedOutput, StateTransitionsAreExplicit) {
    iot::TimedOutput policy({1000, 200});
    EXPECT_EQ(policy.state(), iot::OutputState::off);
    ASSERT_TRUE(policy.update(true, {42, 0, true}, 0));
    EXPECT_EQ(policy.state(), iot::OutputState::running);
    policy.trip_fault();
    EXPECT_EQ(policy.state(), iot::OutputState::fault);
    EXPECT_FALSE(policy.update(true, {42, 1, true}, 1));
    ASSERT_TRUE(policy.reset_fault());
    EXPECT_EQ(policy.state(), iot::OutputState::off);
}

} // namespace
