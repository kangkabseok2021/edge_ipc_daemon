#ifndef INFORMATION_MODEL_H
#define INFORMATION_MODEL_H

#include "open62541.h"

/* Register namespace "urn:cnc-edge:CncMachine" (returns ns index = 2),
 * add the CncMachine object node under Objects, and add 9 Float variable
 * nodes (Spindle.*, Vibration.*, Axis.*).
 * Returns UA_STATUSCODE_GOOD or the first failing status code. */
UA_StatusCode information_model_init(UA_Server *server);

#endif /* INFORMATION_MODEL_H */
