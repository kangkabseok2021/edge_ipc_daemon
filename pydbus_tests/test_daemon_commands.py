import time
import pytest


def test_start_transitions_to_running(daemon_proxy):
    daemon_proxy.Start()
    assert daemon_proxy.CurrentState == "RUNNING"


def test_halt_returns_to_idle(daemon_proxy):
    daemon_proxy.Start()
    daemon_proxy.Halt()
    assert daemon_proxy.CurrentState == "IDLE"


def test_halt_from_idle_is_safe(daemon_proxy):
    daemon_proxy.Halt()
    assert daemon_proxy.CurrentState == "IDLE"


def test_diagnostics_returns_valid_struct(daemon_proxy):
    daemon_proxy.Start()
    result = daemon_proxy.RunDiagnostics()
    memory_rss_kb, processed_count, filter_state, uptime_s = result
    assert memory_rss_kb >= 0
    assert processed_count >= 0
    assert filter_state in ("IDLE", "RUNNING", "FAULT", "SHUTDOWN")
    assert uptime_s >= 0


def test_state_changed_signal_received(daemon_proxy):
    from pydbus import SystemBus
    from gi.repository import GLib

    bus = SystemBus()
    received = []

    def on_state_changed(new_state, prev_state):
        received.append((new_state, prev_state))

    proxy = bus.get("org.agntx.EdgeDaemon", "/org/agntx/EdgeDaemon")
    proxy.StateChanged.connect(on_state_changed)

    daemon_proxy.Start()

    loop = GLib.MainLoop()
    GLib.timeout_add(1000, loop.quit)
    loop.run()

    assert any(new == "RUNNING" for new, _ in received), \
        f"StateChanged to RUNNING not received; got: {received}"
