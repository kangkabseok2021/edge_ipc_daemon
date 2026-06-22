#pragma once
#include <cstdint>

namespace vsc {

struct SolverParams {
    double dt        = 50e-6;    // timestep [s]
    double L_f       = 3e-3;    // filter inductor [H]
    double R_f       = 0.1;     // filter inductor series R [Ω]
    double C_f       = 20e-6;   // filter capacitor [F]
    double L_grid    = 1e-3;    // grid inductor [H]
    double R_grid    = 0.05;    // grid inductor series R [Ω]
    double C_snub    = 0.1e-6;  // IGBT snubber capacitor [F]
    double V_dc      = 400.0;   // DC bus voltage [V]
    double V_grid_rms = 230.0;  // Grid voltage [Vrms]
    double f_grid    = 50.0;    // Grid frequency [Hz]
    double G_on      = 100.0;   // IGBT on conductance [S]
    double G_off     = 1e-6;    // IGBT off conductance [S]
    double V_rated   = 400.0;   // Rated voltage for fault detection [V]
    double R_src     = 0.01;    // Grid source resistance (stiff Norton) [Ω]
};

// 9-node three-phase VSC network with LC filter.
// Node layout (ground = reference):
//   0,1,2 : inverter AC terminals (V_inv_a/b/c)
//   3,4,5 : filter cap midpoints  (V_cap_a/b/c, after L_f)
//   6,7,8 : grid terminals        (V_grid_a/b/c)
class VscNetwork {
public:
    static constexpr int N = 9;

    struct State {
        double v[N]{};          // nodal voltages [V]
        double i_Lf[3]{};       // filter inductor currents [A]
        double i_Cf[3]{};       // filter capacitor currents [A]
        double i_Lgrid[3]{};    // grid inductor currents [A]
        double I_Lf_hist[3]{};  // Norton history for L_f
        double I_Cf_hist[3]{};  // Norton history for C_f
        double I_Lgrid_hist[3]{};// Norton history for L_grid
        double I_snub_hist[6]{}; // snubber capacitor history (one per IGBT)
        double t{0.0};          // simulation time [s]
        uint8_t gate{0};        // current gate state (6 IGBT bits)
        bool fault{false};
    };

    explicit VscNetwork(const SolverParams& p);

    // Advance one timestep. gate bits: bit0=T1a,bit1=T2a,bit2=T1b,bit3=T2b,bit4=T1c,bit5=T2c
    // Returns true if fault detected (|V|>2*V_rated).
    bool step(State& s, uint8_t gate);

    // Read assembled G_total (for test inspection).
    const double* G_total_ptr() const { return &G_[0][0]; }

private:
    SolverParams p_;
    double G_[N][N]{};
    double LU_[N][N]{};
    int piv_[N]{};
    uint8_t gate_factored_{0xFF};

    void assemble_and_factor(uint8_t gate);
};

} // namespace vsc
