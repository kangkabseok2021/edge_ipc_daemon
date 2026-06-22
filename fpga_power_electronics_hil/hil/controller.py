"""Discrete PI current controller and SPWM gate modulator."""
from __future__ import annotations

import math

_TWO_PI = 2.0 * math.pi


class PiController:
    """Per-phase discrete PI with anti-windup clamp."""

    def __init__(
        self,
        k_p: float = 0.5,
        k_i: float = 20.0,
        dt: float = 50e-6,
        x_max: float = 5.0,
    ) -> None:
        self.k_p = k_p
        self.k_i = k_i
        self.dt = dt
        self.x_max = x_max
        self._x: list[float] = [0.0, 0.0, 0.0]  # integrator state per phase

    def step(self, i_ref: list[float], i_meas: list[float]) -> list[float]:
        """Returns duty cycle demand u[k] ∈ [-1, 1] per phase."""
        u = []
        for k in range(3):
            e = i_ref[k] - i_meas[k]
            x_new = self._x[k] + self.dt * e
            x_new = max(-self.x_max, min(self.x_max, x_new))
            self._x[k] = x_new
            u_k = self.k_p * e + self.k_i * x_new
            u.append(max(-1.0, min(1.0, u_k)))
        return u


class SpwmModulator:
    """Sinusoidal PWM — 10 kHz carrier (every 5 simulation steps at 20 kHz)."""

    def __init__(self, dt: float = 50e-6, carrier_freq: float = 10e3) -> None:
        self._dt = dt
        self._carrier_period = 1.0 / carrier_freq
        # Start at quarter-period so carrier(t=0)=0; avoids forcing upper switches
        # at t=0 when u=0, which would create a large initial current transient.
        self._carrier_t = 0.25 * self._carrier_period

    def _carrier(self) -> float:
        """Triangle wave in [-1, 1]."""
        phi = (self._carrier_t % self._carrier_period) / self._carrier_period
        return 4.0 * phi - 1.0 if phi < 0.5 else 3.0 - 4.0 * phi

    def modulate(self, u: list[float]) -> int:
        """Return 6-bit gate state from duty cycles.
        Bit layout: bit 2k = T1k (upper), bit 2k+1 = T2k (lower), k=0,1,2.
        """
        carrier = self._carrier()
        self._carrier_t += self._dt
        gate = 0
        for k in range(3):
            if u[k] > carrier:
                gate |= (1 << (2 * k))       # T1k on
            else:
                gate |= (1 << (2 * k + 1))   # T2k on
        return gate


def make_i_ref(t: float, I_rated: float, f_grid: float = 50.0, ramp_s: float = 0.04) -> list[float]:
    """Three-phase sinusoidal current reference with linear ramp-up."""
    amp = min(1.0, t / ramp_s) * I_rated
    return [
        amp * math.sin(_TWO_PI * f_grid * t),
        amp * math.sin(_TWO_PI * f_grid * t - _TWO_PI / 3.0),
        amp * math.sin(_TWO_PI * f_grid * t + _TWO_PI / 3.0),
    ]
