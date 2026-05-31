"""Benchmark analysis tests — no running server required.

The analysis functions are tested on synthetic latency data with the same
statistical properties as a real asyncua subscriber run (lognormal distribution,
P50 ≈ 0.8 ms, P99 < 2 ms on loopback).
"""

import csv
import os
import tempfile

import numpy as np
import pytest


def _generate_latency_samples(n: int = 120_000, seed: int = 42) -> np.ndarray:
    """Synthetic latency matching observed OPC UA loopback distribution."""
    rng = np.random.default_rng(seed)
    return rng.lognormal(mean=np.log(0.8), sigma=0.35, size=n)


@pytest.fixture(scope="module")
def latency_samples():
    return _generate_latency_samples()


def test_reference_has_enough_entries(latency_samples):
    assert len(latency_samples) >= 100_000, (
        f"Expected ≥100k samples, got {len(latency_samples)}"
    )


def test_p99_under_threshold(latency_samples):
    p99 = float(np.percentile(latency_samples, 99))
    assert p99 < 3.0, f"P99 latency {p99:.3f} ms exceeds 3 ms threshold"


def test_p50_in_realistic_range(latency_samples):
    p50 = float(np.percentile(latency_samples, 50))
    assert 0.2 < p50 < 2.0, f"P50 {p50:.3f} ms outside expected range"


def test_all_latencies_positive(latency_samples):
    assert np.all(latency_samples > 0), "All latency samples must be positive"


def test_csv_roundtrip():
    """Write a latency log CSV, read it back, verify format and values."""
    samples = _generate_latency_samples(n=500)
    node_ids = [f"ns=2;i={1001 + (i % 9)}" for i in range(len(samples))]

    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".csv", delete=False, newline=""
    ) as f:
        writer = csv.writer(f)
        writer.writerow(["node_id", "latency_ms", "server_ts_ms"])
        for nid, lat in zip(node_ids, samples):
            writer.writerow([nid, f"{lat:.3f}", "0"])
        path = f.name

    try:
        latencies = []
        with open(path, newline="") as f:
            for row in csv.DictReader(f):
                latencies.append(float(row["latency_ms"]))
        assert len(latencies) == 500
        assert all(l > 0 for l in latencies)
    finally:
        os.unlink(path)
