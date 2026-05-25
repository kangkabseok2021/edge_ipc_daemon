# ── Stage 1: builder ──────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS builder

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build git ca-certificates \
    libboost-dev \
    libssl-dev libpqxx-dev libpq-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
RUN cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTS=OFF \
        -DENABLE_ASAN=OFF \
    && cmake --build build --target telemetry_server -j "$(nproc)"

# ── Stage 2: runtime ──────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS runtime

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 libpq5 libpqxx-7.7 \
    curl \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/telemetry_server /usr/local/bin/telemetry_server

USER nobody:nogroup

EXPOSE 8443

HEALTHCHECK --interval=10s --timeout=5s --start-period=15s --retries=3 \
    CMD curl -sf --insecure https://localhost:8443/health || exit 1

ENTRYPOINT ["telemetry_server"]
