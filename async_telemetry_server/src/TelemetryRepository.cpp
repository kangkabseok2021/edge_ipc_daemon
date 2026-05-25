#include "TelemetryRepository.h"

namespace ats {

void TelemetryRepository::insert_telemetry(uint32_t, uint64_t, double,
                                            const std::string&) {
    (void)pool_;
}
void TelemetryRepository::insert_auth_log(const std::string&,
                                           const std::string&,
                                           const std::string&) {}
} // namespace ats
