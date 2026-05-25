#include "TelemetryCoordinator.h"
#include <chrono>
#include <thread>

TelemetryCoordinator::TelemetryCoordinator(std::vector<WorkerConfig> workers, TlsCredentials creds)
    : creds_(std::move(creds)) {
    for (auto& w : workers) {
        workers_.emplace_back();
        WorkerConn& conn = workers_.back();
        conn.config   = w;
        conn.channel  = grpc::CreateChannel(w.endpoint, creds_.ChannelCredentials());
        conn.stub     = telemetry::TelemetryService::NewStub(conn.channel);
    }
}

TelemetryCoordinator::~TelemetryCoordinator() { Stop(); }

void TelemetryCoordinator::Start() {
    running_.store(true);
    for (size_t i = 0; i < workers_.size(); ++i) {
        threads_.emplace_back([this, i]() { PollWorker(i); });
    }
}

void TelemetryCoordinator::Stop() {
    if (!running_.exchange(false)) return;
    for (auto& t : threads_)
        if (t.joinable()) t.join();
    threads_.clear();
}

void TelemetryCoordinator::PollWorker(size_t idx) {
    while (running_.load()) {
        grpc::ClientContext ctx;
        ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(3));
        telemetry::NodeStatusRequest req;
        req.set_node_id(workers_[idx].config.node_id);
        telemetry::NodeStatus resp;
        auto status = workers_[idx].stub->GetNodeStatus(&ctx, req, &resp);
        {
            std::unique_lock lock(mu_);
            if (status.ok()) {
                workers_[idx].last_status = resp;
                workers_[idx].reachable.store(true);
            } else {
                workers_[idx].reachable.store(false);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

std::map<std::string, telemetry::NodeStatus> TelemetryCoordinator::LatestStatuses() const {
    std::shared_lock lock(mu_);
    std::map<std::string, telemetry::NodeStatus> result;
    for (const auto& w : workers_)
        result[w.config.node_id] = w.last_status;
    return result;
}

bool TelemetryCoordinator::IsReachable(const std::string& node_id) const {
    std::shared_lock lock(mu_);
    for (const auto& w : workers_) {
        if (w.config.node_id == node_id)
            return w.reachable.load();
    }
    return false;
}

bool TelemetryCoordinator::AllNodesRunning() const {
    std::shared_lock lock(mu_);
    for (const auto& w : workers_) {
        if (!w.reachable.load() || w.last_status.fsm_state() != "RUNNING")
            return false;
    }
    return !workers_.empty();
}
