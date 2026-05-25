#include <gtest/gtest.h>
#include "FrameProtocol.h"
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

using namespace ats;

// ── CRC-16/CCITT ──────────────────────────────────────────────────────────────
TEST(Crc16, EmptyPayload) {
    // CRC-16/CCITT of empty input = 0xFFFF (initial value, no data processed)
    std::vector<std::byte> empty;
    EXPECT_EQ(crc16(empty), 0xFFFF);
}

TEST(Crc16, KnownVector) {
    // "123456789" -> CRC-16/CCITT = 0x29B1
    std::string s = "123456789";
    std::vector<std::byte> data;
    data.reserve(s.size());
    for (char c : s) data.push_back(static_cast<std::byte>(c));
    EXPECT_EQ(crc16(data), 0x29B1);
}

// ── Frame::serialize ──────────────────────────────────────────────────────────
TEST(FrameProto, SerializeHeader) {
    Frame f;
    f.msg_type = MsgType::TELEMETRY;
    f.payload  = {};
    auto bytes = f.serialize();
    // Must be exactly 12 bytes (header only, empty payload)
    ASSERT_EQ(bytes.size(), 12u);
    // Magic check
    EXPECT_EQ(static_cast<uint8_t>(bytes[0]), 0x4E);
    EXPECT_EQ(static_cast<uint8_t>(bytes[1]), 0x45);
    EXPECT_EQ(static_cast<uint8_t>(bytes[2]), 0x58);
    EXPECT_EQ(static_cast<uint8_t>(bytes[3]), 0x54);
    // Version
    EXPECT_EQ(static_cast<uint8_t>(bytes[4]), 0x01);
    // MsgType = TELEMETRY = 0x02
    EXPECT_EQ(static_cast<uint8_t>(bytes[5]), 0x02);
}

TEST(FrameProto, SerializePayloadLen) {
    Frame f;
    f.msg_type = MsgType::AUTH;
    f.payload  = {std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}};
    auto bytes = f.serialize();
    ASSERT_GE(bytes.size(), 12u);
    // payload_len at offset 6 (4 bytes, big-endian) = 3
    uint32_t len_be{};
    std::memcpy(&len_be, bytes.data() + 6, 4);
    EXPECT_EQ(ntohl(len_be), 3u);
    // Total size = 12 + 3 = 15
    EXPECT_EQ(bytes.size(), 15u);
}

TEST(FrameProto, RoundTrip) {
    Frame out;
    out.msg_type = MsgType::TELEMETRY;
    std::string payload = "hello world";
    out.payload.assign(
        reinterpret_cast<const std::byte*>(payload.data()),
        reinterpret_cast<const std::byte*>(payload.data() + payload.size()));
    auto bytes = out.serialize();

    Frame in = Frame::deserialize(bytes);
    EXPECT_EQ(in.msg_type, MsgType::TELEMETRY);
    EXPECT_EQ(in.payload,  out.payload);
}

TEST(FrameProto, DeserializeRejectsBadMagic) {
    Frame f;
    f.msg_type = MsgType::AUTH;
    f.payload  = {};
    auto bytes = f.serialize();
    // Corrupt magic byte
    bytes[0] = std::byte{0xFF};
    EXPECT_THROW((void)Frame::deserialize(bytes), std::invalid_argument);
}

TEST(FrameProto, DeserializeRejectsBadVersion) {
    Frame f;
    f.msg_type = MsgType::AUTH;
    f.payload  = {};
    auto bytes = f.serialize();
    bytes[4] = std::byte{0x99}; // wrong version
    EXPECT_THROW((void)Frame::deserialize(bytes), std::invalid_argument);
}

TEST(FrameProto, DeserializeRejectsBadCrc) {
    Frame f;
    f.msg_type = MsgType::AUTH;
    f.payload  = {std::byte{0x01}, std::byte{0x02}};
    auto bytes = f.serialize();
    // Flip a payload byte to break CRC
    bytes[12] = std::byte{static_cast<uint8_t>(~static_cast<uint8_t>(bytes[12]))};
    EXPECT_THROW((void)Frame::deserialize(bytes), std::invalid_argument);
}

TEST(FrameProto, SocketpairRoundTrip) {
    int fds[2];
    ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    Frame out;
    out.msg_type = MsgType::ACK;
    std::string payload = "socketpair test";
    out.payload.assign(
        reinterpret_cast<const std::byte*>(payload.data()),
        reinterpret_cast<const std::byte*>(payload.data() + payload.size()));
    auto bytes = out.serialize();

    // Write
    ssize_t written = ::write(fds[0], bytes.data(), bytes.size());
    ASSERT_EQ(static_cast<size_t>(written), bytes.size());

    // Read header
    std::vector<std::byte> hdr_buf(12);
    ::read(fds[1], hdr_buf.data(), 12);

    // Read payload length from header
    uint32_t plen_be{};
    std::memcpy(&plen_be, hdr_buf.data() + 6, 4);
    uint32_t plen = ntohl(plen_be);

    std::vector<std::byte> full_buf(12 + plen);
    std::memcpy(full_buf.data(), hdr_buf.data(), 12);
    ::read(fds[1], full_buf.data() + 12, plen);

    Frame in = Frame::deserialize(full_buf);
    EXPECT_EQ(in.msg_type, MsgType::ACK);
    EXPECT_EQ(in.payload,  out.payload);

    ::close(fds[0]);
    ::close(fds[1]);
}
