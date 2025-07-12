#include <assert.h>
#include <stddef.h>

#include "../include/join.h"
#include "../include/runtime.h"
#include "../include/task.h"
#include "../include/tachy.h"

struct tachy_join_handle tachy_join(struct task *task, int state) {
    if (task != NULL) {
        task_ref_inc(task);
    }
    return (struct tachy_join_handle) {.task = task, .state = state};
}

void tachy_join_detach(struct tachy_join_handle *handle) {
    task_register_consumer(handle->task, NULL);
    task_ref_dec(handle->task);
    handle->task = NULL;
}

enum tachy_poll tachy_join_poll(struct tachy_join_handle *handle, void *output) {
    assert(handle != NULL);

    if (handle->state == TACHY_FUTURE_CREATED) {
        struct task *task = rt_cur_task();
        task_register_consumer(handle->task, task);
        handle->state = TACHY_JOIN_REGISTERED;
    }

    if (handle->state == TACHY_JOIN_REGISTERED) {
        if (!task_try_copy_output(handle->task, output)) {
            return TACHY_POLL_PENDING;
        }

        task_register_consumer(handle->task, NULL);
        handle->state = TACHY_JOIN_COMPLETED;
        task_ref_dec(handle->task);
    }
    return TACHY_POLL_READY;
}
