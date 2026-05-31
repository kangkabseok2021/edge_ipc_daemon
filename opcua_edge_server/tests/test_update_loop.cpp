#include <gtest/gtest.h>
#include <thread>
#include <chrono>

extern "C" {
#include "update_loop.h"
#include "data_source.h"
}

#ifdef __linux__

TEST(UpdateLoop, StartsAndStopsCleanly) {
    EXPECT_EQ(update_loop_start(), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_NO_THROW(update_loop_stop());
}

TEST(UpdateLoop, TickCounterIncrements) {
    ASSERT_EQ(update_loop_start(), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    uint64_t ticks = update_loop_tick_count();
    update_loop_stop();
    /* At 1 kHz for 100 ms expect ≥ 50 ticks (50% threshold for CI jitter) */
    EXPECT_GE(ticks, static_cast<uint64_t>(50));
}

TEST(UpdateLoop, NodeValuesUpdatedAfterStart) {
    /* Reset all values to a sentinel before starting */
    for (int i = 0; i < NUM_NODES; i++)
        data_source_set(static_cast<CncNodeIdx>(i), 999.0f);

    ASSERT_EQ(update_loop_start(), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    update_loop_stop();

    /* Temperature starts near TEMP_AMBIENT (not 999) once the loop runs */
    float temp = data_source_get(IDX_SPINDLE_TEMP);
    EXPECT_NE(temp, 999.0f);
}

#else

TEST(UpdateLoop, SkippedOnNonLinux) {
    GTEST_SKIP() << "timerfd not available on this platform";
}

#endif /* __linux__ */
