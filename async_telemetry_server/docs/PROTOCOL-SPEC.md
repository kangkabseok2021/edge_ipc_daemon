# ATS Binary Frame Protocol Specification

## Frame Layout

```
Offset  Size  Field         Description
------  ----  -----------   ------------------------------------------
0       4     magic         0x4E455854 ("NEXT") — identifies ATS frames
4       1     version       Protocol version. Currently 0x01.
5       1     msg_type      AUTH=0x01, TELEMETRY=0x02, ACK=0x03, ERROR=0x04
6       4     payload_len   Big-endian uint32. Byte count of payload.
10      2     checksum      CRC-16/CCITT of payload (big-endian). 0xFFFF for empty payload.
12      N     payload       Application data (N = payload_len bytes)
```

## Message Types

| Type      | Hex  | Sender | Payload |
|-----------|------|--------|---------|
| AUTH      | 0x01 | Client | JWT Bearer token (UTF-8 string, no null terminator) |
| TELEMETRY | 0x02 | Client | `SensorFrame`: u32 sensor_id + u64 ts_us + f64 value (20 bytes, host byte-order) |
| ACK       | 0x03 | Server | Empty (payload_len = 0) |
| ERROR     | 0x04 | Server | Optional UTF-8 error description (never sent to clients — see ADR-003) |

## CRC-16/CCITT

- Polynomial: 0x1021
- Initial value: 0xFFFF
- No final XOR, no reflection (CCITT variant)
- Test vector: `crc16("123456789") == 0x29B1`
- Empty payload: `crc16("") == 0xFFFF` (initial value returned unchanged)

## Session Flow

```
Client                              Server
  │                                    │
  │── AUTH (JWT Bearer token) ────────►│
  │◄── ACK (authenticated) ───────────│  or ERROR (invalid token — no detail)
  │── TELEMETRY (SensorFrame) ────────►│
  │◄── ACK ───────────────────────────│
  │── TELEMETRY ──────────────────────►│
  │◄── ACK ───────────────────────────│
  ╌╌╌ (60 frames at 1 Hz typical) ╌╌╌
  │── TCP FIN ────────────────────────►│
```

## SensorFrame Wire Format (20 bytes)

| Offset | Size | Field        | Type   |
|--------|------|--------------|--------|
| 0      | 4    | sensor_id    | uint32 |
| 4      | 8    | timestamp_us | uint64 |
| 12     | 8    | value        | double |

Fields are in host byte-order (not network byte-order) — client and server run on the same architecture in the reference implementation.
