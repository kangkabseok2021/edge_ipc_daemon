#!/usr/bin/env bash
set -euo pipefail

CERT_DIR="${1:-certs}"
mkdir -p "$CERT_DIR"

echo "[gen_certs] Generating self-signed TLS cert in $CERT_DIR/"

openssl req -x509 -newkey rsa:4096 -sha256 -days 3650 \
    -nodes \
    -keyout "$CERT_DIR/server.key" \
    -out    "$CERT_DIR/server.crt" \
    -subj "/C=DE/O=ATS Test/CN=localhost" \
    -addext "subjectAltName=DNS:localhost,IP:127.0.0.1"

chmod 600 "$CERT_DIR/server.key"
echo "[gen_certs] Done: $CERT_DIR/server.crt  $CERT_DIR/server.key"
