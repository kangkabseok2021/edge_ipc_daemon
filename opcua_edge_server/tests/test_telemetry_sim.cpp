#include <gtest/gtest.h>
#include <cmath>
#include <numeric>
#include <vector>

extern "C" {
#include "telemetry_sim.h"
#include "telemetry_params.h"
}

TEST(TelemetrySim, VibrationWithinPhysicalBounds) {
    uint32_t seed = 42;
    for (int i = 0; i < 1000; i++) {
        float v = telemetry_vibration(i * 0.001, 0, &seed);
        /* A·sin + N(0,σ²): P(|v| > A + 5σ) < 3e-7 */
        EXPECT_LT(v,  VIB_A + 5.0f * VIB_SIGMA);
        EXPECT_GT(v, -(VIB_A + 5.0f * VIB_SIGMA));
    }
}

TEST(TelemetrySim, TemperatureAtZeroIsNearAmbient) {
    uint32_t seed = 1;
    /* At t=0 transient = 0, so T ≈ T_AMBIENT ± 3σ */
    float T0 = telemetry_temperature(0.0, &seed);
    EXPECT_NEAR(T0, TEMP_AMBIENT, 3.0f * TEMP_SIGMA + 0.5f);
}

TEST(TelemetrySim, TemperatureConvergesAtFiveTimeConstants) {
    uint32_t seed = 7;
    /* At t=5τ, e^{-5} ≈ 0.007, so T ≈ T_AMB + 0.993·ΔT */
    float T_final = TEMP_AMBIENT + TEMP_DELTA;
    float T_5tau  = telemetry_temperature(5.0 * TEMP_TAU, &seed);
    EXPECT_NEAR(T_5tau, T_final, 3.0f * TEMP_SIGMA + 0.5f);
}

TEST(TelemetrySim, BoxMullerMeanNearZero) {
    uint32_t seed = 123;
    const int N = 10000;
    double sum = 0.0;
    for (int i = 0; i < N; i++) sum += box_muller(&seed);
    /* 3σ/√N = 3/100 = 0.03 — generous bound to avoid flaky CI */
    EXPECT_NEAR(sum / N, 0.0, 0.05);
}

TEST(TelemetrySim, BoxMullerStdDevNearOne) {
    uint32_t seed = 456;
    const int N = 10000;
    std::vector<double> s(N);
    for (auto &x : s) x = box_muller(&seed);
    double mean = std::accumulate(s.begin(), s.end(), 0.0) / N;
    double var  = 0.0;
    for (auto x : s) var += (x - mean) * (x - mean);
    var /= (N - 1);
    EXPECT_NEAR(std::sqrt(var), 1.0, 0.05);
}

TEST(TelemetrySim, SpindleSpeedNearBaseline) {
    uint32_t seed = 789;
    float rpm = telemetry_spindle_speed(0.0, &seed);
    EXPECT_NEAR(rpm, RPM_BASE, 5.0f * RPM_SIGMA);
}
