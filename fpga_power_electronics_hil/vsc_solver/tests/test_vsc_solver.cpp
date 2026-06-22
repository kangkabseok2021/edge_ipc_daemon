#include <gtest/gtest.h>
#include <cmath>
#include <stdexcept>
#include "LuSolver.h"
#include "NortonUpdate.h"
#include "VscNetwork.h"

// ─── LuSolver ────────────────────────────────────────────────────────────────

TEST(LuSolver, SolvesTridiagonal3x3) {
    // [[4,1,0],[1,3,1],[0,1,4]] * [1,2,3] = [6,9,11]
    double A[3][3] = {{4,1,0},{1,3,1},{0,1,4}};
    int pivot[3];
    vsc::lu_factor<3>(A, pivot);

    double b[3] = {6, 10, 14};  // A * [1,2,3]
    double x[3];
    vsc::lu_solve<3>(A, pivot, b, x);

    EXPECT_NEAR(x[0], 1.0, 1e-10);
    EXPECT_NEAR(x[1], 2.0, 1e-10);
    EXPECT_NEAR(x[2], 3.0, 1e-10);
}

TEST(LuSolver, SolvesIdentityTimesB) {
    double A[4][4] = {};
    for (int i = 0; i < 4; ++i) A[i][i] = 1.0;
    int pivot[4];
    vsc::lu_factor<4>(A, pivot);

    double b[4] = {3.0, -1.5, 7.0, 0.25};
    double x[4];
    vsc::lu_solve<4>(A, pivot, b, x);

    for (int i = 0; i < 4; ++i)
        EXPECT_NEAR(x[i], b[i], 1e-10);
}

TEST(LuSolver, ThrowsOnSingularMatrix) {
    double A[3][3] = {{1,2,3},{2,4,6},{0,0,1}};
    int pivot[3];
    EXPECT_THROW(vsc::lu_factor<3>(A, pivot), std::runtime_error);
}

// ─── NortonUpdate ─────────────────────────────────────────────────────────────

TEST(NortonUpdate, InductorCompanionAdmittance) {
    // G_eq = dt / (2L + R*dt)
    double dt = 50e-6, L = 3e-3, R = 0.1;
    auto n = vsc::make_inductor(L, R, dt);
    double expected_G = dt / (2.0*L + R*dt);
    EXPECT_NEAR(n.G_eq, expected_G, 1e-12);
}

TEST(NortonUpdate, InductorHistoryCurrentFormula) {
    // I_hist[k] = G_eq * v_branch[k] + beta * i_L[k]
    double dt = 50e-6, L = 3e-3, R = 0.1;
    auto n = vsc::make_inductor(L, R, dt);
    double i_L = 5.0, v_branch = 10.0;
    double expected = n.G_eq * v_branch + n.beta * i_L;
    EXPECT_NEAR(vsc::inductor_history(n, i_L, v_branch), expected, 1e-12);
}

TEST(NortonUpdate, CapacitorHistoryCurrentFormula) {
    // I_C_hist[k] = -i_C[k] - G_C * v_node[k]
    double dt = 50e-6, C = 20e-6;
    auto n = vsc::make_capacitor(C, dt);
    double G_C_expected = 2.0 * C / dt;
    EXPECT_NEAR(n.G_C, G_C_expected, 1e-12);

    double i_C = 2.0, v_node = 300.0;
    double expected = -i_C - n.G_C * v_node;
    EXPECT_NEAR(vsc::capacitor_history(n, i_C, v_node), expected, 1e-12);
}

// ─── VscNetwork ───────────────────────────────────────────────────────────────

TEST(VscNetwork, GtotalHasPositiveDiagonal) {
    vsc::SolverParams p;
    vsc::VscNetwork net(p);
    const double* G = net.G_total_ptr();
    for (int i = 0; i < vsc::VscNetwork::N; ++i)
        EXPECT_GT(G[i * vsc::VscNetwork::N + i], 0.0)
            << "Diagonal G[" << i << "][" << i << "] must be positive";
}

TEST(VscNetwork, SingleStepAllGatesOffFiniteState) {
    vsc::SolverParams p;
    vsc::VscNetwork net(p);
    vsc::VscNetwork::State s;
    // Start with DC bus charge applied via gate sequence
    bool fault = net.step(s, 0x00);
    EXPECT_FALSE(fault);
    for (int i = 0; i < vsc::VscNetwork::N; ++i)
        EXPECT_TRUE(std::isfinite(s.v[i])) << "v[" << i << "] is not finite";
}

TEST(VscNetwork, FaultFlagOnOvervoltage) {
    vsc::SolverParams p;
    p.V_rated = 1.0;  // very low threshold → normal voltages trigger fault
    vsc::VscNetwork net(p);
    vsc::VscNetwork::State s;
    // Force a non-zero voltage by turning on all upper gates
    bool fault = false;
    for (int i = 0; i < 50 && !fault; ++i)
        fault = net.step(s, 0x15);  // T1a=1, T1b=1, T1c=1 (bits 0,2,4)
    EXPECT_TRUE(fault);
}
