#include <stdlib.h>

#include "../include/clock.h"
#include "../include/tachy.h"
#include "../include/time.h"

struct tachy_sleep tachy_sleep(uint64_t timeout_ms) {
    uint64_t deadline = tachy_clock_timeout_ticks(timeout_ms);
    struct tachy_task *task = tachy_rt_cur_task();
    struct tachy_time_entry *entry = tachy_time_entry_new(task, deadline);
    return (struct tachy_sleep) {.entry = entry, .registered = false};
}

enum tachy_poll sleep_poll(struct tachy_sleep *self, struct tachy_sleep_result *result) {
    if (self->entry == NULL) {
        result->status = TACHY_OUT_OF_MEMORY_ERROR;
        return TACHY_POLL_READY;
    }

    if (!self->registered) {
        self->registered = true;
        struct tachy_time_driver *driver = tachy_rt_time_driver();
        tachy_time_insert_timeout(driver, self->entry);
    }

    if (!tachy_time_entry_fired(self->entry)) {
        return TACHY_POLL_PENDING;
    }

    result->status = TACHY_NO_ERROR;
    tachy_time_entry_free(self->entry);
    return TACHY_POLL_READY;
}
