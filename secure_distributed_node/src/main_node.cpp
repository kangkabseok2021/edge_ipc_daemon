#include "TelemetryNode.h"
#include "TlsCredentials.h"
#include <csignal>
#include <atomic>
#include <iostream>
#include <stdexcept>

static std::atomic<bool> g_quit{false};
static void sigHandler(int) { g_quit = true; }

int main(int argc, char* argv[]) {
    const std::string node_name = (argc > 1) ? argv[1] : "node0";
    uint16_t grpc_port = 50051;
    if (argc > 2) {
        try {
            int p = std::stoi(argv[2]);
            if (p < 1 || p > 65535) throw std::out_of_range("port out of range");
            grpc_port = static_cast<uint16_t>(p);
        } catch (const std::exception& e) {
            std::cerr << "Invalid port '" << argv[2] << "': " << e.what() << "\n";
            return 1;
        }
    }

    std::signal(SIGTERM, sigHandler);
    std::signal(SIGINT,  sigHandler);

    std::shared_ptr<grpc::ServerCredentials> creds;
    if (argc > 3) {
        auto tls = TlsCredentials::LoadFromDir(argv[3], node_name);
        creds = tls.ServerCredentials();
        std::cout << "[" << node_name << "] mTLS enabled from " << argv[3] << "\n";
    } else {
        creds = grpc::InsecureServerCredentials();
    }

    TelemetryNode node(node_name);
    node.Start(grpc_port, creds);

    std::cout << "[" << node_name << "] gRPC server started on port "
              << node.BoundPort() << "\n";

    while (!g_quit.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    node.Stop();
    return 0;
}
