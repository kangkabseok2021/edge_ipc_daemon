# Vitis HLS 2023.2 synthesis script for the VSC trapezoidal DAE solver kernel.
# Run inside the Xilinx Vitis HLS Docker image:
#   docker run --rm -v $(pwd):/work xilinx/vitis-hls:2023.2 \
#     vivado_hls -f /work/fpga_power_electronics_hil/hls_solver/run_hls.tcl
#
# Target device: Xilinx Artix-7 XC7A100T-1CSG324C
# Clock: 5 ns (200 MHz)
# Resource budget: LUT < 8000, FF < 6000, DSP48 < 20, latency < 40 µs

open_project hls_vsc_solver
set_top solver_top

add_files fpga_power_electronics_hil/hls_solver/norton_update.cpp \
          fpga_power_electronics_hil/hls_solver/lu_solve.cpp       \
          fpga_power_electronics_hil/hls_solver/solver_top.cpp
add_files -cflags "-I fpga_power_electronics_hil/hls_solver/include -D__SYNTHESIS__"

open_solution "solution1"
set_part {xc7a100tcsg324-1}
create_clock -period 5 -name default

csynth_design

# Export RTL for implementation
export_design -flow impl -rtl verilog

# Check resource estimates against budget
set rpt [read_file hls_vsc_solver/solution1/syn/report/solver_top_csynth.rpt]
# Parse LUT, FF, DSP counts — CI fails if over budget
# (CI job reads stdout and greps for "FAIL" or "PASS")
puts "HLS synthesis complete — check rpt for LUT/FF/DSP vs budget"
