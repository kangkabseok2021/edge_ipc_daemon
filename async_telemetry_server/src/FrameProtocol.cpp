#include "FrameProtocol.h"
#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>

namespace ats {

// CRC-16/CCITT — poly 0x1021, init 0xFFFF, no final XOR, no reflection
uint16_t crc16(const std::vector<std::byte>& data) noexcept {
    uint16_t crc = 0xFFFF;
    for (auto b : data) {
        crc ^= static_cast<uint16_t>(static_cast<uint8_t>(b)) << 8;
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x8000u)
                crc = static_cast<uint16_t>((static_cast<uint32_t>(crc) << 1u) ^ 0x1021u);
            else
                crc = static_cast<uint16_t>(static_cast<uint32_t>(crc) << 1u);
        }
    }
    return crc;
}

std::vector<std::byte> Frame::serialize() const {
    uint32_t plen    = static_cast<uint32_t>(payload.size());
    uint16_t chk     = crc16(payload);
    uint32_t plen_be = htonl(plen);
    uint16_t chk_be  = htons(chk);

    std::vector<std::byte> buf(12 + plen);
    size_t off = 0;

    // Magic
    for (auto m : MAGIC) buf[off++] = std::byte{m};
    // Version
    buf[off++] = std::byte{VERSION};
    // MsgType
    buf[off++] = std::byte{static_cast<uint8_t>(msg_type)};
    // payload_len (4B big-endian)
    std::memcpy(buf.data() + off, &plen_be, 4); off += 4;
    // checksum (2B big-endian)
    std::memcpy(buf.data() + off, &chk_be,  2); off += 2;
    // payload
    if (plen > 0)
        std::memcpy(buf.data() + off, payload.data(), plen);

    return buf;
}

Frame Frame::deserialize(const std::vector<std::byte>& buf) {
    if (buf.size() < 12)
        throw std::invalid_argument("buffer too small");

    // Magic check
    for (int i = 0; i < 4; ++i) {
        if (static_cast<uint8_t>(buf[static_cast<size_t>(i)]) != MAGIC[i])
            throw std::invalid_argument("bad magic");
    }
    // Version check
    if (static_cast<uint8_t>(buf[4]) != VERSION)
        throw std::invalid_argument("unsupported version");

    Frame f;
    f.msg_type = static_cast<MsgType>(static_cast<uint8_t>(buf[5]));

    uint32_t plen_be{};
    std::memcpy(&plen_be, buf.data() + 6, 4);
    uint32_t plen = ntohl(plen_be);

    uint16_t chk_be{};
    std::memcpy(&chk_be, buf.data() + 10, 2);
    uint16_t expected_crc = ntohs(chk_be);

    if (buf.size() < static_cast<size_t>(12) + plen)
        throw std::invalid_argument("buffer too short for payload");

    f.payload.assign(buf.begin() + 12,
                     buf.begin() + 12 + static_cast<std::ptrdiff_t>(plen));

    uint16_t actual_crc = crc16(f.payload);
    if (actual_crc != expected_crc)
        throw std::invalid_argument("CRC mismatch");

    return f;
}

} // namespace ats
