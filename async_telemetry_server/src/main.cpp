#include "TlsServer.h"
#include "DbPool.h"
#include "TelemetryRepository.h"
#include "JwtValidator.h"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

using namespace ats;
namespace asio = boost::asio;

int main() {
    const char* port_env  = std::getenv("SERVER_PORT");
    const char* db_url    = std::getenv("DATABASE_URL");
    const char* cert_env  = std::getenv("TLS_CERT_PATH");
    const char* key_env   = std::getenv("TLS_KEY_PATH");

    uint16_t    port      = port_env  ? static_cast<uint16_t>(std::stoi(port_env)) : 8443u;
    std::string db        = db_url    ? db_url  : "postgresql://localhost/telemetry";
    std::string cert_path = cert_env  ? cert_env : "certs/server.crt";
    std::string key_path  = key_env   ? key_env  : "certs/server.key";

    std::cout << "[main] async telemetry server starting on port " << port << "\n";

    auto nthreads = std::thread::hardware_concurrency();
    asio::io_context ctx{static_cast<int>(nthreads)};

    DbPool              pool{db};
    JwtValidator        validator;
    TelemetryRepository repo{pool};
    TlsServer           server{ctx, port, validator, repo, cert_path, key_path};

    asio::co_spawn(ctx, server.listen(), asio::detached);

    std::vector<std::thread> threads;
    threads.reserve(nthreads);
    for (unsigned i = 0; i < nthreads; ++i)
        threads.emplace_back([&ctx] { ctx.run(); });
    for (auto& t : threads) t.join();
    return 0;
}
