#ifndef TELEMETRY_PARAMS_H
#define TELEMETRY_PARAMS_H

#include <math.h>

/* Vibration: v(t) = A·sin(ω·t + phase) + N(0,σ_v²) */
#define VIB_A       2.5f
#define VIB_OMEGA   (2.0 * M_PI * 50.0)   /* 50 Hz spindle fundamental */
#define VIB_SIGMA   0.1f

/* Temperature: T(t) = T_AMB + ΔT·(1 − e^{−t/τ}) + N(0,σ_T²) */
#define TEMP_AMBIENT  22.0f
#define TEMP_DELTA    45.0f
#define TEMP_TAU      120.0f   /* s thermal time constant */
#define TEMP_SIGMA    0.05f

/* Spindle speed */
#define RPM_BASE   3000.0f
#define RPM_SIGMA  5.0f

/* Torque: TORQUE_BASE·(1 + 0.1·sin(ω_t·t)) + N(0,σ_T²) */
#define TORQUE_BASE   12.0f
#define TORQUE_SIGMA  0.2f
#define TORQUE_OMEGA  (2.0 * M_PI * 2.0)   /* 2 Hz variation */

/* Axis position: AXIS_RANGE·sin(2π·AXIS_FREQ·t + phase) */
#define AXIS_RANGE  50.0f
#define AXIS_FREQ   0.1        /* Hz sweep */

/* Server */
#define OPC_UA_PORT        4840
#define UPDATE_FREQ_HZ     1000
#define MAX_SECURE_CHANNELS 100
#define MAX_SESSIONS        50

#endif /* TELEMETRY_PARAMS_H */
