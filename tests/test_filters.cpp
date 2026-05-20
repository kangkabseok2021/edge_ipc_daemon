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
