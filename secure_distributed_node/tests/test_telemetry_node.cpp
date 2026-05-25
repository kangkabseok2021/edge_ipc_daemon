#include "TelemetryNode.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <grpcpp/grpcpp.h>
#include "telemetry.grpc.pb.h"
#include <thread>
#include <chrono>

class TelemetryNodeTest : public ::testing::Test {
protected:
    TelemetryNode node_{"test-node"};
    std::unique_ptr<telemetry::TelemetryService::Stub> stub_;

    void SetUp() override {
        node_.Start(0, grpc::InsecureServerCredentials());
        // Wait for server to be ready
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto channel = grpc::CreateChannel(
            "localhost:" + std::to_string(node_.BoundPort()),
            grpc::InsecureChannelCredentials());
        stub_ = telemetry::TelemetryService::NewStub(channel);
    }

    void TearDown() override { node_.Stop(); }
};

TEST_F(TelemetryNodeTest, GetNodeStatus_ReturnsFsmState) {
    grpc::ClientContext ctx;
    telemetry::NodeStatusRequest req;
    req.set_node_id("test-node");
    telemetry::NodeStatus resp;
    auto status = stub_->GetNodeStatus(&ctx, req, &resp);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(resp.node_id(), "test-node");
    EXPECT_THAT((std::vector<std::string>{"IDLE", "RUNNING", "FAULT", "SHUTDOWN"}),
                ::testing::Contains(resp.fsm_state()));
}

TEST_F(TelemetryNodeTest, GetNodeStatus_ProcessedCountIncreasesOverTime) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    grpc::ClientContext ctx1, ctx2;
    telemetry::NodeStatusRequest req;
    telemetry::NodeStatus r1, r2;
    stub_->GetNodeStatus(&ctx1, req, &r1);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    stub_->GetNodeStatus(&ctx2, req, &r2);
    EXPECT_GE(r2.processed_count(), r1.processed_count());
}

TEST_F(TelemetryNodeTest, StreamTelemetry_DeliversAtLeastThreeSamples) {
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    telemetry::NodeStatusRequest req;
    req.set_node_id("test-node");
    auto reader = stub_->StreamTelemetry(&ctx, req);
    telemetry::TelemetrySample sample;
    int count = 0;
    while (count < 3 && reader->Read(&sample)) {
        EXPECT_EQ(sample.node_id(), "test-node");
        EXPECT_FALSE(sample.fsm_state().empty());
        ++count;
    }
    ctx.TryCancel();
    reader->Finish();
    EXPECT_GE(count, 3);
}

TEST_F(TelemetryNodeTest, GetNodeStatus_NodeIdMatchesConstructorArg) {
    grpc::ClientContext ctx;
    telemetry::NodeStatusRequest req;
    telemetry::NodeStatus resp;
    stub_->GetNodeStatus(&ctx, req, &resp);
    EXPECT_EQ(resp.node_id(), "test-node");
}
