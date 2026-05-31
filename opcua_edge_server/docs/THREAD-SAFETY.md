# Thread Safety

## Shared state

`g_node_values[NUM_NODES]` is `_Atomic float`.  Two threads access it:

| Thread | Operation | Memory order |
|---|---|---|
| timerfd update thread | `atomic_store` | `relaxed` |
| OPC UA callback thread | `atomic_load` | `relaxed` |

`relaxed` ordering is correct for a telemetry display:  no synchronisation
guarantee is needed between the float write and any other memory operation.
The C11 standard guarantees that individual `_Atomic float` load/stores are
indivisible — no partial reads of the 4-byte value.

## Helgrind note

Running `valgrind --tool=helgrind` on the naive build (`data_source.c`) will
report a potential data race on `g_node_values` because Helgrind does not
recognise C11 `_Atomic` as a synchronisation primitive.  This is a known
Helgrind limitation.  The `data_source_atomic.c` variant (not yet merged)
wraps accesses in a spinlock to satisfy Helgrind at the cost of ~3 ns/call.

## Alternative: `memory_order_acquire` / `memory_order_release`

If the server must guarantee that a client reads a value that was computed
*after* a specific external event (e.g., a command was received on D-Bus),
replace the relaxed stores/loads with:

```c
atomic_store_explicit(&g_node_values[idx], val, memory_order_release);
float v = atomic_load_explicit(&g_node_values[idx], memory_order_acquire);
```

This establishes a happens-before relationship but adds a memory fence on
ARM64 (`stlr` / `ldar` instructions) — acceptable overhead for safety-critical paths.
