#include "VscNetwork.h"
#include "LuSolver.h"
#include "NortonUpdate.h"
#include <cstring>
#include <cmath>

#ifndef M_PI
static constexpr double M_PI = 3.14159265358979323846;
#endif

namespace vsc {

// ─── helper: stamp symmetric conductance branch (nodes p ↔ m) ────────────────
static void stamp(double G[VscNetwork::N][VscNetwork::N], int p, int m, double g) {
    G[p][p] += g;
    if (m >= 0) {
        G[m][m] += g;
        G[p][m] -= g;
        G[m][p] -= g;
    }
}

VscNetwork::VscNetwork(const SolverParams& p) : p_(p) {
    gate_factored_ = 0xFF;
    std::memset(G_, 0, sizeof(G_));
    // Initial assembly with all gates off so G_ is ready for G_total_ptr()
    assemble_and_factor(0x00);
}

void VscNetwork::assemble_and_factor(uint8_t gate) {
    std::memset(G_, 0, sizeof(G_));

    // ── Filter inductors L_f (RL companion): nodes inv_k ↔ cap_k ────────────
    auto n_Lf = make_inductor(p_.L_f, p_.R_f, p_.dt);
    for (int k = 0; k < 3; ++k)
        stamp(G_, k, k + 3, n_Lf.G_eq);

    // ── Filter capacitors C_f: cap_k ↔ ground ────────────────────────────────
    auto n_Cf = make_capacitor(p_.C_f, p_.dt);
    for (int k = 3; k < 6; ++k)
        stamp(G_, k, -1, n_Cf.G_C);

    // ── Grid inductors L_grid (RL companion): cap_k ↔ grid_k ─────────────────
    auto n_Lgrid = make_inductor(p_.L_grid, p_.R_grid, p_.dt);
    for (int k = 0; k < 3; ++k)
        stamp(G_, k + 3, k + 6, n_Lgrid.G_eq);

    // ── Grid voltage source Norton equivalents (stiff): grid_k ↔ ground ──────
    double G_vs = 1.0 / p_.R_src;
    for (int k = 6; k < 9; ++k)
        stamp(G_, k, -1, G_vs);

    // ── IGBT conductances (state-dependent) ──────────────────────────────────
    // Snubbers (τ≈1ns≪dt=50µs) are in quasi-static limit C→0 → omitted
    // bit 2k   = T1k (upper), bit 2k+1 = T2k (lower), for k=0,1,2
    for (int k = 0; k < 3; ++k) {
        bool T1 = (gate >> (2 * k))     & 1;
        bool T2 = (gate >> (2 * k + 1)) & 1;
        double G_T1 = T1 ? p_.G_on : p_.G_off;
        double G_T2 = T2 ? p_.G_on : p_.G_off;
        // Upper T1k: between DC bus (treated as stiff Norton) and inv_k
        //   → only diagonal stamp on inv_k (DC bus not in nodal unknowns)
        stamp(G_, k, -1, G_T1 + G_T2);
    }

    // LU factorise (copy to LU_ first)
    std::memcpy(LU_, G_, sizeof(G_));
    lu_factor<N>(LU_, piv_);
    gate_factored_ = gate;
}

bool VscNetwork::step(State& s, uint8_t gate) {
    if (gate != gate_factored_)
        assemble_and_factor(gate);

    s.gate = gate;

    // ── Assemble Norton injection vector ─────────────────────────────────────
    double I_N[N]{};

    for (int k = 0; k < 3; ++k) {
        // Inductor Norton: current source flows inv→cap, so -I_hist at inv, +I_hist at cap
        I_N[k]     -= s.I_Lf_hist[k];
        I_N[k + 3] += s.I_Lf_hist[k];

        // Capacitor Norton: companion current flows cap_k→ground (leaves node)
        I_N[k + 3] -= s.I_Cf_hist[k];

        // Grid inductor Norton: current source flows cap→grid
        I_N[k + 3] -= s.I_Lgrid_hist[k];
        I_N[k + 6] += s.I_Lgrid_hist[k];

        // Grid voltage source Norton injection at grid_k
        double phase_offset = static_cast<double>(k) * (-2.0 * M_PI / 3.0);
        double V_grid_peak  = p_.V_grid_rms * std::sqrt(2.0);
        double v_src = V_grid_peak * std::sin(2.0 * M_PI * p_.f_grid * s.t + phase_offset);
        I_N[k + 6] += (1.0 / p_.R_src) * v_src;

        // Upper switch injects current from DC bus into inv_k
        bool T1 = (gate >> (2 * k)) & 1;
        double G_T1 = T1 ? p_.G_on : p_.G_off;
        I_N[k] += G_T1 * p_.V_dc;
    }

    // ── Solve G*V_new = I_N ───────────────────────────────────────────────────
    double V_new[N]{};
    lu_solve<N>(LU_, piv_, I_N, V_new);

    // ── Fault detection ───────────────────────────────────────────────────────
    s.fault = false;
    for (int i = 0; i < N; ++i) {
        double absV = V_new[i] < 0 ? -V_new[i] : V_new[i];
        if (absV > 2.0 * p_.V_rated) { s.fault = true; break; }
    }

    // ── Update currents and history ───────────────────────────────────────────
    auto n_Lf_    = make_inductor(p_.L_f,    p_.R_f,    p_.dt);
    auto n_Cf_    = make_capacitor(p_.C_f,   p_.dt);
    auto n_Lgrid_ = make_inductor(p_.L_grid, p_.R_grid, p_.dt);

    for (int k = 0; k < 3; ++k) {
        double v_Lf_new = V_new[k] - V_new[k + 3];
        s.i_Lf[k]     = n_Lf_.G_eq * v_Lf_new + s.I_Lf_hist[k];
        s.I_Lf_hist[k] = inductor_history(n_Lf_, s.i_Lf[k], v_Lf_new);

        s.i_Cf[k]     = n_Cf_.G_C * V_new[k + 3] + s.I_Cf_hist[k];
        s.I_Cf_hist[k] = capacitor_history(n_Cf_, s.i_Cf[k], V_new[k + 3]);

        double v_Lgrid_new = V_new[k + 3] - V_new[k + 6];
        s.i_Lgrid[k]      = n_Lgrid_.G_eq * v_Lgrid_new + s.I_Lgrid_hist[k];
        s.I_Lgrid_hist[k]  = inductor_history(n_Lgrid_, s.i_Lgrid[k], v_Lgrid_new);

    }

    // Update nodal voltages and time
    for (int i = 0; i < N; ++i) s.v[i] = V_new[i];
    s.t += p_.dt;

    return s.fault;
}

} // namespace vsc
