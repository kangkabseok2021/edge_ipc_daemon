#!/usr/bin/env bash
set -euo pipefail

BINARY="edge_ipc_daemon"

echo "Building ARM64 release binary..."
cmake -B build-release -S . \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DSTUB_DBUS=OFF \
    -DBUILD_TESTS=OFF \
    -DCMAKE_POLICY_DEFAULT_CMP0135=NEW
cmake --build build-release --target edge_ipc_daemon -j$(nproc)

cp build-release/edge_ipc_daemon ./$BINARY

echo "Computing SHA-256 checksum..."
sha256sum $BINARY > $BINARY.sha256
cat $BINARY.sha256

echo "Signing with GPG..."
gpg --detach-sign --armor $BINARY

echo ""
echo "Release artifacts:"
echo "  $BINARY         — ARM64 daemon binary"
echo "  $BINARY.sha256  — SHA-256 checksum"
echo "  $BINARY.asc     — GPG detached signature"
echo ""
echo "Verify with:"
echo "  gpg --verify $BINARY.asc $BINARY"
