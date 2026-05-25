#include "TelemetryNode.h"
#include <csignal>
#include <atomic>
#include <iostream>
#include <cstdlib>

static std::atomic<bool> g_quit{false};
static void sigHandler(int) { g_quit = true; }

int main(int argc, char* argv[]) {
    const std::string node_name = (argc > 1) ? argv[1] : "node0";
    const uint16_t    grpc_port = (argc > 2)
        ? static_cast<uint16_t>(std::atoi(argv[2]))
        : 50051;

    std::signal(SIGTERM, sigHandler);
    std::signal(SIGINT,  sigHandler);

    TelemetryNode node(node_name);
    node.Start(grpc_port, grpc::InsecureServerCredentials());

    std::cout << "[" << node_name << "] gRPC server started on port "
              << node.BoundPort() << "\n";

    while (!g_quit.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    node.Stop();
    return 0;
}
