# Secure Distributed Telemetry Node — Distribution Design

## Node Topology

```
coordinator ──mTLS──▶ worker1:50051  (TelemetryNode + existing pipeline)
            ──mTLS──▶ worker2:50051  (TelemetryNode + existing pipeline)
```

## Component Reuse

| Component | Origin | Role in SDN |
|-----------|--------|-------------|
| `ITelemetrySource` / `SimulatedSource` | edge_ipc_daemon | Data generation (unchanged) |
| `MovingAverageFilter` / `ThresholdDetector` | edge_ipc_daemon | Signal processing (unchanged) |
| `DataProcessor` | edge_ipc_daemon | Filter chain orchestration (unchanged) |
| `TelemetryReceiver` (SPSC) | edge_ipc_daemon | Lock-free inter-thread queue (unchanged) |
| `StateManager` FSM | edge_ipc_daemon | IDLE/RUNNING/FAULT/SHUTDOWN lifecycle (unchanged) |
| `DaemonBus` | edge_ipc_daemon | Local D-Bus IPC — kept alongside gRPC |
| CMake ARM64 toolchain | edge_ipc_daemon | Cross-compile (unchanged) |
| Python pytest subprocess fixture | edge_ipc_daemon | Test pattern (extended for gRPC) |

New in SDN: `TelemetryNode` (gRPC server), `TelemetryCoordinator` (mTLS client), `TlsCredentials`, docker-compose cluster.

## Security Layers

1. **D-Bus security policy** — local process isolation (inherited from edge_ipc_daemon)
2. **systemd `CapabilityBoundingSet`** — minimal privileges (inherited)
3. **Mutual TLS (gRPC)** — cross-node authentication; coordinator and workers verify each other's certificates

## gRPC Service Design

Two RPC patterns demonstrated:

- **`StreamTelemetry(NodeStatusRequest) → stream TelemetrySample`** — server-streaming; worker pushes continuous telemetry to any connected coordinator
- **`GetNodeStatus(NodeStatusRequest) → NodeStatus`** — unary; coordinator polls for health aggregation every 500 ms

REST (`GET /health`) is for monitoring only — not suitable for real-time control (HTTP overhead ~ms vs gRPC ~µs).

## Certificate Chain

```
CA (TelemetryCA)
├── coordinator.crt  (used by TelemetryCoordinator)
├── worker1.crt      (used by TelemetryNode worker1)
└── worker2.crt      (used by TelemetryNode worker2)
```

`gen_certs.sh` generates all certificates. Private keys are never committed to git.
Rotation: rerun `gen_certs.sh`, redeploy containers.
