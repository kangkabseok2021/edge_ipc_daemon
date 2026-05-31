#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include <stdatomic.h>
#include "open62541.h"
#include "cnc_node_ids.h"

/* Shared node-value array — written by the timerfd thread, read by OPC UA
 * DataSource callbacks. _Atomic float gives sequentially consistent access
 * without a mutex in the hot read path.
 * Defined in data_source.c (or data_source_opt.c). */
extern _Atomic float g_node_values[NUM_NODES];

/* Register UA_DataSource read callbacks for all 9 CNC nodes.
 * Must be called after information_model_init(). */
void data_source_init(UA_Server *server);

/* Helpers for unit tests and update_loop */
static inline float data_source_get(CncNodeIdx idx) {
    return atomic_load_explicit(&g_node_values[idx], memory_order_relaxed);
}
static inline void data_source_set(CncNodeIdx idx, float val) {
    atomic_store_explicit(&g_node_values[idx], val, memory_order_relaxed);
}

#endif /* DATA_SOURCE_H */
