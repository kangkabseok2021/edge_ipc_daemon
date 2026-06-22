"""Python reference trapezoidal DAE solver for the 9-node VSC network.

Node layout (ground = reference):
  0,1,2 : inverter AC terminals (V_inv_a/b/c)
  3,4,5 : filter cap midpoints  (V_cap_a/b/c)
  6,7,8 : grid terminals        (V_grid_a/b/c)
"""
from __future__ import annotations

import math
import numpy as np

_TWO_PI = 2.0 * math.pi


class SolverParams:
    dt: float = 50e-6
    L_f: float = 3e-3
    R_f: float = 0.1
    C_f: float = 20e-6
    L_grid: float = 1e-3
    R_grid: float = 0.05
    C_snub: float = 0.1e-6
    V_dc: float = 400.0
    V_grid_rms: float = 230.0
    f_grid: float = 50.0
    G_on: float = 100.0
    G_off: float = 1e-6
    V_rated: float = 400.0
    R_src: float = 0.01


def _make_inductor(L: float, R: float, dt: float) -> tuple[float, float]:
    """Returns (G_eq, beta) for RL companion circuit."""
    denom = 2.0 * L + R * dt
    return dt / denom, (2.0 * L - R * dt) / denom


def _make_cap(C: float, dt: float) -> float:
    """Returns G_C = 2C/dt for capacitor companion."""
    return 2.0 * C / dt


def _inductor_history(G_eq: float, beta: float, i_L: float, v_branch: float) -> float:
    return G_eq * v_branch + beta * i_L


def _cap_history(G_C: float, i_C: float, v_node: float) -> float:
    return -i_C - G_C * v_node


def _stamp(G: np.ndarray, p: int, m: int | None, g: float) -> None:
    G[p, p] += g
    if m is not None:
        G[m, m] += g
        G[p, m] -= g
        G[m, p] -= g


def _assemble_G(p: SolverParams, gate: int) -> np.ndarray:
    N = 9
    G = np.zeros((N, N))
    G_Lf, _ = _make_inductor(p.L_f, p.R_f, p.dt)
    G_Cf = _make_cap(p.C_f, p.dt)
    G_Lgrid, _ = _make_inductor(p.L_grid, p.R_grid, p.dt)
    G_vs = 1.0 / p.R_src
    for k in range(3):
        _stamp(G, k, k + 3, G_Lf)
        _stamp(G, k + 3, None, G_Cf)
        _stamp(G, k + 3, k + 6, G_Lgrid)
        _stamp(G, k + 6, None, G_vs)
        T1 = bool((gate >> (2 * k)) & 1)
        T2 = bool((gate >> (2 * k + 1)) & 1)
        G_T1 = p.G_on if T1 else p.G_off
        G_T2 = p.G_on if T2 else p.G_off
        _stamp(G, k, None, G_T1 + G_T2)
    return G


class VscNetworkState:
    def __init__(self) -> None:
        self.v = np.zeros(9)
        self.i_Lf = np.zeros(3)
        self.i_Cf = np.zeros(3)
        self.i_Lgrid = np.zeros(3)
        self.I_Lf_hist = np.zeros(3)
        self.I_Cf_hist = np.zeros(3)
        self.I_Lgrid_hist = np.zeros(3)
        self.I_snub_hist = np.zeros(6)
        self.t: float = 0.0
        self.gate: int = 0
        self.fault: bool = False


class VscNetwork:
    def __init__(self, p: SolverParams | None = None) -> None:
        self._p = p or SolverParams()
        self._gate_factored = -1
        self._G_lu: tuple[np.ndarray, np.ndarray] | None = None  # (LU, piv)
        self._G: np.ndarray = _assemble_G(self._p, 0)

    def _get_lu(self, gate: int) -> tuple[np.ndarray, np.ndarray]:
        if gate != self._gate_factored:
            G = _assemble_G(self._p, gate)
            self._G = G
            # scipy-style LU
            from scipy.linalg import lu_factor  # type: ignore[import-untyped]
            self._G_lu = lu_factor(G)
            self._gate_factored = gate
        return self._G_lu  # type: ignore[return-value]

    def step(self, s: VscNetworkState, gate: int) -> bool:
        p = self._p
        lu = self._get_lu(gate)
        s.gate = gate

        N = 9
        I_N = np.zeros(N)
        G_Lf, _ = _make_inductor(p.L_f, p.R_f, p.dt)
        G_Lgrid, _ = _make_inductor(p.L_grid, p.R_grid, p.dt)
        G_Cf = _make_cap(p.C_f, p.dt)

        V_grid_peak = p.V_grid_rms * math.sqrt(2.0)
        G_vs = 1.0 / p.R_src

        for k in range(3):
            # Inductor Norton: current source flows from inv→cap, so -I_hist at inv, +I_hist at cap
            I_N[k] -= s.I_Lf_hist[k]
            I_N[k + 3] += s.I_Lf_hist[k]
            # Capacitor Norton: companion current flows cap_k→ground (leaves node)
            I_N[k + 3] -= s.I_Cf_hist[k]
            # Grid inductor Norton: current source flows from cap→grid
            I_N[k + 3] -= s.I_Lgrid_hist[k]
            I_N[k + 6] += s.I_Lgrid_hist[k]

            phase_off = k * (-_TWO_PI / 3.0)
            v_src = V_grid_peak * math.sin(_TWO_PI * p.f_grid * s.t + phase_off)
            I_N[k + 6] += G_vs * v_src

            T1 = bool((gate >> (2 * k)) & 1)
            G_T1 = p.G_on if T1 else p.G_off
            # Upper switch injects current from DC bus into inv_k
            # Snubbers (τ≈1ns≪dt) are in the quasi-static limit C→0 → omitted
            I_N[k] += G_T1 * p.V_dc

        from scipy.linalg import lu_solve  # type: ignore[import-untyped]
        V_new = lu_solve(lu, I_N)

        s.fault = bool(np.any(np.abs(V_new) > 2.0 * p.V_rated))

        G_Lf_eq, beta_Lf = _make_inductor(p.L_f, p.R_f, p.dt)
        G_Lgrid_eq, beta_Lgrid = _make_inductor(p.L_grid, p.R_grid, p.dt)

        for k in range(3):
            v_Lf = V_new[k] - V_new[k + 3]
            s.i_Lf[k] = G_Lf_eq * v_Lf + s.I_Lf_hist[k]
            s.I_Lf_hist[k] = _inductor_history(G_Lf_eq, beta_Lf, s.i_Lf[k], v_Lf)

            s.i_Cf[k] = G_Cf * V_new[k + 3] + s.I_Cf_hist[k]
            s.I_Cf_hist[k] = _cap_history(G_Cf, s.i_Cf[k], V_new[k + 3])

            v_Lg = V_new[k + 3] - V_new[k + 6]
            s.i_Lgrid[k] = G_Lgrid_eq * v_Lg + s.I_Lgrid_hist[k]
            s.I_Lgrid_hist[k] = _inductor_history(G_Lgrid_eq, beta_Lgrid, s.i_Lgrid[k], v_Lg)

            # Snubber τ ≈ 1ns ≪ dt=50µs → quasi-static, no history tracking needed

        s.v[:] = V_new
        s.t += p.dt
        return s.fault
