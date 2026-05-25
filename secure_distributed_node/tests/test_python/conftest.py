import os
import subprocess
import time
import pytest

# certs live at secure_distributed_node/certs/
CERTS_DIR = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "..", "..", "certs")
)

_node_bin_default = os.path.abspath(
    os.path.join(
        os.path.dirname(__file__), "..", "..", "..",
        "build", "secure_distributed_node", "telemetry_node",
    )
)
NODE_BIN = os.path.abspath(
    os.environ.get("TELEMETRY_NODE_BIN", _node_bin_default)
)

# Ensure the test directory is on sys.path so pb2 imports resolve
import sys
sys.path.insert(0, os.path.dirname(__file__))


@pytest.fixture(scope="session")
def worker_node_stub():
    """Start telemetry_node worker1 on port 50152 with mTLS, yield a gRPC stub."""
    import grpc
    from telemetry_pb2_grpc import TelemetryServiceStub
    from telemetry_pb2 import NodeStatusRequest

    certs = os.path.abspath(CERTS_DIR)
    with open(os.path.join(certs, "ca.crt"),      "rb") as f: ca_cert   = f.read()
    with open(os.path.join(certs, "worker1.crt"), "rb") as f: node_cert = f.read()
    with open(os.path.join(certs, "worker1.key"), "rb") as f: node_key  = f.read()

    if not os.path.exists(NODE_BIN):
        pytest.skip(f"telemetry_node binary not found: {NODE_BIN}")

    proc = subprocess.Popen(
        [NODE_BIN, "worker1", "50152", certs],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    )

    # Poll until gRPC port is ready (same pattern as pydbus conftest)
    channel_creds = grpc.ssl_channel_credentials(
        root_certificates=ca_cert,
        private_key=node_key,
        certificate_chain=node_cert,
    )
    # Override hostname: server cert has CN=worker1, not localhost
    channel = grpc.secure_channel(
        "localhost:50152",
        channel_creds,
        options=[("grpc.ssl_target_name_override", "worker1")],
    )
    stub = TelemetryServiceStub(channel)

    deadline = time.time() + 10.0
    ready = False
    while time.time() < deadline:
        try:
            stub.GetNodeStatus(NodeStatusRequest(node_id="worker1"), timeout=1.0)
            ready = True
            break
        except grpc.RpcError:
            time.sleep(0.3)

    if not ready:
        proc.terminate()
        proc.wait()
        pytest.fail("telemetry_node did not start within 10 seconds")

    yield stub

    proc.terminate()
    proc.wait()
