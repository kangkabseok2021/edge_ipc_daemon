#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include "open62541.h"
#include "cnc_node_ids.h"

/* Register UA_DataSource read callbacks for all 9 CNC nodes.
 * Must be called after information_model_init(). */
void data_source_init(UA_Server *server);

/* Thread-safe accessors for g_node_values[].
 * The underlying _Atomic float array is an implementation detail of
 * data_source.c — not exposed here so C++ test files compile cleanly. */
float data_source_get(CncNodeIdx idx);
void  data_source_set(CncNodeIdx idx, float val);

#endif /* DATA_SOURCE_H */
