import os
import tempfile
import time
import pytest


@pytest.fixture
def spike_csv():
    """CSV with 20 values above the 80.0 threshold — triggers K=3 alarm."""
    with tempfile.NamedTemporaryFile(mode="w", suffix=".csv", delete=False) as f:
        for _ in range(20):
            f.write("95.0\n")
        path = f.name
    yield path
    os.unlink(path)


@pytest.fixture
def normal_csv():
    with tempfile.NamedTemporaryFile(mode="w", suffix=".csv", delete=False) as f:
        for _ in range(20):
            f.write("50.0\n")
        path = f.name
    yield path
    os.unlink(path)


def test_spike_csv_triggers_fault(daemon_factory, spike_csv):
    proxy = daemon_factory({"TELEMETRY_PATH": spike_csv})
    proxy.Start()
    deadline = time.time() + 5.0
    while time.time() < deadline:
        if proxy.CurrentState == "FAULT":
            break
        time.sleep(0.1)
    assert proxy.CurrentState == "FAULT"


def test_alarm_raised_signal_emitted(daemon_factory, spike_csv):
    from gi.repository import GLib

    alarms = []

    def on_alarm(sensor_id, value, threshold):
        alarms.append((sensor_id, value, threshold))

    proxy = daemon_factory({"TELEMETRY_PATH": spike_csv})
    proxy.AlarmRaised.connect(on_alarm)
    proxy.Start()

    loop = GLib.MainLoop()
    GLib.timeout_add(3000, loop.quit)
    loop.run()

    assert len(alarms) > 0, "AlarmRaised signal was never emitted"


def test_recovery_from_fault(daemon_factory, spike_csv, normal_csv):
    proxy = daemon_factory({"TELEMETRY_PATH": spike_csv})
    proxy.Start()
    deadline = time.time() + 5.0
    while time.time() < deadline and proxy.CurrentState != "FAULT":
        time.sleep(0.1)
    assert proxy.CurrentState == "FAULT"
    proxy.Start()
    assert proxy.CurrentState == "RUNNING"


def test_single_spike_does_not_trigger_fault(daemon_factory):
    """K=3 guard: a CSV with one spike surrounded by normal values stays RUNNING."""
    with tempfile.NamedTemporaryFile(mode="w", suffix=".csv", delete=False) as f:
        # 5 normal, 1 spike, 5 normal — only 1 consecutive, not 3
        for _ in range(5):
            f.write("50.0\n")
        f.write("95.0\n")
        for _ in range(5):
            f.write("50.0\n")
        path = f.name

    try:
        proxy = daemon_factory({"TELEMETRY_PATH": path})
        proxy.Start()
        time.sleep(1.5)  # allow full CSV to be processed
        assert proxy.CurrentState != "FAULT", \
            "Single spike should not trigger FAULT (K=3 guard)"
    finally:
        os.unlink(path)
