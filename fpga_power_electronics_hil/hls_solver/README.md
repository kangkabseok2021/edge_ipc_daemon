# HLS Solver — Vitis HLS Kernel

These C++ files implement the FPGA trapezoidal DAE solver kernel using Vitis HLS 2023.2.

## Files

| File | Role |
|---|---|
| `include/hls_compat.h` | Maps `ap_fixed<32,16>` → `double` on host; HLS pragmas → noop |
| `include/solver_types.h` | `axis_gate_t`, `axis_state_t`, `FixedParams` |
| `norton_update.cpp` | O(N) Norton injection vector + history update |
| `lu_solve.cpp` | Forward/back substitution on pre-factored 9×9 BRAM matrices |
| `solver_top.cpp` | AXI4-Stream top-level with DATAFLOW pipeline |
| `run_hls.tcl` | Synthesis Tcl script (Artix-7 XC7A100T, 200 MHz) |

## CI Note

These files are **not compiled in CI** — CI tests the equivalent double-precision
C++ in `../vsc_solver/`. To synthesise:

```bash
docker run --rm -v $(pwd):/work xilinx/vitis-hls:2023.2 \
  vivado_hls -f /work/fpga_power_electronics_hil/hls_solver/run_hls.tcl
```

## Design Decisions

- `ap_fixed<32,16>`: 16 integer bits (covers ±32767 V/A) × 16 fractional bits (15 µV resolution)
- LU pre-factored once at init; only Norton history update (O(N)) runs each 50 µs step
- Snubbers (C_snub = 0.1 µF, τ≈1 ns ≪ dt = 50 µs) omitted via quasi-static limit C→0
- `#pragma HLS PIPELINE II=1` on forward/back substitution loops → ~405 ns latency at 200 MHz
- `#pragma HLS ARRAY_PARTITION complete` on 9-element vectors enables parallel register access
