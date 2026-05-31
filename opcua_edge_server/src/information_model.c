#include "information_model.h"
#include "cnc_node_ids.h"

/* Parallel arrays — keep in sync with CncNodeId / CncNodeIdx enums */
const unsigned int g_node_numeric_ids[NUM_NODES] = {
    SPINDLE_TEMP, SPINDLE_TORQUE, SPINDLE_SPEED,
    VIBRATION_X,  VIBRATION_Y,   VIBRATION_Z,
    AXIS_X,       AXIS_Y,        AXIS_Z,
};
const char * const g_node_names[NUM_NODES] = {
    "Spindle.Temperature_C",
    "Spindle.Torque_Nm",
    "Spindle.SpeedRPM",
    "Vibration.X_mms",
    "Vibration.Y_mms",
    "Vibration.Z_mms",
    "Axis.X_mm",
    "Axis.Y_mm",
    "Axis.Z_mm",
};

static UA_StatusCode add_variable_node(UA_Server *server, int idx) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Float init = 0.0f;
    UA_Variant_setScalar(&attr.value, &init, &UA_TYPES[UA_TYPES_FLOAT]);
    attr.displayName              = UA_LOCALIZEDTEXT("en-US", (char *)g_node_names[idx]);
    attr.dataType                 = UA_TYPES[UA_TYPES_FLOAT].typeId;
    attr.accessLevel              = UA_ACCESSLEVELMASK_READ;
    attr.minimumSamplingInterval  = 1.0;   /* 1 ms → server supports 1 kHz subscriptions */

    UA_NodeId nodeId      = UA_NODEID_NUMERIC(CNC_NS_IDX, g_node_numeric_ids[idx]);
    UA_NodeId parentId    = UA_NODEID_NUMERIC(CNC_NS_IDX, 1000);  /* CncMachine object */
    UA_NodeId refType     = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    UA_NodeId typeDefId   = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
    UA_QualifiedName bname = UA_QUALIFIEDNAME(CNC_NS_IDX, (char *)g_node_names[idx]);

    return UA_Server_addVariableNode(
        server, nodeId, parentId, refType,
        bname, typeDefId, attr,
        (void *)(uintptr_t)idx,   /* nodeContext = array index for DataSource */
        NULL);
}

UA_StatusCode information_model_init(UA_Server *server) {
    UA_Server_addNamespace(server, "urn:cnc-edge:CncMachine");

    /* Add CncMachine container object under Objects folder */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "CncMachine");
    UA_StatusCode sc = UA_Server_addObjectNode(
        server,
        UA_NODEID_NUMERIC(CNC_NS_IDX, 1000),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(CNC_NS_IDX, "CncMachine"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
        oAttr, NULL, NULL);
    if (sc != UA_STATUSCODE_GOOD) return sc;

    for (int i = 0; i < NUM_NODES; i++) {
        sc = add_variable_node(server, i);
        if (sc != UA_STATUSCODE_GOOD) return sc;
    }
    return UA_STATUSCODE_GOOD;
}
