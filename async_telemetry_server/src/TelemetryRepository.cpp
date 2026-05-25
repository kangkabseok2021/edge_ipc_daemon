#include "TelemetryRepository.h"
#include <pqxx/pqxx>

namespace ats {

void TelemetryRepository::insert_telemetry(
        uint32_t sensor_id, uint64_t timestamp_us,
        double value, const std::string& client_subject) {
    auto& conn = pool_.acquire();
    try {
        pqxx::work txn{conn};
        txn.exec(
            "INSERT INTO telemetry(sensor_id, ts, value, client_subject)"
            " VALUES($1, to_timestamp($2::double precision / 1e6), $3, $4)",
            pqxx::params{sensor_id,
                         static_cast<double>(timestamp_us),
                         value,
                         client_subject});
        txn.commit();
    } catch (...) {
        pool_.release();
        throw;
    }
    pool_.release();
}

void TelemetryRepository::insert_auth_log(
        const std::string& client_subject,
        const std::string& event,
        const std::string& ip_addr) {
    auto& conn = pool_.acquire();
    try {
        pqxx::work txn{conn};
        txn.exec(
            "INSERT INTO auth_log(client_subject, event, ip_addr)"
            " VALUES($1, $2, $3::inet)",
            pqxx::params{client_subject, event, ip_addr});
        txn.commit();
    } catch (...) {
        pool_.release();
        throw;
    }
    pool_.release();
}

} // namespace ats
