#include <gtest/gtest.h>

extern "C" {
#include "cnc_node_ids.h"
#include "information_model.h"
}

TEST(InformationModel, NumNodesIs9) {
    EXPECT_EQ(NUM_NODES, 9);
}

TEST(InformationModel, NodeIdEnumValues) {
    EXPECT_EQ(SPINDLE_TEMP,   1001);
    EXPECT_EQ(SPINDLE_TORQUE, 1002);
    EXPECT_EQ(SPINDLE_SPEED,  1003);
    EXPECT_EQ(VIBRATION_X,    1004);
    EXPECT_EQ(VIBRATION_Y,    1005);
    EXPECT_EQ(VIBRATION_Z,    1006);
    EXPECT_EQ(AXIS_X,         1007);
    EXPECT_EQ(AXIS_Y,         1008);
    EXPECT_EQ(AXIS_Z,         1009);
}

TEST(InformationModel, ParallelArraysMatchEnums) {
    EXPECT_EQ(g_node_numeric_ids[IDX_SPINDLE_TEMP],   (unsigned)SPINDLE_TEMP);
    EXPECT_EQ(g_node_numeric_ids[IDX_SPINDLE_TORQUE], (unsigned)SPINDLE_TORQUE);
    EXPECT_EQ(g_node_numeric_ids[IDX_SPINDLE_SPEED],  (unsigned)SPINDLE_SPEED);
    EXPECT_EQ(g_node_numeric_ids[IDX_VIBRATION_X],    (unsigned)VIBRATION_X);
    EXPECT_EQ(g_node_numeric_ids[IDX_AXIS_Z],         (unsigned)AXIS_Z);
}

TEST(InformationModel, AddNodesToServerSucceeds) {
    UA_Server *server = UA_Server_new();
    ASSERT_NE(server, nullptr);
    UA_StatusCode sc = information_model_init(server);
    EXPECT_EQ(sc, UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}
