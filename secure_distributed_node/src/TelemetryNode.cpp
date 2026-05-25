#include "TelemetryNode.h"
#include "DaemonState.h"
#include "telemetry.grpc.pb.h"
#include "telemetry.pb.h"

#include <grpcpp/grpcpp.h>
#include <chrono>
#include <thread>
#include <string>

// ── ServiceImpl ───────────────────────────────────────────────────────────────

struct TelemetryNode::ServiceImpl : public telemetry::TelemetryService::Service {
    explicit ServiceImpl(TelemetryNode* owner) : owner_(owner) {}

    grpc::Status GetNodeStatus(grpc::ServerContext*,
                               const telemetry::NodeStatusRequest* /*req*/,
                               telemetry::NodeStatus* resp) override
    {
        resp->set_node_id(owner_->node_id_);
        resp->set_fsm_state(std::string(stateToString(owner_->fsm_->state())));
        resp->set_processed_count(owner_->processed_count_.load());
        resp->set_last_value(owner_->last_filtered_.load());
        return grpc::Status::OK;
    }

    grpc::Status StreamTelemetry(grpc::ServerContext* ctx,
                                  const telemetry::NodeStatusRequest* /*req*/,
                                  grpc::ServerWriter<telemetry::TelemetrySample>* writer) override
    {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (!ctx->IsCancelled() && std::chrono::steady_clock::now() < deadline) {
            ProcessedSnapshot snap;
            if (owner_->PopSnapshot(snap, std::chrono::milliseconds(100))) {
                telemetry::TelemetrySample s;
                s.set_node_id(snap.node_id);
                s.set_raw_value(snap.raw_value);
                s.set_filtered_value(snap.filtered_value);
                s.set_alarm_active(snap.alarm_active);
                s.set_fsm_state(snap.fsm_state);
                s.set_timestamp_ms(snap.timestamp_ms);
                if (!writer->Write(s)) break;
            }
        }
        return grpc::Status::OK;
    }

    TelemetryNode* owner_;
};

// ── TelemetryNode ─────────────────────────────────────────────────────────────

TelemetryNode::TelemetryNode(std::string node_id)
    : node_id_(std::move(node_id))
    , fsm_(std::make_shared<StateManager>())
    , receiver_(std::make_shared<TelemetryReceiver>())
    , processor_(std::make_unique<DataProcessor>())
    , service_(std::make_unique<ServiceImpl>(this))
{
    // Wire FSM alarm callback
    fsm_->setAlarmCallback([this](const std::string& /*sensor_id*/,
                                   double /*value*/, double /*threshold*/) {
        alarm_active_.store(true);
    });
    fsm_->setStateChangedCallback([this](DaemonState next, DaemonState /*prev*/) {
        if (next != DaemonState::FAULT) {
            alarm_active_.store(false);
        }
    });
}

TelemetryNode::~TelemetryNode() {
    Stop();
}

void TelemetryNode::Start(uint16_t grpc_port,
                          std::shared_ptr<grpc::ServerCredentials> creds)
{
    if (running_.exchange(true)) return;   // already started

    fsm_->start();

    // Start producer thread
    producer_thread_ = std::thread([this] { ProducerLoop(); });

    // Build gRPC server
    int selected_port = 0;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(
        "0.0.0.0:" + std::to_string(grpc_port), creds, &selected_port);
    builder.RegisterService(service_.get());
    grpc_server_ = builder.BuildAndStart();
    actual_port_.store(selected_port);
}

void TelemetryNode::Stop() {
    if (!running_.exchange(false)) return;  // already stopped

    // 1. Wake any blocked PopSnapshot calls so StreamTelemetry can exit
    //    before the server drain blocks waiting for it.
    stream_cv_.notify_all();

    // 2. Drain and destroy the gRPC server (explicit Wait so the server thread
    //    fully exits before we proceed).
    if (grpc_server_) {
        grpc_server_->Shutdown();
        grpc_server_->Wait();
        grpc_server_.reset();
    }

    // 3. Join the producer thread.
    if (producer_thread_.joinable()) {
        producer_thread_.join();
    }

    // 4. Shut down the FSM.
    fsm_->shutdown();
}

DaemonState TelemetryNode::CurrentState() const {
    return fsm_->state();
}

uint64_t TelemetryNode::ProcessedCount() const {
    return processed_count_.load();
}

double TelemetryNode::LastValue() const {
    return last_filtered_.load();
}

void TelemetryNode::PushSnapshot(ProcessedSnapshot snap) {
    std::unique_lock<std::mutex> lk(stream_mu_);
    if (stream_queue_.size() >= STREAM_QUEUE_CAP) {
        stream_queue_.pop_front();   // drop oldest to keep bounded
    }
    stream_queue_.push_back(std::move(snap));
    lk.unlock();
    stream_cv_.notify_one();
}

bool TelemetryNode::PopSnapshot(ProcessedSnapshot& out,
                                std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lk(stream_mu_);
    if (!stream_cv_.wait_for(lk, timeout,
            [this] { return !stream_queue_.empty() || !running_.load(); })) {
        return false;
    }
    if (stream_queue_.empty()) return false;
    out = std::move(stream_queue_.front());
    stream_queue_.pop_front();
    return true;
}

void TelemetryNode::ProducerLoop() {
    SimulatedSource src(node_id_ + "_sensor", 50.0, 10.0);

    while (running_.load()) {
        auto frame_opt = src.read();
        if (frame_opt) {
            receiver_->push(*frame_opt);
        }

        // Pop and process all available frames
        TelemetryFrame frame;
        while (receiver_->pop(frame)) {
            auto metric = processor_->process(frame);
            last_raw_.store(frame.value);
            last_filtered_.store(metric.filtered_value);
            bool alarm = metric.alarm_flag;
            if (alarm) {
                alarm_active_.store(true);
                // Drive FSM only when RUNNING
                if (fsm_->state() == DaemonState::RUNNING) {
                    fsm_->fault(metric.sensor_id, frame.value, 80.0);
                }
            }
            processed_count_.fetch_add(1);

            // Push a processed snapshot for StreamTelemetry subscribers
            ProcessedSnapshot snap;
            snap.node_id        = node_id_;
            snap.raw_value      = frame.value;
            snap.filtered_value = metric.filtered_value;
            snap.alarm_active   = alarm;
            snap.fsm_state      = std::string(stateToString(fsm_->state()));
            snap.timestamp_ms   = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            PushSnapshot(std::move(snap));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
