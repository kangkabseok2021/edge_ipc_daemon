#include "NortonUpdate.h"

namespace vsc {

NortonInductor make_inductor(double L, double R, double dt) {
    double denom = 2.0 * L + R * dt;
    return { dt / denom, (2.0 * L - R * dt) / denom };
}

NortonCapacitor make_capacitor(double C, double dt) {
    return { 2.0 * C / dt };
}

double inductor_history(const NortonInductor& n, double i_L, double v_branch) {
    return n.G_eq * v_branch + n.beta * i_L;
}

double capacitor_history(const NortonCapacitor& n, double i_C, double v_node) {
    return -i_C - n.G_C * v_node;
}

} // namespace vsc
