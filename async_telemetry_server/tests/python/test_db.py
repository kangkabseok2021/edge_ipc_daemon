"""PostgreSQL integration tests for the ATS schema.

Uses pytest-postgresql to spin up an ephemeral PostgreSQL instance
and apply 001_schema.sql before each test.
"""
import pathlib
import pytest
try:
    import psycopg.errors as _pg_errors          # psycopg v3 (pytest-postgresql >=6)
    _CheckViolation = _pg_errors.CheckViolation
except ImportError:
    import psycopg2.errors as _pg_errors         # psycopg v2 fallback
    _CheckViolation = _pg_errors.CheckViolation

SCHEMA_SQL = (
    pathlib.Path(__file__).parent.parent.parent
    / "postgres" / "init" / "001_schema.sql"
).read_text()


@pytest.fixture
def db(postgresql):
    """Apply schema and yield a psycopg2 connection."""
    with postgresql.cursor() as cur:
        cur.execute(SCHEMA_SQL)
    postgresql.commit()
    yield postgresql


def test_telemetry_insert(db):
    with db.cursor() as cur:
        cur.execute(
            "INSERT INTO telemetry(sensor_id, ts, value, client_subject)"
            " VALUES (%s, NOW(), %s, %s)",
            (1, 42.5, "client_a"),
        )
        db.commit()
        cur.execute("SELECT sensor_id, value, client_subject FROM telemetry")
        row = cur.fetchone()
    assert row == (1, 42.5, "client_a")


def test_auth_log_insert(db):
    with db.cursor() as cur:
        cur.execute(
            "INSERT INTO auth_log(client_subject, event, ip_addr)"
            " VALUES (%s, %s, %s::inet)",
            ("client_a", "AUTH_OK", "127.0.0.1"),
        )
        cur.execute(
            "INSERT INTO auth_log(client_subject, event, ip_addr)"
            " VALUES (%s, %s, %s::inet)",
            ("client_b", "AUTH_FAIL_SIG", "10.0.0.1"),
        )
        db.commit()
        cur.execute("SELECT COUNT(*) FROM auth_log")
        count = cur.fetchone()[0]
    assert count == 2


def test_auth_log_check_constraint(db):
    with db.cursor() as cur:
        cur.execute("SAVEPOINT sp1")
        try:
            cur.execute(
                "INSERT INTO auth_log(client_subject, event, ip_addr)"
                " VALUES (%s, %s, %s::inet)",
                ("client_c", "INVALID_EVENT", "1.2.3.4"),
            )
            cur.execute("RELEASE SAVEPOINT sp1")
            assert False, "expected CheckViolation"
        except _CheckViolation:
            cur.execute("ROLLBACK TO SAVEPOINT sp1")


def test_brin_index_exists(db):
    with db.cursor() as cur:
        cur.execute(
            "SELECT indexname FROM pg_indexes"
            " WHERE tablename = 'telemetry'"
            "   AND indexname = 'idx_telemetry_ts_brin'"
        )
        row = cur.fetchone()
    assert row is not None, "BRIN index idx_telemetry_ts_brin not found"


def test_partial_index_exists(db):
    with db.cursor() as cur:
        cur.execute(
            "SELECT indexname FROM pg_indexes"
            " WHERE tablename = 'auth_log'"
            "   AND indexname = 'idx_auth_fail'"
        )
        row = cur.fetchone()
    assert row is not None, "Partial index idx_auth_fail not found"
