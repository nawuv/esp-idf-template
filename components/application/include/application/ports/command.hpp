#pragma once

#include <cstddef>

namespace application {

// Bounded FIFO. try_push fails when full; it must not overwrite queued work.
// Callers are not ISRs. Cross-task use requires a thread-safe implementation.
template <typename T> class BoundedFifo {
    public:
    virtual ~BoundedFifo() = default;
    virtual bool try_push(const T& value) noexcept = 0;
    virtual bool try_pop(T& value) noexcept = 0;
    virtual std::size_t size() const noexcept = 0;
    virtual std::size_t capacity() const noexcept = 0;
};

} // namespace application
