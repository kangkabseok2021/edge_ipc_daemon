#include "TelemetryReceiver.h"

bool TelemetryReceiver::push(TelemetryFrame frame) {
    size_t tail = tail_.load(std::memory_order_relaxed);
    size_t next = (tail + 1) % CAPACITY;
    if (next == head_.load(std::memory_order_acquire)) {
        dropped_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    buf_[tail] = std::move(frame);
    tail_.store(next, std::memory_order_release);
    return true;
}

bool TelemetryReceiver::pop(TelemetryFrame& out) {
    size_t head = head_.load(std::memory_order_relaxed);
    if (head == tail_.load(std::memory_order_acquire)) return false;
    out = std::move(buf_[head]);
    head_.store((head + 1) % CAPACITY, std::memory_order_release);
    return true;
}
