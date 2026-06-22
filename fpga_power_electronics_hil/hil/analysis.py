"""Waveform analysis: THD, power factor, step-response settle time."""
from __future__ import annotations

import numpy as np


def compute_thd(v: np.ndarray, dt: float, f_fund: float = 50.0) -> float:
    """Total harmonic distortion (%) using harmonics 2–10 vs fundamental."""
    n = len(v)
    window = np.hanning(n)
    spectrum = np.fft.rfft(v * window)
    freqs = np.fft.rfftfreq(n, d=dt)
    k1 = int(round(f_fund / freqs[1]))
    if k1 >= len(spectrum):
        return float("inf")
    V1 = abs(spectrum[k1])
    if V1 < 1e-12:
        return float("inf")
    harmonics_sq = sum(
        abs(spectrum[k1 * h]) ** 2
        for h in range(2, 11)
        if k1 * h < len(spectrum)
    )
    return 100.0 * float(np.sqrt(harmonics_sq)) / V1


def compute_rms(v: np.ndarray) -> float:
    return float(np.sqrt(np.mean(v**2)))


def settle_time_steps(
    signal: list[float],
    reference: list[float],
    band: float = 0.05,
    consecutive: int = 10,
) -> int | None:
    """First step index where |signal-ref| < band*|ref_max| holds for `consecutive` steps."""
    ref_max = max(abs(r) for r in reference) or 1.0
    tol = band * ref_max
    for i in range(len(signal) - consecutive):
        if all(abs(signal[i + j] - reference[i + j]) < tol for j in range(consecutive)):
            return i
    return None
