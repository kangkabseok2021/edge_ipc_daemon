FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build pkg-config \
    libssl-dev libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
    nlohmann-json3-dev libsystemd-dev \
    && rm -rf /var/lib/apt/lists/*

# Install cpp-httplib (header-only)
RUN apt-get update && apt-get install -y --no-install-recommends \
    libcpp-httplib-dev 2>/dev/null || true
# Fallback: download httplib.h directly
RUN if [ ! -f /usr/include/httplib.h ] && [ ! -f /usr/local/include/httplib.h ]; then \
    apt-get install -y curl && \
    curl -sSL https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.15.3/httplib.h \
         -o /usr/local/include/httplib.h; fi

WORKDIR /src
COPY . .
RUN cmake -B build/release -S secure_distributed_node \
          -DCMAKE_BUILD_TYPE=Release \
          -DSTUB_DBUS=ON \
          -DBUILD_TESTS=OFF \
    && cmake --build build/release \
             --target telemetry_node telemetry_coordinator -j$(nproc)

FROM ubuntu:22.04 AS runtime
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    libgrpc++1.46 libprotobuf23 libssl3 libsystemd0 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -r -u 1001 -s /bin/false telemetry
COPY --from=builder /src/build/release/telemetry_node       /usr/local/bin/
COPY --from=builder /src/build/release/telemetry_coordinator /usr/local/bin/
USER telemetry
