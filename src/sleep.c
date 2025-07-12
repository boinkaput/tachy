#include <assert.h>

#include "../include/clock.h"
#include "../include/runtime.h"
#include "../include/tachy.h"
#include "../include/time_driver.h"

struct tachy_sleep_handle tachy_sleep(struct tachy_sleep_duration duration) {
    uint64_t deadline = clock_timeout_ticks(S_TO_MS(duration.secs) + duration.msecs);
    struct task *task = rt_cur_task();
    struct time_entry *entry = time_entry_new(task, deadline);
    return (struct tachy_sleep_handle) {
        .entry = entry,
        .state = ((entry != NULL) ? TACHY_FUTURE_CREATED : TACHY_OUT_OF_MEMORY_ERROR)
    };
}

enum tachy_poll tachy_sleep_poll(struct tachy_sleep_handle *handle, TACHY_UNUSED void *output) {
    assert(handle != NULL);
    assert(handle->state != TACHY_SLEEP_CANCELED);

    if (handle->state == TACHY_FUTURE_CREATED) {
        handle->state = TACHY_SLEEP_REGISTERED;
        struct time_driver *driver = rt_time_driver();
        time_insert_timeout(driver, handle->entry);
    }

    if (handle->state == TACHY_SLEEP_REGISTERED) {
        if (!time_entry_fired(handle->entry)) {
            return TACHY_POLL_PENDING;
        }

        handle->state = TACHY_SLEEP_COMPLETED;
        time_entry_free(handle->entry);
    }
    return TACHY_POLL_READY;
}

void tachy_cancel_sleep(struct tachy_sleep_handle *handle) {
    struct time_driver *driver = rt_time_driver();
    time_remove_timeout(driver, handle->entry);
    handle->entry = NULL;
    handle->state = TACHY_SLEEP_CANCELED;
}
