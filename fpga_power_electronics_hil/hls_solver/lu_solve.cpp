// FPGA kernel: forward and back substitution on pre-factored 9×9 LU matrices.
// L and U are stored in BRAM (written once at init via AXI4-Lite).
// Each solver step is O(N²) = 81 MAC operations at II=1 → ~405 ns at 200 MHz.
//
// Synthesis target (Artix-7 XC7A100T): LUT<8000, FF<6000, DSP48<20, latency<40 µs.
#include "include/hls_compat.h"
#include "include/solver_types.h"

static constexpr int N = 9;

// Forward substitution: L*y = P*b (permutation already applied in b).
// L is unit lower triangular (diagonal = 1, implicit).
static void forward_sub(
    const ap_fixed_32_16 L[N][N],
    const ap_fixed_32_16 b[N],
    ap_fixed_32_16       y[N])
{
    HLS_PRAGMA(HLS INLINE)
    for (int i = 0; i < N; ++i) {
        HLS_PRAGMA(HLS PIPELINE II=1)
        ap_fixed_32_16 s = b[i];
        for (int j = 0; j < i; ++j)
            s = s - L[i][j] * y[j];
        y[i] = s;
    }
}

// Back substitution: U*x = y.
static void back_sub(
    const ap_fixed_32_16 U[N][N],
    const ap_fixed_32_16 y[N],
    ap_fixed_32_16       x[N])
{
    HLS_PRAGMA(HLS INLINE)
    for (int i = N - 1; i >= 0; --i) {
        HLS_PRAGMA(HLS PIPELINE II=1)
        ap_fixed_32_16 s = y[i];
        for (int j = i + 1; j < N; ++j)
            s = s - U[i][j] * x[j];
        x[i] = s / U[i][i];
    }
}

// Top-level LU solve: G_total * V_new = I_N
// L_bram, U_bram: pre-factored matrices written to BRAM at init (read-only at runtime).
// piv: row permutation (int array, written once at init).
// I_N: Norton injection vector from norton_update.
// V_new_out: solved nodal voltages (9 elements).
void lu_solve(
    const ap_fixed_32_16 L_bram[N][N],
    const ap_fixed_32_16 U_bram[N][N],
    const int            piv[N],
    const ap_fixed_32_16 I_N[N],
    ap_fixed_32_16       V_new_out[N])
{
    HLS_PRAGMA(HLS ARRAY_PARTITION variable=L_bram    cyclic factor=3 dim=2)
    HLS_PRAGMA(HLS ARRAY_PARTITION variable=U_bram    cyclic factor=3 dim=2)
    HLS_PRAGMA(HLS ARRAY_PARTITION variable=I_N       complete)
    HLS_PRAGMA(HLS ARRAY_PARTITION variable=V_new_out complete)

    // Apply row permutation to I_N
    ap_fixed_32_16 b_perm[N];
    HLS_PRAGMA(HLS ARRAY_PARTITION variable=b_perm complete)
    for (int i = 0; i < N; ++i)
        b_perm[i] = I_N[piv[i]];

    ap_fixed_32_16 y[N];
    HLS_PRAGMA(HLS ARRAY_PARTITION variable=y complete)

    forward_sub(L_bram, b_perm, y);
    back_sub(U_bram, y, V_new_out);
}
