# Edge IPC Daemon & Secure Distributed Telemetry Node

![CI](https://github.com/kangkabseok2021/edge_ipc_daemon/actions/workflows/ci.yml/badge.svg)

Two C++ systems in one repository — a production-quality Linux daemon and a gRPC extension layer with mutual TLS.

| Project | Description | Docs |
|---|---|---|
| **Edge IPC Daemon** | D-Bus IPC via `sd-bus`, four-state FSM, signal filtering (moving average + threshold), systemd `Type=notify` lifecycle, ARM64 cross-compile, and AddressSanitizer defect analysis | [docs/defect-report-001.md](docs/defect-report-001.md) |
| **Secure Distributed Node** | gRPC server-streaming `TelemetryService`, mutual TLS (OpenSSL CA), 3-node docker-compose cluster, coordinator REST `/health` endpoint, and a Python mTLS integration test suite | [secure_distributed_node/docs/DISTRIBUTION-DESIGN.md](secure_distributed_node/docs/DISTRIBUTION-DESIGN.md) |

---

## Repository Layout

```
edge_ipc_daemon/
├── include/
│   ├── ITelemetrySource.h           Abstract source interface
│   ├── IDataFilter.h                Abstract filter interface
│   ├── SimulatedSource.h            Gaussian-noise synthetic sensor
│   ├── FileSource.h                 Line-by-line CSV replay source
│   ├── MovingAverageFilter.h/cpp    Sliding-window signal smoother
│   ├── ThresholdDetector.h/cpp      Level-crossing alarm detector
│   ├── DataProcessor.h/cpp          Filter + threshold orchestrator
│   ├── TelemetryReceiver.h/cpp      SPSC lock-free ring buffer
│   ├── StateManager.h/cpp           IDLE → RUNNING → FAULT → IDLE FSM
│   └── DaemonBus.h/cpp              sd-bus org.agntx.EdgeDaemon1 service
├── src/main.cpp                     sd_notify READY / WATCHDOG / STOPPING
├── tests/
│   ├── test_filters.cpp             7 GoogleTests  (MovingAverage + Threshold)
│   ├── test_fsm.cpp                 9 GoogleTests  (StateManager transitions)
│   ├── test_sources.cpp             2 GoogleTests  (SimulatedSource + FileSource)
│   └── test_spsc.cpp                2 GoogleTests  (TelemetryReceiver)
├── pydbus_tests/
│   ├── test_daemon_commands.py      5 D-Bus method tests  (xfail without system bus)
│   ├── test_dbus_policy.py          2 D-Bus policy tests  (xfail without system bus)
│   └── test_fault_injection.py      4 fault-injection tests (xfail without system bus)
├── deploy/
│   ├── edge-ipc-daemon.service      systemd unit  (DynamicUser, PrivateTmp, NoNewPrivileges)
│   └── org.agntx.EdgeDaemon1.conf   D-Bus policy  (deny-all + root/daemon-user allow)
├── docs/defect-report-001.md        ASan use-after-free analysis + weak_ptr fix
├── cmake/toolchain-arm64.cmake      AArch64 cross-compile toolchain
├── secure_distributed_node/
│   ├── proto/telemetry.proto        TelemetryService (GetNodeStatus + StreamTelemetry)
│   ├── include/
│   │   ├── TelemetryNode.h          gRPC server wrapping the full pipeline
│   │   ├── TelemetryCoordinator.h   Multi-worker poller + REST /health
│   │   └── TlsCredentials.h         mTLS cert loader (server + channel creds)
│   ├── src/
│   │   ├── TelemetryNode.cpp        ServiceImpl + ProducerLoop + stream queue
│   │   ├── TelemetryCoordinator.cpp Worker polling, cpp-httplib REST server
│   │   ├── TlsCredentials.cpp       grpc::SslServerCredentials + CLIENT_AND_VERIFY
│   │   ├── main_node.cpp            node_id + port + certs-dir CLI entry point
│   │   └── main_coordinator.cpp     worker-list + certs-dir CLI entry point
│   ├── tests/
│   │   ├── test_telemetry_node.cpp  4 C++ unit tests  (start/stop, port, FSM)
│   │   ├── test_tls_credentials.cpp 4 C++ unit tests  (LoadFromDir, missing cert)
│   │   └── test_python/
│   │       └── test_grpc_client.py  5 Python mTLS integration tests
│   ├── certs/gen_certs.sh           OpenSSL CA + per-node cert generator
│   ├── docker/
│   │   ├── Dockerfile               Multi-stage ubuntu:22.04 build + runtime
│   │   └── docker-compose.yml       coordinator + worker1 + worker2 cluster
│   └── docs/DISTRIBUTION-DESIGN.md  Architecture, sequence diagrams, ADRs
└── CMakeLists.txt                   Core build; SDN opt-in via -DBUILD_DISTRIBUTED=ON
```

---

## Quick Start

### Core daemon (macOS / no libsystemd)

```bash
cmake -B build -S . -DSTUB_DBUS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure -V    # 20 GoogleTests
```

### Core daemon (Linux with libsystemd)

```bash
sudo apt-get install libsystemd-dev
cmake -B build -S . -DSTUB_DBUS=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure -V    # 20 GoogleTests
```

### ARM64 cross-compile

```bash
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
cmake -B build-arm64 -S . \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm64.cmake \
  -DSTUB_DBUS=ON -DBUILD_TESTS=OFF
cmake --build build-arm64 --target edge_ipc_daemon -j$(nproc)
```

### Secure Distributed Node (requires libgrpc++-dev)

```bash
# macOS:  brew install grpc
# Linux:  sudo apt-get install libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
#                               libssl-dev libgtest-dev libgmock-dev

cmake -B build/sdn -S secure_distributed_node -DCMAKE_BUILD_TYPE=Release
cmake --build build/sdn --target telemetry_node telemetry_coordinator sdn_tests -j$(nproc)
ctest --test-dir build/sdn --output-on-failure -V    # 8 C++ tests

bash secure_distributed_node/certs/gen_certs.sh
TELEMETRY_NODE_BIN=build/sdn/telemetry_node \
  uv run pytest secure_distributed_node/tests/test_python/ -v    # 5 Python tests
```

### 3-node docker-compose cluster

```bash
cd secure_distributed_node/docker
docker compose up --build
curl http://localhost:8080/health
```

---

## D-Bus Interface

**Service:** `org.agntx.EdgeDaemon` · **Object:** `/org/agntx/EdgeDaemon` · **Interface:** `org.agntx.EdgeDaemon1`

| Method | Signature | Description |
|---|---|---|
| `Start()` | → | IDLE / FAULT → RUNNING |
| `Halt()` | → | any → IDLE |
| `RunDiagnostics()` | → `(ttst)` | memory_rss_kb, processed_count, state, uptime_s |

| Signal | Signature | Description |
|---|---|---|
| `StateChanged` | `ss` | new_state, prev_state |
| `AlarmRaised` | `sdd` | sensor_id, value, threshold |

---

## CI

| Job | Platform | Tests |
|---|---|---|
| `cpp-x86` | ubuntu-latest | cmake + 20 GoogleTests (real libsystemd) |
| `cpp-asan` | ubuntu-latest | ASan + UBSan, same 20 tests |
| `cpp-arm64` | ubuntu-latest | AArch64 cross-compile smoke check |
| `python` | ubuntu-latest | 11 pydbus tests (all xfail — no system bus on CI) |
| `sdn-grpc-tests` | ubuntu-22.04 | 8 C++ gRPC unit tests |
| `sdn-python-tests` | ubuntu-22.04 | 5 Python mTLS integration tests |

---

## Security

- **D-Bus policy:** `deploy/org.agntx.EdgeDaemon1.conf` — deny all; allow only `edge-daemon` user and `root`.
- **systemd sandbox:** `DynamicUser=true`, `PrivateTmp=true`, `NoNewPrivileges=true`, `ProtectSystem=strict`.
- **mTLS:** workers reject any client not signed by the project CA (`GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY`).

## Defect Analysis

A use-after-free caught by AddressSanitizer during teardown is documented in [`docs/defect-report-001.md`](docs/defect-report-001.md). Root cause: `DaemonBus` captured a raw `StateManager*` in D-Bus callbacks. Fix: `weak_ptr<StateManager>` + `lock()` before every dereference.
