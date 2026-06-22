#pragma once

namespace vsc {

struct NortonInductor {
    double G_eq;   // companion conductance = dt / (2L + R*dt)
    double beta;   // history coefficient  = (2L - R*dt) / (2L + R*dt)
};

struct NortonCapacitor {
    double G_C;    // companion conductance = 2C / dt
};

// Build companion parameters for RL branch.
NortonInductor make_inductor(double L, double R, double dt);

// Build companion parameters for capacitor.
NortonCapacitor make_capacitor(double C, double dt);

// Update history current for RL inductor companion.
// v_branch = v_pos - v_neg (voltage across the branch at current step)
// i_L = current through inductor at current step
// Returns new I_hist for next step.
double inductor_history(const NortonInductor& n, double i_L, double v_branch);

// Update history current for capacitor companion.
// v_node = node voltage at current step (capacitor between node and ground)
// i_C = current into capacitor at current step
// Returns new I_hist for next step.
double capacitor_history(const NortonCapacitor& n, double i_C, double v_node);

} // namespace vsc
