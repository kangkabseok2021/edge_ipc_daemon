#!/usr/bin/env python3
"""asyncua benchmark client — measures OPC UA round-trip latency under load.

Usage:
    python scripts/benchmark_client.py [--url opc.tcp://localhost:4840] \
                                        [--duration 600] [--output data/latency_log.csv]
"""

import argparse
import asyncio
import csv
import time
from pathlib import Path

from asyncua import Client

CNC_NS = 2
NODE_IDS = [1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009]
SAMPLE_INTERVAL_MS = 10   # 100 Hz subscription


class LatencyHandler:
    def __init__(self, queue: asyncio.Queue):
        self._queue = queue

    def datachange_notification(self, node, val, data):
        recv_ns = time.time_ns()
        src_ts  = data.monitored_item.Value.SourceTimestamp
        if src_ts is not None:
            src_ms = src_ts.timestamp() * 1000.0
            rtt_ms = recv_ns / 1e6 - src_ms
            node_str = str(node.nodeid)
            self._queue.put_nowait((node_str, rtt_ms, src_ms))


async def run_subscriber(url: str, duration_s: float, queue: asyncio.Queue):
    async with Client(url=url) as client:
        handler = LatencyHandler(queue)
        sub = await client.create_subscription(SAMPLE_INTERVAL_MS, handler)
        nodes = [client.get_node(f"ns={CNC_NS};i={nid}") for nid in NODE_IDS]
        await sub.subscribe_data_change(nodes)
        await asyncio.sleep(duration_s)
        await sub.unsubscribe(nodes)
        await sub.delete()


async def drain_queue(queue: asyncio.Queue, writer: csv.writer, done_event: asyncio.Event):
    while not done_event.is_set() or not queue.empty():
        try:
            node_id, rtt_ms, src_ms = queue.get_nowait()
            writer.writerow([node_id, f"{rtt_ms:.3f}", f"{src_ms:.3f}"])
        except asyncio.QueueEmpty:
            await asyncio.sleep(0.01)


async def main(url: str, duration_s: float, output: str, n_clients: int):
    Path(output).parent.mkdir(parents=True, exist_ok=True)
    queue: asyncio.Queue = asyncio.Queue()
    done  = asyncio.Event()

    with open(output, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["node_id", "latency_ms", "server_ts_ms"])

        drain_task = asyncio.create_task(drain_queue(queue, writer, done))
        sub_tasks  = [
            asyncio.create_task(run_subscriber(url, duration_s, queue))
            for _ in range(n_clients)
        ]
        await asyncio.gather(*sub_tasks)
        done.set()
        await drain_task

    print(f"Wrote latency log → {output}")


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--url",      default="opc.tcp://localhost:4840")
    p.add_argument("--duration", type=float, default=600.0,
                   help="Test duration in seconds (default: 600 = 10 min)")
    p.add_argument("--output",   default="data/latency_log.csv")
    p.add_argument("--clients",  type=int, default=1,
                   help="Number of parallel subscribers (use 10 for load test)")
    args = p.parse_args()
    asyncio.run(main(args.url, args.duration, args.output, args.clients))
