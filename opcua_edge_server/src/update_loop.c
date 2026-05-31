#include "update_loop.h"
#include "data_source.h"
#include "telemetry_sim.h"
#include "cnc_node_ids.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __linux__

#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <errno.h>
#include <string.h>

static pthread_t        g_thread;
static atomic_bool      g_running;
static _Atomic uint64_t g_tick_count;

/* Per-axis seeds for rand_r — kept in the update thread only */
static uint32_t g_seeds[NUM_NODES] = {
    0xDEAD, 0xBEEF, 0xCAFE, 0xBABE, 0xFACE, 0xD00D, 0xF00D, 0xACE1, 0xB00B
};

static void update_all_nodes(double t_s) {
    data_source_set(IDX_SPINDLE_TEMP,   telemetry_temperature(t_s,      &g_seeds[0]));
    data_source_set(IDX_SPINDLE_TORQUE, telemetry_torque(t_s,            &g_seeds[1]));
    data_source_set(IDX_SPINDLE_SPEED,  telemetry_spindle_speed(t_s,     &g_seeds[2]));
    data_source_set(IDX_VIBRATION_X,    telemetry_vibration(t_s, 0,      &g_seeds[3]));
    data_source_set(IDX_VIBRATION_Y,    telemetry_vibration(t_s, 1,      &g_seeds[4]));
    data_source_set(IDX_VIBRATION_Z,    telemetry_vibration(t_s, 2,      &g_seeds[5]));
    data_source_set(IDX_AXIS_X,         telemetry_axis_position(t_s, 0));
    data_source_set(IDX_AXIS_Y,         telemetry_axis_position(t_s, 1));
    data_source_set(IDX_AXIS_Z,         telemetry_axis_position(t_s, 2));
}

static void *update_thread(void *arg) {
    (void)arg;

    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (tfd < 0) {
        perror("timerfd_create");
        return NULL;
    }

    struct itimerspec ts = {
        .it_interval = { .tv_sec = 0, .tv_nsec = 1000000L },   /* 1 ms */
        .it_value    = { .tv_sec = 0, .tv_nsec = 1000000L },
    };
    timerfd_settime(tfd, 0, &ts, NULL);

    uint64_t tick = 0;
    while (atomic_load_explicit(&g_running, memory_order_relaxed)) {
        uint64_t exp = 0;
        ssize_t  n   = read(tfd, &exp, sizeof(exp));
        if (n != sizeof(exp)) continue;
        if (exp > 1) {
            fprintf(stderr, "[update_loop] timer overrun: %" PRIu64 " ticks\n",
                    exp - 1);
        }
        tick += exp;
        double t_s = (double)tick * 0.001;
        update_all_nodes(t_s);
        atomic_store_explicit(&g_tick_count, tick, memory_order_relaxed);
    }

    close(tfd);
    return NULL;
}

int update_loop_start(void) {
    atomic_store(&g_running, true);
    atomic_store(&g_tick_count, 0);

    if (pthread_create(&g_thread, NULL, update_thread, NULL) != 0) {
        perror("pthread_create");
        return -1;
    }

    /* Request SCHED_FIFO priority 50 — silently skip if unprivileged */
    struct sched_param sp = { .sched_priority = 50 };
    if (pthread_setschedparam(g_thread, SCHED_FIFO, &sp) != 0 && errno != EPERM) {
        fprintf(stderr, "[update_loop] pthread_setschedparam: %s\n",
                strerror(errno));
    }
    return 0;
}

void update_loop_stop(void) {
    atomic_store(&g_running, false);
    pthread_join(g_thread, NULL);
}

uint64_t update_loop_tick_count(void) {
    return atomic_load_explicit(&g_tick_count, memory_order_relaxed);
}

#else /* non-Linux stubs */

int      update_loop_start(void) { return -1; }
void     update_loop_stop(void)  { }
uint64_t update_loop_tick_count(void) { return 0; }

#endif /* __linux__ */
