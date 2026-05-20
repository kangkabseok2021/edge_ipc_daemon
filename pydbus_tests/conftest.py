import os
import subprocess
import time
import pytest

DBUS_AVAILABLE = bool(
    os.environ.get("DBUS_SESSION_BUS_ADDRESS")
    or os.environ.get("DBUS_SYSTEM_BUS_ADDRESS")
)

DAEMON_BIN = os.environ.get(
    "EDGE_IPC_DAEMON_BIN",
    os.path.join(os.path.dirname(__file__), "..", "build", "edge_ipc_daemon"),
)

def pytest_collection_modifyitems(items):
    if not DBUS_AVAILABLE:
        marker = pytest.mark.xfail(reason="dbus-not-available", strict=False)
        for item in items:
            item.add_marker(marker)


@pytest.fixture
def daemon_factory():
    """Factory that starts daemon subprocess with optional env overrides.
    Yields a callable: make_daemon(env_overrides=None) → pydbus proxy.
    Cleans up all started processes on teardown.
    """
    if not DBUS_AVAILABLE:
        pytest.xfail("dbus-not-available")

    procs = []

    def make_daemon(env_overrides=None):
        env = os.environ.copy()
        if env_overrides:
            env.update(env_overrides)
        proc = subprocess.Popen(
            [DAEMON_BIN], env=env,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        )
        procs.append(proc)

        from pydbus import SystemBus
        bus = SystemBus()
        deadline = time.time() + 5.0
        while time.time() < deadline:
            try:
                proxy = bus.get("org.agntx.EdgeDaemon", "/org/agntx/EdgeDaemon")
                return proxy
            except Exception:
                time.sleep(0.1)

        proc.terminate()
        proc.wait()
        procs.remove(proc)
        pytest.fail("Daemon did not appear on D-Bus within 5 seconds")

    yield make_daemon

    for proc in procs:
        try:
            proc.terminate()
            proc.wait(timeout=3)
        except Exception:
            proc.kill()


@pytest.fixture
def daemon_proxy(daemon_factory):
    """Simple fixture for a default daemon with no env overrides."""
    return daemon_factory()
