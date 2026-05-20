#include <gtest/gtest.h>
#include "MovingAverageFilter.h"

TEST(MovingAverageFilter, ConvergesToMean) {
    MovingAverageFilter f(5);
    double result = 0.0;
    for (int i = 0; i < 10; ++i) result = f.apply(50.0);
    EXPECT_NEAR(result, 50.0, 0.1);
}

TEST(MovingAverageFilter, WindowOfOne) {
    MovingAverageFilter f(1);
    EXPECT_DOUBLE_EQ(f.apply(7.0), 7.0);
    EXPECT_DOUBLE_EQ(f.apply(3.0), 3.0);
}

TEST(MovingAverageFilter, ResetClearsState) {
    MovingAverageFilter f(3);
    f.apply(100.0); f.apply(100.0); f.apply(100.0);
    f.reset();
    EXPECT_NEAR(f.apply(10.0), 10.0, 0.1);
}

#include "ThresholdDetector.h"
#include "DataProcessor.h"
#include "TelemetryFrame.h"
#include <chrono>

TEST(ThresholdDetector, KConsecutiveTriggersAlarm) {
    ThresholdDetector d(80.0, 20.0, 3);
    d.apply(90.0); EXPECT_FALSE(d.alarm());
    d.apply(90.0); EXPECT_FALSE(d.alarm());
    d.apply(90.0); EXPECT_TRUE(d.alarm());
}

TEST(ThresholdDetector, SingleSpikeDoesNotLatch) {
    ThresholdDetector d(80.0, 20.0, 3);
    d.apply(90.0);
    d.apply(50.0);  // resets consecutive count
    d.apply(90.0);
    EXPECT_FALSE(d.alarm());
}

TEST(DataProcessor, AlarmFlagSetAfterKConsecutive) {
    DataProcessor proc(5, 80.0, 20.0, 3);
    auto now = std::chrono::steady_clock::now();
    ProcessedMetric m;
    for (int i = 0; i < 3; ++i) m = proc.process({"s1", 90.0, now});
    EXPECT_TRUE(m.alarm_flag);
}

TEST(DataProcessor, FilteredValueConverges) {
    DataProcessor proc(5, 80.0, 20.0, 3);
    auto now = std::chrono::steady_clock::now();
    ProcessedMetric m;
    for (int i = 0; i < 5; ++i) m = proc.process({"s1", 60.0, now});
    EXPECT_NEAR(m.filtered_value, 60.0, 0.1);
    EXPECT_EQ(m.sensor_id, "s1");
}
