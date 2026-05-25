#include "TelemetryCoordinator.h"
#include "TlsCredentials.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <csignal>
#include <atomic>
#include <iostream>
#include <stdexcept>

static std::atomic<bool> g_quit{false};
static void sigHandler(int) { g_quit = true; }

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: telemetry_coordinator <certs_dir> [health_port]\n";
        return 1;
    }
    const std::string certs_dir  = argv[1];
    const int         health_port = (argc > 2) ? std::stoi(argv[2]) : 8080;

    std::signal(SIGTERM, sigHandler);
    std::signal(SIGINT,  sigHandler);

    auto creds = TlsCredentials::LoadFromDir(certs_dir, "coordinator");

    TelemetryCoordinator coord(
        {{"worker1", "worker1:50051"}, {"worker2", "worker2:50051"}}, creds);
    coord.Start();

    httplib::Server svr;
    svr.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        auto statuses = coord.LatestStatuses();
        nlohmann::json j;
        for (const auto& [id, st] : statuses) {
            j[id] = {
                {"fsm_state",  st.fsm_state()},
                {"last_value", st.last_value()},
                {"reachable",  coord.IsReachable(id)}   // fix: real reachability flag
            };
        }
        j["all_running"] = coord.AllNodesRunning();
        res.set_content(j.dump(), "application/json");
    });

    std::cout << "Coordinator health endpoint: http://0.0.0.0:" << health_port << "/health\n";

    // Run httplib server in background thread; main loop watches g_quit
    std::thread server_thread([&]() { svr.listen("0.0.0.0", health_port); });

    while (!g_quit.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    svr.stop();
    server_thread.join();
    coord.Stop();
    return 0;
}
