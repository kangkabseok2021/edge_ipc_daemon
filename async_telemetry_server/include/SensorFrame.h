#pragma once
#include "Concepts.h"
#include <cstdint>
#include <vector>
#include <cstddef>
#include <cstring>

namespace ats {

struct SensorFrame {
    uint32_t sensor_id{};
    uint64_t timestamp_us{};
    double   value{};

    [[nodiscard]] std::vector<std::byte> serialize() const {
        std::vector<std::byte> buf(sizeof(sensor_id) + sizeof(timestamp_us) + sizeof(value));
        size_t off = 0;
        std::memcpy(buf.data() + off, &sensor_id,    sizeof(sensor_id));    off += sizeof(sensor_id);
        std::memcpy(buf.data() + off, &timestamp_us, sizeof(timestamp_us)); off += sizeof(timestamp_us);
        std::memcpy(buf.data() + off, &value,        sizeof(value));
        return buf;
    }
};

static_assert(TelemetryRecord<SensorFrame>);

} // namespace ats
