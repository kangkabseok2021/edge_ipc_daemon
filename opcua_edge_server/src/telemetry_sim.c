#include "telemetry_sim.h"
#include "telemetry_params.h"

#include <math.h>
#include <stdlib.h>

double box_muller(uint32_t *seed) {
    /* Avoids log(0) by keeping u1 in (0, 1] */
    double u1 = ((double)rand_r(seed) + 1.0) / ((double)RAND_MAX + 2.0);
    double u2 = (double)rand_r(seed)         / ((double)RAND_MAX + 1.0);
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

float telemetry_vibration(double t, int axis_idx, uint32_t *seed) {
    double phase  = axis_idx * (2.0 * M_PI / 3.0);   /* 120° offset per axis */
    double signal = VIB_A * sin(VIB_OMEGA * t + phase);
    double noise  = VIB_SIGMA * box_muller(seed);
    return (float)(signal + noise);
}

float telemetry_temperature(double t, uint32_t *seed) {
    double transient = TEMP_DELTA * (1.0 - exp(-(double)t / TEMP_TAU));
    double noise     = TEMP_SIGMA * box_muller(seed);
    return (float)(TEMP_AMBIENT + transient + noise);
}

float telemetry_spindle_speed(double t, uint32_t *seed) {
    (void)t;
    return (float)(RPM_BASE + RPM_SIGMA * box_muller(seed));
}

float telemetry_torque(double t, uint32_t *seed) {
    double variation = TORQUE_BASE * 0.1 * sin(TORQUE_OMEGA * t);
    double noise     = TORQUE_SIGMA * box_muller(seed);
    return (float)(TORQUE_BASE + variation + noise);
}

float telemetry_axis_position(double t, int axis_idx) {
    double phase = axis_idx * (M_PI / 6.0);   /* 30° offset per axis */
    return (float)(AXIS_RANGE * sin(2.0 * M_PI * AXIS_FREQ * t + phase));
}
