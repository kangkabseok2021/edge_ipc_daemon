#ifndef TELEMETRY_SIM_H
#define TELEMETRY_SIM_H

#include <stdint.h>

/* Box-Muller: one N(0,1) sample using rand_r (reentrant, no global state) */
double box_muller(uint32_t *seed);

/* Vibration v(t) = A·sin(ω·t + phase_offset(axis)) + N(0,σ_v²) — mm/s */
float telemetry_vibration(double t, int axis_idx, uint32_t *seed);

/* Temperature T(t) = T_AMB + ΔT·(1 − e^{−t/τ}) + N(0,σ_T²) — °C */
float telemetry_temperature(double t, uint32_t *seed);

/* Spindle speed: RPM_BASE + N(0,σ_rpm²) — RPM */
float telemetry_spindle_speed(double t, uint32_t *seed);

/* Torque: BASE·(1 + 0.1·sin(ω_t·t)) + N(0,σ_τ²) — N·m */
float telemetry_torque(double t, uint32_t *seed);

/* Axis position: RANGE·sin(2π·FREQ·t + phase_offset(axis)) — mm */
float telemetry_axis_position(double t, int axis_idx);

#endif /* TELEMETRY_SIM_H */
