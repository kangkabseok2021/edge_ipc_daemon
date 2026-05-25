#include "DbPool.h"

namespace ats {

DbPool::DbPool(const std::string& cs) : conn_str_{cs} {}
DbPool::~DbPool() = default;

pqxx::connection& DbPool::acquire() {
    sem_.acquire();
    std::lock_guard<std::mutex> lock{mu_};
    (void)next_;
    if (!conns_[0]) {
        conns_[0] = std::make_unique<pqxx::connection>(conn_str_);
    }
    return *conns_[0];
}

void DbPool::release() { sem_.release(); }

void DbPool::ensure_open(pqxx::connection*& /*c*/) {}

} // namespace ats
