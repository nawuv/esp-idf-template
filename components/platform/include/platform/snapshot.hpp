#pragma once

#include "application/ports/device.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "platform/constants.hpp"

#include <cstdlib>
#include <type_traits>

namespace platform {

// Latest-value transport only. Command FIFOs are FreeRtosFifo and reject when full.
template <typename T>
class FreeRtosSnapshot final : public application::SnapshotReader<T>,
                               public application::SnapshotWriter<T> {
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

} // namespace platform
