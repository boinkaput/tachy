#include <stddef.h>

#include "../include/tachy.h"
#include "../include/task.h"

struct tachy_join_handle tachy_join_handle(struct tachy_task *task) {
    return (struct tachy_join_handle) {.task = task};
}

struct tachy_join_result tachy_join_result(void *output) {
    return (struct tachy_join_result) {.status = TACHY_NO_ERROR, .output = output};
}

enum tachy_poll tachy_join_handle_poll(struct tachy_join_handle *self, struct tachy_join_result *result) {
    assert(self != NULL);

    if (self->task == NULL) {
        result->status = TACHY_OUT_OF_MEMORY_ERROR;
        return TACHY_POLL_READY;
    }

    struct tachy_task *task = tachy_rt_cur_task();
    tachy_task_register_consumer(self->task, task);
    if (!tachy_task_try_copy_output(self->task, result->output)) {
        return TACHY_POLL_PENDING;
    }

    result->status = TACHY_NO_ERROR;
    return TACHY_POLL_READY;
}
