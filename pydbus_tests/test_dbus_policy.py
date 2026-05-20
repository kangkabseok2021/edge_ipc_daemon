import subprocess
import sys
import pytest


def test_unprivileged_start_denied():
    """Calling Start() as an unprivileged user must raise AccessDenied."""
    result = subprocess.run(
        [sys.executable, "-c",
         "from pydbus import SystemBus; "
         "b = SystemBus(); "
         "p = b.get('org.agntx.EdgeDaemon', '/org/agntx/EdgeDaemon'); "
         "p.Start()"],
        capture_output=True, text=True,
    )
    assert result.returncode != 0, "Expected non-zero exit for unprivileged call"
    assert "AccessDenied" in result.stderr or "org.freedesktop.DBus.Error" in result.stderr


def test_unprivileged_own_denied():
    """An unprivileged process must not be able to own org.agntx.EdgeDaemon."""
    result = subprocess.run(
        [sys.executable, "-c",
         "import dbus; "
         "bus = dbus.SystemBus(); "
         "bus.request_name('org.agntx.EdgeDaemon')"],
        capture_output=True, text=True,
    )
    assert result.returncode != 0
