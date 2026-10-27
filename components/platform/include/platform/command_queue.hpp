#pragma once

#include "application/ports/command.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <cstddef>
#include <limits>
#include <type_traits>

namespace platform {

// Bounded command FIFO. A full queue rejects the new item; it does not overwrite.
// Do not call from an interrupt. Latest-value mailboxes remain FreeRtosSnapshot.
template <typename T> class FreeRtosFifo final : public application::BoundedFifo<T> {
    static_assert(std::is_trivially_copyable_v<T>);

    public:
    explicit FreeRtosFifo(std::size_t capacity) : capacity_(capacity) {
        if (capacity_ == 0 ||
            capacity_ > static_cast<std::size_t>(std::numeric_limits<UBaseType_t>::max())) {
            capacity_ = 0;
            return;
        }
        queue_ = xQueueCreate(static_cast<UBaseType_t>(capacity_), sizeof(T));
    }

    ~FreeRtosFifo() override {
        if (queue_) {
            vQueueDelete(queue_);
        }
    }

    FreeRtosFifo(const FreeRtosFifo&) = delete;
    FreeRtosFifo& operator=(const FreeRtosFifo&) = delete;

    bool ready() const noexcept {
        return queue_ != nullptr;
    }

    bool try_push(const T& value) noexcept override {
        return queue_ && xQueueSend(queue_, &value, 0) == pdPASS;
    }

    bool try_pop(T& value) noexcept override {
        return queue_ && xQueueReceive(queue_, &value, 0) == pdTRUE;
    }

    std::size_t size() const noexcept override {
        return queue_ ? static_cast<std::size_t>(uxQueueMessagesWaiting(queue_)) : 0;
    }

    std::size_t capacity() const noexcept override {
        return queue_ ? capacity_ : 0;
    }

    private:
    std::size_t capacity_;
    QueueHandle_t queue_{nullptr};
};

} // namespace platform
