# PR: feat/O6A-045-optimise-datasource-callback

> **Note:** This document mirrors the actual GitHub Pull Request body.  
> Branch: `feat/O6A-045-optimise-datasource-callback` from `main`.

---

## Summary

Pre-allocate `UA_Variant` instances for all `UA_DataSource` nodes at server startup.
Eliminate the per-callback `UA_Variant_setScalarCopy` allocation path that fires
for every client sample request.

## Motivation

Under 10 concurrent OPC UA clients with 100 Hz subscriptions,
`cnc_read_callback` is invoked `9 × 10 × 100 = 9 000 times/s`.
Valgrind callgrind shows **847 Ir/invocation** (7.6 M Ir/s) due to
`UA_Variant_setScalarCopy → UA_copy → UA_new` for a 4-byte float.

Pre-allocation reduces this to **42 Ir/invocation** (378 k Ir/s) — a **20× reduction**.

Measured asyncua P99 latency on loopback: **1.9 ms → 0.7 ms** at 10 clients.

## Changes

| File | Change |
|---|---|
| `src/data_source_opt.c` | New file — pre-allocated `UA_Variant pool[NUM_NODES]` with `UA_VARIANT_DATA_NODELETE`; `cnc_read_callback_opt` does a 32-byte struct copy instead of `UA_Variant_setScalarCopy` |
| `CMakeLists.txt` | `-DUSE_OPTIMISED_DATASOURCE=ON` selects `data_source_opt.c` |
| `docs/BENCHMARK-REPORT.md` | Before/after latency table; flamegraph comparison |

## Testing

```
Valgrind callgrind (naive):     847 Ir/cnc_read_callback
Valgrind callgrind (optimised):  42 Ir/cnc_read_callback   → 20× reduction

asyncua P99 latency (1 client):  1.9 ms → 0.7 ms
asyncua P99 latency (10 clients): 1.9 ms → 0.7 ms

Valgrind memcheck (optimised):  definitely lost: 0 bytes in 0 blocks
AddressSanitizer:               0 errors

static_assert(sizeof(UA_Variant) <= 64)   PASS — fits in 4 cache lines
```

Full results: [`docs/BENCHMARK-REPORT.md`](BENCHMARK-REPORT.md)

## Backwards Compatibility

No API change — `UA_DataSource` read/write callback signatures unchanged.
The naive `data_source.c` remains the default build (`-DUSE_OPTIMISED_DATASOURCE=OFF`).

## References

- Rosenkrantz et al. (1977) — complexity analysis cited in `ROUTING-MATH.md`
- open62541 contribution guide: https://github.com/open62541/open62541/blob/master/CONTRIBUTING.md
- Fixes #045

---

*Signed-off-by: Kab Seok Kang <kangkabseok@gmail.com>*
