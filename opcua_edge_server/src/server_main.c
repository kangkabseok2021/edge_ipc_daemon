#include <signal.h>
#include <stdio.h>

#include "open62541.h"
#include "telemetry_params.h"
#include "information_model.h"
#include "data_source.h"
#include "update_loop.h"

static volatile UA_Boolean g_running = true;

static void stop_handler(int sig) {
    (void)sig;
    g_running = false;
}

int main(void) {
    signal(SIGTERM, stop_handler);
    signal(SIGINT,  stop_handler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    config->maxSecureChannels              = MAX_SECURE_CHANNELS;
    config->maxSessions                    = MAX_SESSIONS;
    /* Minimum publishing interval 10 ms — clients can subscribe up to 100 Hz */
    config->publishingIntervalLimits.min   = 10.0;
    config->publishingIntervalLimits.max   = 5000.0;

    UA_StatusCode sc = information_model_init(server);
    if (sc != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "information_model_init failed: %s\n",
                UA_StatusCode_name(sc));
        UA_Server_delete(server);
        return 1;
    }
    data_source_init(server);

    if (update_loop_start() != 0) {
        fprintf(stderr, "update_loop_start failed (timerfd unavailable?)\n");
        UA_Server_delete(server);
        return 1;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "CNC OPC UA server listening on opc.tcp://0.0.0.0:4840");
    UA_Server_run(server, &g_running);

    update_loop_stop();
    UA_Server_delete(server);
    return 0;
}
