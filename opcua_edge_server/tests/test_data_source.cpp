#include <gtest/gtest.h>

extern "C" {
#include "data_source.h"
#include "information_model.h"
#include "cnc_node_ids.h"
}

TEST(DataSource, WriteReadRoundTrip) {
    data_source_set(IDX_SPINDLE_TEMP, 42.5f);
    EXPECT_FLOAT_EQ(data_source_get(IDX_SPINDLE_TEMP), 42.5f);
}

TEST(DataSource, AllIndicesWriteWithoutCrash) {
    for (int i = 0; i < NUM_NODES; i++) {
        EXPECT_NO_THROW(data_source_set(static_cast<CncNodeIdx>(i), (float)i));
        EXPECT_FLOAT_EQ(data_source_get(static_cast<CncNodeIdx>(i)), (float)i);
    }
}

TEST(DataSource, MultipleWritesPreserveLastValue) {
    for (int i = 0; i < 100; i++)
        data_source_set(IDX_VIBRATION_X, (float)i);
    EXPECT_FLOAT_EQ(data_source_get(IDX_VIBRATION_X), 99.0f);
}

TEST(DataSource, RegisterDataSourcesOnServer) {
    UA_Server *server = UA_Server_new();
    ASSERT_NE(server, nullptr);
    EXPECT_EQ(information_model_init(server), UA_STATUSCODE_GOOD);
    EXPECT_NO_THROW(data_source_init(server));
    UA_Server_delete(server);
}

TEST(DataSource, GetReflectsLastSetValue) {
    data_source_set(IDX_AXIS_X, 1.234f);
    EXPECT_FLOAT_EQ(data_source_get(IDX_AXIS_X), 1.234f);
    data_source_set(IDX_AXIS_X, -7.5f);
    EXPECT_FLOAT_EQ(data_source_get(IDX_AXIS_X), -7.5f);
}
