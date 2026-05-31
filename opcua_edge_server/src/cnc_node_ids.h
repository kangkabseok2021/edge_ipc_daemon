#ifndef CNC_NODE_IDS_H
#define CNC_NODE_IDS_H

/* OPC UA namespace index for the CncMachine model (second custom ns on a default server) */
#define CNC_NS_IDX  2
#define NUM_NODES   9

/* OPC UA numeric NodeIds — used in the Information Model and data source registration */
typedef enum {
    SPINDLE_TEMP   = 1001,
    SPINDLE_TORQUE = 1002,
    SPINDLE_SPEED  = 1003,
    VIBRATION_X    = 1004,
    VIBRATION_Y    = 1005,
    VIBRATION_Z    = 1006,
    AXIS_X         = 1007,
    AXIS_Y         = 1008,
    AXIS_Z         = 1009,
} CncNodeId;

/* Array-index mapping into g_node_values[NUM_NODES] */
typedef enum {
    IDX_SPINDLE_TEMP   = 0,
    IDX_SPINDLE_TORQUE = 1,
    IDX_SPINDLE_SPEED  = 2,
    IDX_VIBRATION_X    = 3,
    IDX_VIBRATION_Y    = 4,
    IDX_VIBRATION_Z    = 5,
    IDX_AXIS_X         = 6,
    IDX_AXIS_Y         = 7,
    IDX_AXIS_Z         = 8,
} CncNodeIdx;

/* Parallel arrays — keep in sync with the enums above */
extern const unsigned int  g_node_numeric_ids[NUM_NODES];
extern const char * const  g_node_names[NUM_NODES];

#endif /* CNC_NODE_IDS_H */
