#pragma once
#include "TelemetryFrame.h"   // from parent include/
#include "DataProcessor.h"
#include "StateManager.h"
#include "TelemetryReceiver.h"
#include "SimulatedSource.h"
#include "DaemonBus.h"
// generated gRPC headers included in .cpp
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>

// A lightweight processed-sample snapshot pushed by ProducerLoop.
struct ProcessedSnapshot {
    std::string   node_id;
    double        raw_value{0.0};
    double        filtered_value{0.0};
    bool          alarm_active{false};
    std::string   fsm_state;
    int64_t       timestamp_ms{0};
};

class TelemetryNode {
public:
    explicit TelemetryNode(std::string node_id);
    ~TelemetryNode();

    void Start(uint16_t grpc_port, std::shared_ptr<grpc::ServerCredentials> creds);
    void Stop();

    DaemonState CurrentState() const;
    uint64_t    ProcessedCount() const;
    double      LastValue() const;
    int         BoundPort() const { return actual_port_.load(); }

private:
    std::string node_id_;
    std::shared_ptr<StateManager>      fsm_;
    std::shared_ptr<TelemetryReceiver> receiver_;
    std::unique_ptr<DataProcessor>     processor_;
    std::unique_ptr<grpc::Server>      grpc_server_;
    std::atomic<int>                   actual_port_{0};

    std::atomic<bool>     running_{false};
    std::atomic<uint64_t> processed_count_{0};
    std::atomic<double>   last_raw_{0.0};
    std::atomic<double>   last_filtered_{0.0};
    std::atomic<bool>     alarm_active_{false};

    // Bounded ring-buffer for StreamTelemetry service to consume processed
    // snapshots.  ProducerLoop writes here; ServiceImpl::StreamTelemetry reads.
    static constexpr size_t STREAM_QUEUE_CAP = 256;
    std::deque<ProcessedSnapshot>  stream_queue_;
    std::mutex                     stream_mu_;
    std::condition_variable        stream_cv_;

    void PushSnapshot(ProcessedSnapshot snap);
    bool PopSnapshot(ProcessedSnapshot& out, std::chrono::milliseconds timeout);

    // Producer thread pushes TelemetryFrames from SimulatedSource into receiver_
    std::thread producer_thread_;
    void ProducerLoop();

    // Internal service impl is a friend so it can access private members
    struct ServiceImpl;
    std::unique_ptr<ServiceImpl> service_;
};
