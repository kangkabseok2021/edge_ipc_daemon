#pragma once
#include <concepts>
#include <cstdint>
#include <vector>
#include <cstddef>

namespace ats {

template<typename T>
concept TelemetryRecord = requires(T t) {
    { t.sensor_id    } -> std::convertible_to<uint32_t>;
    { t.timestamp_us } -> std::convertible_to<uint64_t>;
    { t.value        } -> std::convertible_to<double>;
    { t.serialize()  } -> std::same_as<std::vector<std::byte>>;
};

} // namespace ats
