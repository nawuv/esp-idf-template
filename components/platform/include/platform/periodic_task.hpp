#pragma once

#include "application/ports/device.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace platform {

struct TaskOptions {
    const char* name;
    std::uint32_t stack_bytes;
    unsigned priority;
    domain::Milliseconds period_ms;
};

// Borrows a worker. Both objects must outlive the running FreeRTOS task.
class PeriodicTask {
    public:
    PeriodicTask(application::Work& work, TaskOptions options) : work_(work), options_(options) {
    }

    PeriodicTask(const PeriodicTask&) = delete;
    PeriodicTask& operator=(const PeriodicTask&) = delete;
    bool start() noexcept;

    private:
    static void entry(void* context);
    application::Work& work_;
    TaskOptions options_;
    TaskHandle_t handle_{nullptr};
    TickType_t period_ticks_{0};
};

} // namespace platform
