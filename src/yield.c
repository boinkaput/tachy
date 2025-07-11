#include "../include/runtime.h"
#include "../include/tachy.h"

struct tachy_yield_handle tachy_yield(void) {
    return (struct tachy_yield_handle) {0};
}

enum tachy_poll tachy_yield_poll(struct tachy_yield_handle *handle, TACHY_UNUSED void *output) {
    if (handle->yielded) {
        return TACHY_POLL_READY;
    }

    handle->yielded = true;
    struct task *task = rt_cur_task();
    rt_defer_task(task);
    return TACHY_POLL_PENDING;
}
