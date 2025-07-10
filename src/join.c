#include <assert.h>
#include <stddef.h>

#include "../include/runtime.h"
#include "../include/task.h"
#include "../include/tachy.h"

struct tachy_join_handle tachy_join_handle(struct task *task, enum tachy_error_kind error) {
    if (task != NULL) {
        task_ref_inc(task);
    }
    return (struct tachy_join_handle) {.task = task, .error = error};
}

void tachy_join_handle_detach(struct tachy_join_handle *handle) {
    task_register_consumer(handle->task, NULL);
    task_ref_dec(handle->task);
    handle->task = NULL;
}

enum tachy_poll tachy_join_handle_poll(struct tachy_join_handle *handle, void *output) {
    assert(handle != NULL);

    if (handle->error != TACHY_NO_ERROR) {
        return TACHY_POLL_READY;
    }

    assert(handle->task != NULL);

    struct task *task = rt_cur_task();
    task_register_consumer(handle->task, task);
    if (!task_try_copy_output(handle->task, output)) {
        return TACHY_POLL_PENDING;
    }
    task_register_consumer(handle->task, NULL);

    task_ref_dec(handle->task);
    return TACHY_POLL_READY;
}
