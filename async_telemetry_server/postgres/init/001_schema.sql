CREATE TABLE IF NOT EXISTS telemetry (
    id             BIGSERIAL    PRIMARY KEY,
    sensor_id      INT          NOT NULL,
    ts             TIMESTAMPTZ  NOT NULL DEFAULT NOW(),
    value          DOUBLE PRECISION NOT NULL,
    client_subject TEXT         NOT NULL
);

CREATE TABLE IF NOT EXISTS auth_log (
    id             BIGSERIAL    PRIMARY KEY,
    ts             TIMESTAMPTZ  NOT NULL DEFAULT NOW(),
    client_subject TEXT         NOT NULL,
    event          TEXT         NOT NULL
                   CHECK (event IN ('AUTH_OK','AUTH_FAIL_SIG',
                                    'AUTH_FAIL_EXP','AUTH_FAIL_CLAIMS')),
    ip_addr        INET
);

CREATE INDEX IF NOT EXISTS idx_telemetry_ts_brin
    ON telemetry USING BRIN (ts);

CREATE INDEX IF NOT EXISTS idx_auth_fail
    ON auth_log (client_subject)
    WHERE event != 'AUTH_OK';
