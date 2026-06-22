// FPGA kernel: Norton injection vector update (O(N) per timestep).
// Called by solver_top via HLS DATAFLOW before lu_solve.
//
// Compiled with Vitis HLS 2023.2:
//   #pragma HLS INLINE          -- inlined into solver_top dataflow region
//   #pragma HLS ARRAY_PARTITION -- exposes parallel register access
//
// On host: compiled as plain C++ (hls_compat.h maps types; HLS_PRAGMA is noop).
#include "include/hls_compat.h"
#include "include/solver_types.h"
#include <cmath>

static constexpr int N = 9;
static constexpr double TWO_PI = 6.283185307179586;

// Update Norton injection vector I_N[0..8] from history currents and gate state.
// state_in[0..8]: previous-step nodal voltages (V_inv×3, V_cap×3, V_grid×3)
// hist: history currents packed as [I_Lf×3, I_Cf×3, I_Lgrid×3]
// gate_state: 6-bit IGBT pattern
// t: current simulation time [s]
// I_N_out: output injection vector
void norton_update(
    const ap_fixed_32_16 state_in[N],
    const ap_fixed_32_16 hist[9],      // [I_Lf_0..2, I_Cf_0..2, I_Lgrid_0..2]
    const ap_uint_6       gate_state,
    const ap_fixed_32_16  t,
    const FixedParams&    p,
    ap_fixed_32_16        I_N_out[N])
{
    HLS_PRAGMA(HLS INLINE)
    HLS_PRAGMA(HLS ARRAY_PARTITION variable=state_in complete)
    HLS_PRAGMA(HLS ARRAY_PARTITION variable=I_N_out  complete)
    HLS_PRAGMA(HLS ARRAY_PARTITION variable=hist      complete)

    for (int i = 0; i < N; ++i) I_N_out[i] = ap_fixed_32_16(0);

    // V_grid_peak = V_grid_rms * sqrt(2); carried at standard 230 V rms.
    const ap_fixed_32_16 V_grid_peak = ap_fixed_32_16(230.0 * 1.41421356237);

    for (int k = 0; k < 3; ++k) {
        HLS_PRAGMA(HLS UNROLL)
        // Filter inductor Norton: flows inv_k → cap_k
        I_N_out[k]     = I_N_out[k]     - hist[k];       // -I_Lf_hist at inv
        I_N_out[k + 3] = I_N_out[k + 3] + hist[k];       // +I_Lf_hist at cap

        // Filter capacitor Norton: companion current flows cap_k→ground (leaves node)
        I_N_out[k + 3] = I_N_out[k + 3] - hist[k + 3];   // -I_Cf_hist

        // Grid inductor Norton: flows cap_k → grid_k
        I_N_out[k + 3] = I_N_out[k + 3] - hist[k + 6];   // -I_Lgrid_hist at cap
        I_N_out[k + 6] = I_N_out[k + 6] + hist[k + 6];   // +I_Lgrid_hist at grid

        // Grid source Norton injection at grid_k
        ap_fixed_32_16 phase_off = ap_fixed_32_16(k * (-TWO_PI / 3.0));
        // On FPGA, sin() is implemented via CORDIC (HLS math.h); on host uses std::sin
        ap_fixed_32_16 v_src = V_grid_peak
            * ap_fixed_32_16(std::sin(static_cast<double>(TWO_PI * 50.0 * t + phase_off)));
        I_N_out[k + 6] = I_N_out[k + 6] + p.G_vs * v_src;

        // Upper IGBT injects current from DC bus into inv_k
        // Snubbers (τ≈1ns≪dt=50µs) are in quasi-static limit C→0 → omitted
        bool T1 = (static_cast<uint8_t>(gate_state) >> (2 * k)) & 1;
        ap_fixed_32_16 G_T1 = T1 ? p.G_on : p.G_off;
        I_N_out[k] = I_N_out[k] + G_T1 * p.V_dc;
    }
}

// Recompute branch currents and history for next step.
// V_new: solved nodal voltages from lu_solve
// hist_out[0..8]: [I_Lf×3, I_Cf×3, I_Lgrid×3] for next step
void update_history(
    const ap_fixed_32_16 V_new[N],
    const ap_fixed_32_16 hist_in[9],
    const FixedParams&   p,
    ap_fixed_32_16       hist_out[9],
    ap_fixed_32_16       branch_currents[9])  // [i_Lf×3, i_Cf×3, i_Lgrid×3]
{
    HLS_PRAGMA(HLS INLINE)
    HLS_PRAGMA(HLS ARRAY_PARTITION variable=V_new    complete)
    HLS_PRAGMA(HLS ARRAY_PARTITION variable=hist_out complete)

    for (int k = 0; k < 3; ++k) {
        HLS_PRAGMA(HLS UNROLL)
        // Filter inductor
        ap_fixed_32_16 v_Lf = V_new[k] - V_new[k + 3];
        ap_fixed_32_16 i_Lf = p.G_Lf * v_Lf + hist_in[k];
        hist_out[k]          = p.G_Lf * v_Lf + p.beta_Lf * i_Lf;
        branch_currents[k]   = i_Lf;

        // Filter capacitor
        ap_fixed_32_16 i_Cf    = p.G_Cf * V_new[k + 3] + hist_in[k + 3];
        hist_out[k + 3]        = -i_Cf - p.G_Cf * V_new[k + 3];
        branch_currents[k + 3] = i_Cf;

        // Grid inductor
        ap_fixed_32_16 v_Lg    = V_new[k + 3] - V_new[k + 6];
        ap_fixed_32_16 i_Lgrid = p.G_Lgrid * v_Lg + hist_in[k + 6];
        hist_out[k + 6]        = p.G_Lgrid * v_Lg + p.beta_Lgrid * i_Lgrid;
        branch_currents[k + 6] = i_Lgrid;
    }
}
