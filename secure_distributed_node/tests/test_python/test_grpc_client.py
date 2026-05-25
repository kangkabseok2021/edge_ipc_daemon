import itertools
import os
import subprocess
import tempfile
import time
import pytest
import grpc

from telemetry_pb2      import NodeStatusRequest, TelemetrySample
from telemetry_pb2_grpc import TelemetryServiceStub


def test_get_node_status_returns_fsm_state(worker_node_stub):
    """GetNodeStatus returns a valid FSM state string."""
    status = worker_node_stub.GetNodeStatus(
        NodeStatusRequest(node_id="worker1"), timeout=5.0
    )
    assert status.fsm_state in ("IDLE", "RUNNING", "FAULT", "SHUTDOWN")


def test_node_status_processed_count_non_negative(worker_node_stub):
    """ProcessedCount is a non-negative uint64."""
    s = worker_node_stub.GetNodeStatus(
        NodeStatusRequest(node_id="worker1"), timeout=5.0
    )
    assert s.processed_count >= 0


def test_stream_telemetry_delivers_samples(worker_node_stub):
    """StreamTelemetry server-stream delivers at least 3 samples with correct node_id."""
    req = NodeStatusRequest(node_id="worker1")
    reader = worker_node_stub.StreamTelemetry(req)
    samples = list(itertools.islice(reader, 3))
    reader.cancel()
    assert len(samples) == 3
    for s in samples:
        assert s.node_id == "worker1"
        assert s.fsm_state in ("IDLE", "RUNNING", "FAULT", "SHUTDOWN")


def test_mtls_rejected_without_client_cert():
    """Insecure channel to mTLS server must be rejected."""
    bad_channel = grpc.insecure_channel("localhost:50152")
    bad_stub = TelemetryServiceStub(bad_channel)
    with pytest.raises(grpc.RpcError) as exc_info:
        bad_stub.GetNodeStatus(NodeStatusRequest(), timeout=3.0)
    # Server requires mTLS — plain TCP must be refused
    assert exc_info.value.code() in (
        grpc.StatusCode.UNAVAILABLE,
        grpc.StatusCode.UNAUTHENTICATED,
        grpc.StatusCode.UNKNOWN,
    )





def test_wrong_cert_rejected():
    """Self-signed cert NOT signed by the CA must be rejected."""
    with tempfile.TemporaryDirectory() as tmpdir:
        subprocess.run(
            [
                "openssl", "req", "-new", "-x509", "-days", "1", "-nodes",
                "-keyout", f"{tmpdir}/bad.key",
                "-out",    f"{tmpdir}/bad.crt",
                "-subj",   "/CN=attacker/O=BadActor",
            ],
            check=True,
            capture_output=True,
        )
        bad_ca   = open(f"{tmpdir}/bad.crt", "rb").read()
        bad_cert = open(f"{tmpdir}/bad.crt", "rb").read()
        bad_key  = open(f"{tmpdir}/bad.key", "rb").read()

        bad_creds = grpc.ssl_channel_credentials(
            root_certificates=bad_ca,
            private_key=bad_key,
            certificate_chain=bad_cert,
        )
        bad_channel = grpc.secure_channel(
            "localhost:50152",
            bad_creds,
            options=[("grpc.ssl_target_name_override", "worker1")],
        )
        bad_stub = TelemetryServiceStub(bad_channel)
        with pytest.raises(grpc.RpcError):
            bad_stub.GetNodeStatus(NodeStatusRequest(), timeout=3.0)
