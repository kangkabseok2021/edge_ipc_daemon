#include <gtest/gtest.h>
#include "SimulatedSource.h"
#include "FileSource.h"
#include <fstream>
#include <string>

TEST(SimulatedSource, SpikeRateWithinTolerance) {
    // Spike probability 0.20, magnitude 5×sigma → value ≈ mu+5σ = 100
    SimulatedSource src("test", 50.0, 10.0, 0.20, 5.0);
    int spikes = 0;
    const int total = 1000;
    for (int i = 0; i < total; ++i) {
        auto f = src.read();
        ASSERT_TRUE(f.has_value());
        if (f->value > 90.0) ++spikes;
    }
    double rate = static_cast<double>(spikes) / total;
    EXPECT_NEAR(rate, 0.20, 0.10);  // ±10%
}

TEST(FileSource, LoopbackProducesDeterministicReplay) {
    const std::string path = "/tmp/test_file_source.csv";
    { std::ofstream f(path); f << "10.0\n20.0\n30.0\n"; }

    FileSource src("s", path, /*loop=*/true);
    std::vector<double> vals;
    for (int i = 0; i < 6; ++i) {
        auto f = src.read();
        ASSERT_TRUE(f.has_value()) << "read() returned nullopt at index " << i;
        vals.push_back(f->value);
    }
    EXPECT_DOUBLE_EQ(vals[0], 10.0);
    EXPECT_DOUBLE_EQ(vals[1], 20.0);
    EXPECT_DOUBLE_EQ(vals[2], 30.0);
    EXPECT_DOUBLE_EQ(vals[3], 10.0);  // loop-back
    EXPECT_DOUBLE_EQ(vals[4], 20.0);
    EXPECT_DOUBLE_EQ(vals[5], 30.0);
}
