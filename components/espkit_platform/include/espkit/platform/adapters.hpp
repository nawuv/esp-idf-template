#pragma once

#include "espkit/application/ports/device.hpp"
#include "espkit/platform/constants.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <cstdlib>
#include <type_traits>

namespace espkit::platform {

class EspClock final : public espkit::application::Clock {
    public:
    espkit::domain::Milliseconds now() const noexcept override;
};

// Latest-value transport only. Commands require a bounded FIFO with rejection semantics.
template <typename T>
class FreeRtosSnapshot final : public espkit::application::SnapshotReader<T>,
                               public espkit::application::SnapshotWriter<T> {
    static_assert(std::is_trivially_copyable_v<T>);

    public:
    FreeRtosSnapshot()
        : queue_(xQueueCreate(runtime_constants::latest_snapshot_capacity, sizeof(T))) {
    }

    ~FreeRtosSnapshot() override {
        if (queue_) {
            vQueueDelete(queue_);
        }
    }

    FreeRtosSnapshot(const FreeRtosSnapshot&) = delete;
    FreeRtosSnapshot& operator=(const FreeRtosSnapshot&) = delete;

    bool ready() const noexcept {
        return queue_ != nullptr;
    }

    bool latest(T& value) const noexcept override {
        return queue_ && xQueuePeek(queue_, &value, 0) == pdTRUE;
    }

    void publish(const T& value) noexcept override {
        if (!queue_ || xQueueOverwrite(queue_, &value) != pdPASS) {
            std::abort();
        }
    }

    private:
    QueueHandle_t queue_;
};

class LogTelemetry final : public espkit::application::TelemetrySink {
    public:
    // The label is borrowed and must outlive this adapter.
    explicit LogTelemetry(const char* output_mode) : output_mode_(output_mode) {
    }

    void report(const espkit::domain::Telemetry& value) noexcept override;

    private:
    const char* output_mode_;
};

struct TaskOptions {
    const char* name;
    std::uint32_t stack_bytes;
    unsigned priority;
    espkit::domain::Milliseconds period_ms;
};

// Borrows a worker. Both objects must outlive the running FreeRTOS task.
class PeriodicTask {
    public:
    PeriodicTask(espkit::application::Work& work, TaskOptions options)
        : work_(work), options_(options) {
    }

    PeriodicTask(const PeriodicTask&) = delete;
    PeriodicTask& operator=(const PeriodicTask&) = delete;
    bool start() noexcept;

    private:
    static void entry(void* context);
    espkit::application::Work& work_;
    TaskOptions options_;
    TaskHandle_t handle_{nullptr};
    TickType_t period_ticks_{0};
};

} // namespace espkit::platform
