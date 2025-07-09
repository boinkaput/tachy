#include <stddef.h>

#include "../include/tachy.h"
#include "../include/task.h"

struct tachy_join_handle tachy_join_handle(struct tachy_task *task, enum tachy_error_kind error) {
    if (task != NULL) {
        tachy_task_ref_inc(task);
    }
    return (struct tachy_join_handle) {.task = task, .error = error};
}

void tachy_join_handle_detach(struct tachy_join_handle *handle) {
    tachy_task_register_consumer(handle->task, NULL);
    tachy_task_ref_dec(handle->task);
    handle->task = NULL;
}

enum tachy_poll tachy_join_handle_poll(struct tachy_join_handle *handle, void *output) {
    assert(handle != NULL);

    if (handle->error != TACHY_NO_ERROR) {
        return TACHY_POLL_READY;
    }

    assert(handle->task != NULL);

    struct tachy_task *task = tachy_rt_cur_task();
    tachy_task_register_consumer(handle->task, task);
    if (!tachy_task_try_copy_output(handle->task, output)) {
        return TACHY_POLL_PENDING;
    }
    tachy_task_register_consumer(handle->task, NULL);

    tachy_task_ref_dec(handle->task);
    return TACHY_POLL_READY;
}
