#pragma once
#include "TelemetryFrame.h"
#include <array>
#include <atomic>
#include <cstdint>

class TelemetryReceiver {
public:
    static constexpr size_t CAPACITY = 1024;

    bool push(TelemetryFrame frame);
    bool pop(TelemetryFrame& out);
    uint64_t dropped() const { return dropped_.load(std::memory_order_relaxed); }

private:
    std::array<TelemetryFrame, CAPACITY> buf_{};
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
    std::atomic<uint64_t> dropped_{0};
};
