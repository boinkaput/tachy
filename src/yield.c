#include <assert.h>

#include "../include/runtime.h"
#include "../include/tachy.h"

struct tachy_yield_handle tachy_yield(void) {
    return (struct tachy_yield_handle) {.state = TACHY_FUTURE_CREATED};
}

enum tachy_poll tachy_yield_poll(struct tachy_yield_handle *handle, TACHY_UNUSED void *output) {
    assert(handle != NULL);

    if (handle->state == TACHY_FUTURE_CREATED) {
        handle->state = TACHY_YIELDED;
        struct task *task = rt_cur_task();
        rt_defer_task(task);
        return TACHY_POLL_PENDING;
    }
    return TACHY_POLL_READY;
}
