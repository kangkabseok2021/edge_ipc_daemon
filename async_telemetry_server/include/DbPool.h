#pragma once
#include <pqxx/pqxx>
#include <semaphore>
#include <array>
#include <mutex>
#include <string>
#include <memory>

namespace ats {

class DbPool {
public:
    static constexpr int POOL_SIZE = 8;

    explicit DbPool(const std::string& connection_string);
    ~DbPool();

    // Blocks until a connection is available. Returns a pqxx::connection&.
    pqxx::connection& acquire();
    void release();

private:
    std::counting_semaphore<POOL_SIZE>                              sem_{POOL_SIZE};
    std::array<std::unique_ptr<pqxx::connection>, POOL_SIZE>        conns_;
    std::mutex                                                       mu_;
    int                                                              next_{0};
    std::string                                                      conn_str_;

    void ensure_open(std::unique_ptr<pqxx::connection>& ptr);
};

} // namespace ats
