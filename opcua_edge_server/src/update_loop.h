#ifndef UPDATE_LOOP_H
#define UPDATE_LOOP_H

#include <stdint.h>

/* Start the 1 kHz timerfd update thread.
 * Sets SCHED_FIFO priority=50 if CAP_SYS_NICE is available; falls back
 * gracefully to default scheduling without error if privilege is missing.
 * Returns 0 on success, -1 on error. */
int update_loop_start(void);

void     update_loop_stop(void);
uint64_t update_loop_tick_count(void);

#endif /* UPDATE_LOOP_H */
