# edge_ipc_daemon

![CI](https://github.com/kangkabseok2021/edge_ipc_daemon/actions/workflows/ci.yml/badge.svg)

A C++17 Linux daemon demonstrating production-quality embedded software patterns:
D-Bus IPC via `sd-bus`, four-state FSM, signal filtering (moving average + threshold), systemd `Type=notify` lifecycle, and a formal AddressSanitizer defect analysis.

The `secure_distributed_node/` subfolder extends this project with a gRPC distribution layer — mutual TLS, a 3-node docker-compose cluster, and a Python integration test suite.

---

## Architecture

```
SimulatedSource / FileSource
        │  TelemetryFrame
        ▼
TelemetryReceiver (SPSC lock-free ring)
        │
        ▼
DataProcessor (MovingAverageFilter + ThresholdDetector)
        │  ProcessedMetric + alarm_flag
        ▼
StateManager (IDLE → RUNNING → FAULT → IDLE)
        │  StateChanged / AlarmRaised
        ▼
DaemonBus (org.agntx.EdgeDaemon1 on system bus)
        │
        ▼
main()  sd_notify READY / WATCHDOG / STOPPING
```

### Secure Distributed Node (extension)

```
coordinator ──mTLS gRPC──▶ worker1:50051 (TelemetryNode + above pipeline)
            ──mTLS gRPC──▶ worker2:50051 (TelemetryNode + above pipeline)
                │
                ▼
         GET /health → {worker1: {fsm_state, last_value, reachable}, all_running}
```

See [`secure_distributed_node/docs/DISTRIBUTION-DESIGN.md`](secure_distributed_node/docs/DISTRIBUTION-DESIGN.md) for the full design.

---

## Build

### Local (macOS / no libsystemd)

```bash
cmake -B build -S . -DSTUB_DBUS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure -V
```

### Linux (with libsystemd)

```bash
sudo apt-get install libsystemd-dev
cmake -B build -S . -DSTUB_DBUS=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure -V
```

### ARM64 cross-compile

```bash
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
cmake -B build-arm64 -S . \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm64.cmake \
  -DSTUB_DBUS=ON -DBUILD_TESTS=OFF
cmake --build build-arm64 --target edge_ipc_daemon -j$(nproc)
```

### ASan build

```bash
cmake -B build-asan -S . -DENABLE_ASAN=ON -DSTUB_DBUS=OFF
cmake --build build-asan -j$(nproc)
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-asan -V
```

### Secure Distributed Node (gRPC, requires libgrpc++-dev)

```bash
# macOS: brew install grpc
# Linux: sudo apt-get install libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
#                             libssl-dev libgtest-dev libgmock-dev

cmake -B build/sdn -S secure_distributed_node -DCMAKE_BUILD_TYPE=Release
cmake --build build/sdn --target telemetry_node telemetry_coordinator sdn_tests -j$(nproc)
ctest --test-dir build/sdn --output-on-failure -V   # 8 C++ tests
bash secure_distributed_node/certs/gen_certs.sh
uv run pytest secure_distributed_node/tests/test_python/ -v   # 5 Python tests
```

---

## Tests

### GoogleTest (20 tests, no D-Bus required)

```bash
ctest --test-dir build --output-on-failure -V
```

### Python pydbus suite (11 tests, requires system D-Bus)

```bash
uv sync
uv run pytest pydbus_tests/ -v
```

On CI and macOS (no system bus), all pydbus tests are auto-marked `xfail`.

### Secure Distributed Node: 8 C++ + 5 Python gRPC tests

```bash
# C++ unit tests (TelemetryNode + TlsCredentials)
ctest --test-dir build/sdn --output-on-failure -V

# Python mTLS integration tests (requires telemetry_node binary + certs)
bash secure_distributed_node/certs/gen_certs.sh
TELEMETRY_NODE_BIN=build/sdn/telemetry_node \
  uv run pytest secure_distributed_node/tests/test_python/ -v
```

---

## D-Bus Interface

**Service:** `org.agntx.EdgeDaemon`  
**Object:** `/org/agntx/EdgeDaemon`  
**Interface:** `org.agntx.EdgeDaemon1`

| Method | Signature | Description |
|---|---|---|
| `Start()` | → | IDLE/FAULT → RUNNING |
| `Halt()` | → | any → IDLE |
| `RunDiagnostics()` | → `(ttst)` | memory_rss_kb, processed_count, state, uptime_s |

| Signal | Signature | Description |
|---|---|---|
| `StateChanged` | `ss` | new_state, prev_state |
| `AlarmRaised` | `sdd` | sensor_id, value, threshold |

| Property | Type | Description |
|---|---|---|
| `CurrentState` | `s` | Current FSM state string |

```bash
# Example: start the daemon and call methods
busctl call org.agntx.EdgeDaemon /org/agntx/EdgeDaemon \
  org.agntx.EdgeDaemon1 Start
```

---

## systemd Installation

```bash
sudo cp build/edge_ipc_daemon /usr/local/bin/
sudo cp deploy/edge-ipc-daemon.service /etc/systemd/system/
sudo cp deploy/org.agntx.EdgeDaemon1.conf /etc/dbus-1/system.d/
sudo systemctl daemon-reload
sudo systemctl enable --now edge-ipc-daemon
journalctl -fu edge-ipc-daemon
```

---

## Security

- **D-Bus policy:** `deploy/org.agntx.EdgeDaemon1.conf` — deny all; allow only `edge-daemon` user and `root`.
- **systemd sandbox:** `DynamicUser=true`, `PrivateTmp=true`, `NoNewPrivileges=true`, `ProtectSystem=strict`.
- **Input validation:** Invalid D-Bus method arguments return `org.agntx.Error.InvalidArgument` without crashing.

## Defect Analysis

A use-after-free caught by AddressSanitizer during teardown is documented in [`docs/defect-report-001.md`](docs/defect-report-001.md). Root cause: `DaemonBus` captured a raw `StateManager*` in D-Bus callbacks. Fix: `weak_ptr<StateManager>` + `lock()` before every dereference.
