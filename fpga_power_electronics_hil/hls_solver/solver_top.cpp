// FPGA top-level HLS kernel: one trapezoidal solver step per AXI4-Stream transaction.
//
// Interfaces (Vitis HLS):
//   gate_in   — AXI4-Stream slave: receives 6-bit gate state (axis_gate_t)
//   state_out — AXI4-Stream master: emits 9×ap_fixed state vector (axis_state_t each)
//   params    — AXI4-Lite slave: FixedParams (written once at init)
//   return    — AXI4-Lite (implicit)
//
// Dataflow: gate_in → norton_update → lu_solve → update_history → state_out
// Latency target: < 2 µs (400 clock cycles at 200 MHz).
#include "include/hls_compat.h"
#include "include/solver_types.h"
#include <cstring>

static constexpr int N = 9;

// Declarations from companion translation units
void norton_update(
    const ap_fixed_32_16 state_in[N],
    const ap_fixed_32_16 hist[9],
    const ap_uint_6       gate_state,
    const ap_fixed_32_16  t,
    const FixedParams&    p,
    ap_fixed_32_16        I_N_out[N]);

void update_history(
    const ap_fixed_32_16 V_new[N],
    const ap_fixed_32_16 hist_in[9],
    const FixedParams&   p,
    ap_fixed_32_16       hist_out[9],
    ap_fixed_32_16       branch_currents[9]);

void lu_solve(
    const ap_fixed_32_16 L_bram[N][N],
    const ap_fixed_32_16 U_bram[N][N],
    const int            piv[N],
    const ap_fixed_32_16 I_N[N],
    ap_fixed_32_16       V_new_out[N]);

// Static BRAM storage for pre-factored LU matrices and solver state.
// Initialised once by the CPU host via AXI4-Lite writes before simulation starts.
static ap_fixed_32_16 s_L[N][N];
static ap_fixed_32_16 s_U[N][N];
static int            s_piv[N];
static ap_fixed_32_16 s_V[N];        // current nodal voltages
static ap_fixed_32_16 s_hist[9];     // Norton history currents
static ap_fixed_32_16 s_t;           // current simulation time

#ifdef __SYNTHESIS__
// AXI4-Lite register interface — maps FixedParams fields to consecutive 32-bit registers.
// Host uses mmap() via /dev/xdma0_user to write these before starting the RT loop.
// param_regs[0]  = dt,   [1]=G_Lf,  [2]=beta_Lf, [3]=G_Cf,
// param_regs[4]  = G_Lgrid, [5]=beta_Lgrid,
// param_regs[6]  = G_on, [7]=G_off, [8]=G_vs, [9]=V_dc, [10]=V_rated
// param_regs[16..24] = piv[0..8]
// param_regs[32..32+81] = L matrix (row-major), then U matrix
// param_regs[256] = reconfig_flag (write 1 to signal new LU available)
#endif

// Solver top-level — one call = one 50 µs simulation step.
// On host compile: called directly from test harness with software L/U matrices.
void solver_top(
    const axis_gate_t&   gate_in,
    axis_state_t         state_out[N],
    const FixedParams&   params)
{
    HLS_PRAGMA(HLS DATAFLOW)

    ap_fixed_32_16 I_N[N];
    HLS_PRAGMA(HLS ARRAY_PARTITION variable=I_N complete)

    norton_update(s_V, s_hist, gate_in.data, s_t, params, I_N);

    ap_fixed_32_16 V_new[N];
    HLS_PRAGMA(HLS ARRAY_PARTITION variable=V_new complete)

    lu_solve(s_L, s_U, s_piv, I_N, V_new);

    ap_fixed_32_16 hist_new[9];
    ap_fixed_32_16 branch_currents[9];
    update_history(V_new, s_hist, params, hist_new, branch_currents);

    // Update persistent state
    for (int i = 0; i < N; ++i) s_V[i] = V_new[i];
    for (int i = 0; i < 9; ++i) s_hist[i] = hist_new[i];
    s_t = s_t + params.dt;

    // Emit state vector on AXI4-Stream
    for (int i = 0; i < N; ++i) {
        state_out[i].data = V_new[i];
        state_out[i].last = (i == N - 1) ? 1 : 0;
    }
}
