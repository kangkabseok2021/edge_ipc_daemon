#!/usr/bin/env bash
# Record a perf CPU profile of cnc_server under 10-client load for 60 seconds.
# Produces docs/PERF-REPORT.txt and docs/images/flamegraph_naive.svg

set -euo pipefail

BUILD_DIR="${1:-build}"
SERVER="$BUILD_DIR/cnc_server"
FLAMEGRAPH_DIR="${FLAMEGRAPH_DIR:-$HOME/FlameGraph}"

[[ -x "$SERVER" ]] || { echo "Build cnc_server first."; exit 1; }

mkdir -p docs/images

echo "Profiling for 60 seconds (10 clients at 100 Hz)..."
perf record -g -F 99 -o perf.data "$SERVER" &
SPID=$!

sleep 3
python3 scripts/benchmark_client.py --duration 60 --clients 10 --output /tmp/load.csv || true

kill -TERM "$SPID"
wait "$SPID"

perf report --stdio --percent-limit 1 -i perf.data > docs/PERF-REPORT.txt
echo "Perf report → docs/PERF-REPORT.txt"

if [[ -f "$FLAMEGRAPH_DIR/stackcollapse-perf.pl" ]]; then
    perf script -i perf.data | \
        "$FLAMEGRAPH_DIR/stackcollapse-perf.pl" | \
        "$FLAMEGRAPH_DIR/flamegraph.pl" > docs/images/flamegraph_naive.svg
    echo "Flamegraph → docs/images/flamegraph_naive.svg"
else
    echo "FlameGraph not found at $FLAMEGRAPH_DIR — skipping SVG generation."
    echo "Clone: git clone https://github.com/brendangregg/FlameGraph $FLAMEGRAPH_DIR"
fi
