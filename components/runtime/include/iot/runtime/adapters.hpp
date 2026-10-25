#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "iot/core/ports.hpp"
#include "iot/runtime/constants.hpp"

#include <cstdlib>
#include <type_traits>

namespace iot {

class EspClock final : public Clock {
    public:
    Milliseconds now() const noexcept override;
};

// Latest-value transport only. Commands require a bounded FIFO with rejection semantics.
template <typename T>
class FreeRtosSnapshot final : public SnapshotReader<T>, public SnapshotWriter<T> {
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

class LogTelemetry final : public TelemetrySink {
    public:
    void report(const Telemetry& value) noexcept override;
};

struct TaskOptions {
    const char* name;
    std::uint32_t stack_bytes;
    unsigned priority;
    Milliseconds period_ms;
};

// Borrows a worker. Both objects must outlive the running FreeRTOS task.
class PeriodicTask {
    public:
    PeriodicTask(Work& work, TaskOptions options) : work_(work), options_(options) {
    }

    PeriodicTask(const PeriodicTask&) = delete;
    PeriodicTask& operator=(const PeriodicTask&) = delete;
    bool start() noexcept;

    private:
    static void entry(void* context);
    Work& work_;
    TaskOptions options_;
    TaskHandle_t handle_{nullptr};
    TickType_t period_ticks_{0};
};

} // namespace iot
