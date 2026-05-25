#pragma once
#include "TlsCredentials.h"
#include "telemetry.grpc.pb.h"      // generated — in ${CMAKE_CURRENT_BINARY_DIR}
#include <grpcpp/grpcpp.h>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>
#include <atomic>

struct WorkerConfig {
    std::string node_id;
    std::string endpoint;   // e.g. "worker1:50051"
};

class TelemetryCoordinator {
public:
    TelemetryCoordinator(std::vector<WorkerConfig> workers, TlsCredentials creds);
    ~TelemetryCoordinator();
    void Start();
    void Stop();
    // Thread-safe snapshot: latest NodeStatus per worker
    std::map<std::string, telemetry::NodeStatus> LatestStatuses() const;
    bool AllNodesRunning() const;
    bool IsReachable(const std::string& node_id) const;

private:
    struct WorkerConn {
        WorkerConfig                              config;
        std::shared_ptr<grpc::Channel>            channel;
        std::unique_ptr<telemetry::TelemetryService::Stub> stub;
        telemetry::NodeStatus                     last_status;
        std::atomic<bool>                         reachable{false};

        WorkerConn() = default;
        WorkerConn(const WorkerConn&) = delete;
        WorkerConn& operator=(const WorkerConn&) = delete;
        WorkerConn(WorkerConn&&) = delete;
        WorkerConn& operator=(WorkerConn&&) = delete;
    };

    // std::deque does not relocate elements on push_back,
    // so non-moveable WorkerConn (containing std::atomic) is safe here.
    std::deque<WorkerConn>    workers_;
    TlsCredentials            creds_;
    mutable std::shared_mutex mu_;
    std::vector<std::thread>  threads_;
    std::atomic<bool>         running_{false};

    void PollWorker(size_t idx);
};
