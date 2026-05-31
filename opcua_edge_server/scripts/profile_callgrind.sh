#!/usr/bin/env bash
# Valgrind callgrind — identify Ir count per cnc_read_callback invocation.

set -euo pipefail

BUILD_DIR="${1:-build}"
SERVER="$BUILD_DIR/cnc_server"

[[ -x "$SERVER" ]] || { echo "Build cnc_server first."; exit 1; }

echo "Running callgrind (1 client, 30 s)..."
valgrind \
  --tool=callgrind \
  --callgrind-out-file=callgrind.naive \
  --instr-atstart=yes \
  "$SERVER" &
VGPID=$!

sleep 3
python3 scripts/benchmark_client.py --duration 30 --output /tmp/cg_latency.csv || true

kill -TERM "$VGPID"
wait "$VGPID"

callgrind_annotate --auto=yes callgrind.naive src/data_source.c \
    | tee docs/CALLGRIND-REPORT.txt

echo ""
echo "Callgrind report → docs/CALLGRIND-REPORT.txt"
echo "Look for Ir count on cnc_read_callback — expected: ~847 Ir/call (naive)."
