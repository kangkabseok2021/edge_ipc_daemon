#include "DbPool.h"
#include <stdexcept>

namespace ats {

DbPool::DbPool(const std::string& cs) : conn_str_{cs} {
    for (auto& c : conns_)
        c = std::make_unique<pqxx::connection>(cs);
}

DbPool::~DbPool() = default;

void DbPool::ensure_open(std::unique_ptr<pqxx::connection>& ptr) {
    if (!ptr || !ptr->is_open())
        ptr = std::make_unique<pqxx::connection>(conn_str_);
}

pqxx::connection& DbPool::acquire() {
    sem_.acquire();
    std::lock_guard<std::mutex> lk{mu_};
    int idx = next_;
    next_ = (next_ + 1) % POOL_SIZE;
    ensure_open(conns_[static_cast<size_t>(idx)]);
    return *conns_[static_cast<size_t>(idx)];
}

void DbPool::release() {
    sem_.release();
}

} // namespace ats
