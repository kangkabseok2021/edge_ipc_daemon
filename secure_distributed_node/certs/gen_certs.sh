#!/usr/bin/env bash
set -euo pipefail
CERTS_DIR="$(cd "$(dirname "$0")" && pwd)"
DAYS=3650

# Self-signed CA
openssl req -new -x509 -days $DAYS -nodes \
    -keyout "$CERTS_DIR/ca.key" -out "$CERTS_DIR/ca.crt" \
    -subj "/CN=TelemetryCA/O=SecureDistributedNode"

gen_node_cert() {
    local NAME=$1
    openssl req -new -nodes -keyout "$CERTS_DIR/$NAME.key" -out "$CERTS_DIR/$NAME.csr" \
        -subj "/CN=$NAME/O=SecureDistributedNode"
    openssl x509 -req -days $DAYS -in "$CERTS_DIR/$NAME.csr" \
        -CA "$CERTS_DIR/ca.crt" -CAkey "$CERTS_DIR/ca.key" -CAcreateserial \
        -out "$CERTS_DIR/$NAME.crt"
    rm "$CERTS_DIR/$NAME.csr"
}

for NODE in coordinator worker1 worker2; do
    gen_node_cert "$NODE"
done
echo "Generated: ca.crt, coordinator.{crt,key}, worker1.{crt,key}, worker2.{crt,key}"
