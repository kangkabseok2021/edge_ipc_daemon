#!/usr/bin/env bash
# Run Valgrind memcheck on cnc_server for 30 seconds under Python client load.
# Expected: "definitely lost: 0 bytes in 0 blocks"

set -euo pipefail

BUILD_DIR="${1:-build}"
SERVER="$BUILD_DIR/cnc_server"

[[ -x "$SERVER" ]] || { echo "Build first: cmake -B $BUILD_DIR && cmake --build $BUILD_DIR"; exit 1; }

echo "Starting server under Valgrind memcheck..."
valgrind \
  --tool=memcheck \
  --leak-check=full \
  --show-leak-kinds=all \
  --error-exitcode=1 \
  "$SERVER" &
VGPID=$!

sleep 3  # wait for server to start

echo "Running asyncua benchmark client for 30 seconds..."
python3 scripts/benchmark_client.py --duration 30 --output /tmp/memcheck_latency.csv || true

echo "Sending SIGTERM to Valgrind..."
kill -TERM "$VGPID"
wait "$VGPID"
echo "Memcheck complete."
