# TLS Certificate Authority — Secure Distributed Telemetry Node

## Trust Chain

A single self-signed CA (`ca.crt` / `ca.key`) acts as the root of trust for the
cluster. Every node certificate (`coordinator.crt`, `worker1.crt`, `worker2.crt`)
is signed by that CA. When a coordinator and a worker connect, each side presents
its own certificate and verifies the peer's certificate against `ca.crt`. This
gives **mutual TLS (mTLS)**: neither side accepts an unknown or un-signed
counterpart.

```
ca.crt  (self-signed root)
  ├── coordinator.crt  (signed by CA)
  ├── worker1.crt      (signed by CA)
  └── worker2.crt      (signed by CA)
```

## Certificate Rotation

1. Re-run `gen_certs.sh` in the `certs/` directory — it regenerates all keys and
   certificates in-place (10-year validity by default):
   ```bash
   bash secure_distributed_node/certs/gen_certs.sh
   ```
2. Redeploy or restart every container/service so they pick up the new material.
3. All nodes in the cluster must be rotated together because they share the same
   CA root; mixing old and new certificates across a CA boundary will break
   handshakes.

## Key Management

Private keys (`*.key`) are **never committed to git**. The project `.gitignore`
contains:

```
secure_distributed_node/certs/*.key
secure_distributed_node/tests/test_certs/*.key
```

Only the CA certificate (`ca.crt`) and the node certificates (`*.crt`) are
tracked in version control; they contain only public-key material and are safe
to commit.

## Security Model

This certificate layer extends the existing defence-in-depth posture of the
Secure Distributed Telemetry Node:

| Layer | Mechanism |
|-------|-----------|
| Process isolation | systemd `CapabilityBoundingSet`, `ProtectSystem=strict`, `NoNewPrivileges` |
| IPC policy | D-Bus `<policy>` rules restricting which UIDs may call each method |
| **Network layer** | **mTLS — every gRPC connection requires a CA-signed certificate on both sides** |

The mTLS layer ensures that even if an attacker reaches the gRPC port, no
unauthenticated client can exchange telemetry data or issue control commands.
