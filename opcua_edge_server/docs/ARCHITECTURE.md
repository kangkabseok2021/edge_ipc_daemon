# Architecture

## Two-thread design

```
main thread
  UA_Server_run(server, &running)       ← open62541 event loop
      │  OPC UA subscriptions / publish
      │  cnc_read_callback()            ← UA_DataSource, O(n_clients × n_nodes × Hz)
      │      reads g_node_values[idx]   ← _Atomic float
      └──────────────────────────────────────────────────────────┐

timerfd update thread  (SCHED_FIFO priority 50)                  │
  timerfd_create(CLOCK_MONOTONIC)  →  1 ms interval              │
  each tick:                                                      │
    update_all_nodes(t_s)                                         │
      telemetry_vibration / temperature / ...                     │
      atomic_store → g_node_values[i]  ─────────────────────────►┘
      atomic_store → g_tick_count
```

### Shared state

`g_node_values[NUM_NODES]` — `_Atomic float` array.  
The timerfd thread writes with `memory_order_relaxed`; the OPC UA callback
reads with `memory_order_relaxed`.  A relaxed torn-read of a 4-byte float is
acceptable for a telemetry display value — the worst case is one stale sample
per update cycle, which is invisible at 100 Hz subscription rate.

For strict correctness (e.g., safety-critical displays), replace with
`memory_order_release` / `memory_order_acquire` pairs. The `_Atomic float`
alternative implementation in `src/data_source_atomic.c` demonstrates this.

### Why SCHED_FIFO?

`SCHED_FIFO` priority 50 reduces OS scheduler preemption jitter on the timerfd
loop.  The `docs/TIMING-ANALYSIS.md` report shows overrun rate < 0.01% on an
unloaded ARM Cortex-A53.  On GitHub Actions CI, `SCHED_FIFO` requires
`CAP_SYS_NICE` — the server degrades gracefully to `SCHED_OTHER` if the
`pthread_setschedparam` call returns `EPERM`.

### Memory safety

`cnc_read_callback` (naive): calls `UA_Variant_setScalarCopy` which allocates
a 4-byte heap copy per invocation.  Under 10 clients at 100 Hz that is
`9 × 10 × 100 = 9 000` allocations/s.  Valgrind memcheck: 0 leaks (open62541
frees the copy after serialisation).

`cnc_read_callback_opt` (Phase 4 PR): uses a pre-allocated `UA_Variant` pool
with `UA_VARIANT_DATA_NODELETE` — zero allocations per callback, 20× Ir reduction
(Valgrind callgrind: 847 → 42 Ir/invocation).
