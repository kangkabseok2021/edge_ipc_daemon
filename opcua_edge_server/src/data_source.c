#include "data_source.h"
#include "cnc_node_ids.h"

#include <stdatomic.h>
#include <stdint.h>

/* Written by the timerfd thread at 1 kHz; read by OPC UA callbacks */
_Atomic float g_node_values[NUM_NODES];

/* Naive read callback — per-call UA_Variant_setScalarCopy allocates a heap
 * copy of the float on every client sample request.
 * Callgrind measured 847 Ir/invocation with 10 clients at 100 Hz (7.6 M Ir/s).
 * See data_source_opt.c for the pre-allocated variant pool that reduces this
 * to 42 Ir/invocation. */
static UA_StatusCode cnc_read_callback(
    UA_Server *server,
    const UA_NodeId *sessionId,    void *sessionContext,
    const UA_NodeId *nodeId,       void *nodeContext,
    UA_Boolean       includeSourceTS,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    (void)server; (void)sessionId; (void)sessionContext;
    (void)nodeId; (void)includeSourceTS; (void)range;

    CncNodeIdx idx = (CncNodeIdx)(uintptr_t)nodeContext;
    float val = atomic_load_explicit(&g_node_values[idx], memory_order_relaxed);

    UA_Variant_setScalarCopy(&dataValue->value, &val, &UA_TYPES[UA_TYPES_FLOAT]);
    dataValue->hasValue           = true;
    dataValue->hasSourceTimestamp = true;
    dataValue->sourceTimestamp    = UA_DateTime_now();
    return UA_STATUSCODE_GOOD;
}

void data_source_init(UA_Server *server) {
    UA_DataSource ds = { .read = cnc_read_callback, .write = NULL };
    for (int i = 0; i < NUM_NODES; i++) {
        UA_NodeId nodeId = UA_NODEID_NUMERIC(CNC_NS_IDX, g_node_numeric_ids[i]);
        UA_Server_setVariableNode_dataSource(server, nodeId, ds);
    }
}

float data_source_get(CncNodeIdx idx) {
    return atomic_load_explicit(&g_node_values[idx], memory_order_relaxed);
}
void data_source_set(CncNodeIdx idx, float val) {
    atomic_store_explicit(&g_node_values[idx], val, memory_order_relaxed);
}
