#include <stdlib.h>

#include "../include/clock.h"
#include "../include/runtime.h"
#include "../include/tachy.h"
#include "../include/time_driver.h"

struct tachy_sleep_handle tachy_sleep(uint64_t timeout_ms) {
    uint64_t deadline = clock_timeout_ticks(timeout_ms);
    struct task *task = rt_cur_task();
    struct time_entry *entry = time_entry_new(task, deadline);
    return (struct tachy_sleep_handle) {.entry = entry, .registered = false};
}

enum tachy_poll tachy_sleep_poll(struct tachy_sleep_handle *handle, struct tachy_sleep_result *result) {
    if (handle->entry == NULL) {
        result->status = TACHY_OUT_OF_MEMORY_ERROR;
        return TACHY_POLL_READY;
    }

    if (!handle->registered) {
        handle->registered = true;
        struct time_driver *driver = rt_time_driver();
        time_insert_timeout(driver, handle->entry);
    }

    if (!time_entry_fired(handle->entry)) {
        return TACHY_POLL_PENDING;
    }

    result->status = TACHY_NO_ERROR;
    time_entry_free(handle->entry);
    return TACHY_POLL_READY;
}
