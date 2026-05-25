# Security Architecture Decision Records

## ADR-001: TLS 1.3 Minimum Protocol Version

**Status:** Accepted

**Context:** The server transmits sensor telemetry over TCP. Transport security is required.

**Decision:** Enforce TLS 1.3 as the minimum protocol version using `SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION)`. TLS 1.2 and below are rejected.

**Consequences:** Follows NIST SP 800-52r2. TLS 1.3 eliminates known TLS 1.2 weaknesses (BEAST, Lucky13, ROBOT) by design. Cipher suite restricted to `TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256`.

---

## ADR-002: JWT Secret in Environment Variable

**Status:** Accepted

**Context:** The HMAC-SHA256 signing secret must be kept out of source control.

**Decision:** Read `JWT_SECRET` from `std::getenv("JWT_SECRET")` at runtime. Never hardcode or log the secret.

**Consequences:** Compatible with Docker secrets, Kubernetes Secrets, and CI/CD secret injection. Prevents accidental commit of credentials to Git.

---

## ADR-003: Error Details Never Sent to Client

**Status:** Accepted

**Context:** JWT validation failures contain internal diagnostic information.

**Decision:** `ValidationResult::error_detail` is logged server-side only. `ERROR` frames sent to clients contain no diagnostic detail.

**Consequences:** Follows OWASP A09:2021. Prevents information leakage to attackers probing the authentication endpoint.

---

## ADR-004: Cipher Suite Restriction

**Status:** Accepted

**Context:** TLS 1.3 supports several cipher suites. Not all are equally strong.

**Decision:** `SSL_CTX_set_ciphersuites` restricts to `TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256`. ChaCha20-Poly1305 provides strong performance on ARM without hardware AES acceleration.

**Consequences:** Excludes `TLS_AES_128_GCM_SHA256` (128-bit key). Negligible compatibility impact since all TLS 1.3 clients support the chosen suites.
