#pragma once
#include "hls_compat.h"

// AXI4-Stream frame carrying the 6-bit IGBT gate state (one bit per IGBT).
// Bit layout: bit 2k = T1k (upper), bit 2k+1 = T2k (lower), k=0,1,2.
struct axis_gate_t {
    ap_uint_6  data;  // gate state
    uint8_t    last;  // AXI TLAST
};

// AXI4-Stream frame carrying one element of the 9-element state vector.
// The host sends 9 consecutive frames per solver step; TLAST on frame 8.
struct axis_state_t {
    ap_fixed_32_16 data;  // nodal voltage or branch current
    uint8_t        last;
};

// Solver parameters written via AXI4-Lite registers before simulation starts.
struct FixedParams {
    ap_fixed_32_16 dt;
    ap_fixed_32_16 G_Lf;       // = dt / (2*L_f + R_f*dt)
    ap_fixed_32_16 beta_Lf;    // = (2*L_f - R_f*dt) / (2*L_f + R_f*dt)
    ap_fixed_32_16 G_Cf;       // = 2*C_f / dt
    ap_fixed_32_16 G_Lgrid;    // = dt / (2*L_grid + R_grid*dt)
    ap_fixed_32_16 beta_Lgrid; // = (2*L_grid - R_grid*dt) / (2*L_grid + R_grid*dt)
    ap_fixed_32_16 G_on;       // = 1 / R_on
    ap_fixed_32_16 G_off;      // = 1 / R_off
    ap_fixed_32_16 G_vs;       // = 1 / R_src  (grid Norton conductance)
    ap_fixed_32_16 V_dc;
    ap_fixed_32_16 V_rated;
};
