#include <gtest/gtest.h>
#include "TelemetryReceiver.h"
#include <chrono>

TEST(SPSC, PushPopRoundtrip) {
    TelemetryReceiver rx;
    auto now = std::chrono::steady_clock::now();
    EXPECT_TRUE(rx.push({"s1", 42.0, now}));
    TelemetryFrame out;
    EXPECT_TRUE(rx.pop(out));
    EXPECT_EQ(out.sensor_id, "s1");
    EXPECT_DOUBLE_EQ(out.value, 42.0);
}

TEST(SPSC, OverflowIncrementsDropped) {
    TelemetryReceiver rx;
    auto now = std::chrono::steady_clock::now();
    TelemetryFrame f{"s1", 1.0, now};
    size_t pushed = 0;
    while (rx.push(f)) ++pushed;
    EXPECT_EQ(pushed, TelemetryReceiver::CAPACITY - 1);
    EXPECT_EQ(rx.dropped(), 1u);  // while loop's first failed push
    EXPECT_FALSE(rx.push(f));     // second overflow
    EXPECT_EQ(rx.dropped(), 2u);  // both counted
}
