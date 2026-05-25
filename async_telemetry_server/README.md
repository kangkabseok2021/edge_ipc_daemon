# Async Telemetry Server

A C++20 TCP server for secure, authenticated, and persistent telemetry ingestion.

**Stack:** Boost.Asio C++20 coroutines · JWT (jwt-cpp v0.7) · TLS 1.3 (OpenSSL 3) · libpqxx · PostgreSQL 16 · GoogleTest · Docker · GitHub Actions

## Architecture

```
TelemetryClient ──TLS 1.3──► TlsServer (co_await accept)
                                  │
                                  ├── JwtValidator (HMAC-SHA256)
                                  │
                                  └── TelemetryRepository ──► DbPool ──► PostgreSQL 16
```

## Build (macOS)

```bash
brew install boost libpqxx llvm
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
cd async_telemetry_server
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.ncpu)
```

## Run Tests

```bash
cd build && ctest --output-on-failure
```

**14/14** GoogleTests pass (9 FrameProtocol/CRC-16 + 5 JwtValidator).

## Python DB Tests

```bash
export PATH="/opt/homebrew/opt/postgresql@14/bin:$PATH"
cd tests/python
pip install pytest pytest-postgresql psycopg2-binary
pytest -v
```

**5/5** pytest tests pass (schema, indexes, CHECK constraint).

## Run with Docker Compose

```bash
# Generate TLS certificates
./scripts/gen_certs.sh certs

# Start all services
JWT_SECRET=your_secret docker compose -f docker/docker-compose.yml up

# Obtain a JWT token
curl -s -X POST http://localhost:8000/token \
    -H 'Content-Type: application/json' \
    -d '{"client_id":"test_client","client_secret":"test_pass"}' | jq .

# Send telemetry (plain TCP)
./build/telemetry_client localhost 8443 <jwt_token>
```

## CI

Five GitHub Actions jobs (`.github/workflows/async-telemetry-ci.yml`):

| Job | Checks |
|-----|--------|
| `build` | Release build, 0 warnings, 14/14 tests pass |
| `asan` | AddressSanitizer + UBSan — 0 memory errors |
| `clang-tidy` | modernize-*, bugprone-*, cert-*, cppcoreguidelines-* |
| `python-tests` | pytest-postgresql — 5/5 DB integration tests |
| `docker-build` | Multi-stage image builds, compose config validates |

## Protocol

See [`docs/PROTOCOL-SPEC.md`](docs/PROTOCOL-SPEC.md).

## Security

See [`docs/SECURITY.md`](docs/SECURITY.md) for ADRs covering TLS 1.3, JWT secret management, and error information leakage.
