#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <cstddef>
#include <stdexcept>

namespace ats {

enum class MsgType : uint8_t {
    AUTH      = 0x01,
    TELEMETRY = 0x02,
    ACK       = 0x03,
    ERROR     = 0x04,
};

// 12-byte frame header (big-endian on wire)
#pragma pack(push, 1)
struct FrameHeader {
    uint8_t  magic[4];       // 0x4E455854 = "NEXT"
    uint8_t  version;        // 0x01
    uint8_t  msg_type;       // MsgType
    uint32_t payload_len;    // big-endian
    uint16_t checksum;       // CRC-16/CCITT of payload, big-endian
};
#pragma pack(pop)
static_assert(sizeof(FrameHeader) == 12);

static constexpr uint8_t MAGIC[4]  = {0x4E, 0x45, 0x58, 0x54};
static constexpr uint8_t VERSION   = 0x01;

struct Frame {
    MsgType              msg_type{MsgType::AUTH};
    std::vector<std::byte> payload;

    // Returns the 12-byte header followed by the payload bytes.
    [[nodiscard]] std::vector<std::byte> serialize() const;

    // Parses a Frame from a flat byte buffer (header + payload).
    // Throws std::invalid_argument on bad magic, version, or CRC.
    [[nodiscard]] static Frame deserialize(const std::vector<std::byte>& buf);
};

// CRC-16/CCITT (polynomial 0x1021, init 0xFFFF)
[[nodiscard]] uint16_t crc16(const std::vector<std::byte>& data) noexcept;

} // namespace ats
