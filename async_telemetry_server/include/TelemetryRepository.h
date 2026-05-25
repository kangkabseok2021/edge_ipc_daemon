#pragma once
#include "DbPool.h"
#include <cstdint>
#include <string>

namespace ats {

class TelemetryRepository {
public:
    explicit TelemetryRepository(DbPool& pool) : pool_{pool} {}

    void insert_telemetry(uint32_t sensor_id, uint64_t timestamp_us,
                          double value, const std::string& client_subject);

    void insert_auth_log(const std::string& client_subject,
                         const std::string& event,
                         const std::string& ip_addr);

private:
    DbPool& pool_;
};

} // namespace ats
