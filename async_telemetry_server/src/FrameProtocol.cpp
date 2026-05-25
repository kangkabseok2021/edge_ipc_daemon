#include "FrameProtocol.h"

namespace ats {

uint16_t crc16(const std::vector<std::byte>& /*data*/) noexcept { return 0; }

std::vector<std::byte> Frame::serialize() const { return {}; }

Frame Frame::deserialize(const std::vector<std::byte>& /*buf*/) {
    return Frame{};
}

} // namespace ats
