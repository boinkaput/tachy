#include <stdlib.h>
#include <string.h>

#include "../include/task.h"

enum task_status {
    TASK_RUNNABLE = 0b1,
    TASK_RUNNING  = 0b10,
    TASK_COMPLETE = 0b100,
};

struct tachy_task {
    tachy_poll_fn poll_fn;
    size_t future_size_bytes;
    size_t output_size_bytes;
    enum task_status status;
    int ref_count;
    struct tachy_task *consumer;
    char future_and_output[];
};

static bool task_running(struct tachy_task *task) {
    return (task->status & TASK_RUNNING) != 0;
}

static bool task_complete(struct tachy_task *task) {
    return (task->status & TASK_COMPLETE) != 0;
}

static void task_transition_to_running(struct tachy_task *task) {
    assert(tachy_task_runnable(task));
    task->status &= ~TASK_RUNNABLE;
    task->status |= TASK_RUNNING;
}

static void task_transition_to_complete(struct tachy_task *task) {
    assert(task_running(task));
    task->status &= ~TASK_RUNNING;
    task->status |= TASK_COMPLETE;
}

struct tachy_task *tachy_task_new(void *future, tachy_poll_fn poll_fn,
                              size_t future_size_bytes, size_t output_size_bytes)
{
    assert(future != NULL);
    assert(poll_fn != NULL);
    assert(future_size_bytes > 0);

    size_t future_and_output_size_bytes = future_size_bytes + output_size_bytes;
    struct tachy_task *task = malloc(sizeof(struct tachy_task) + future_and_output_size_bytes);
    if (task == NULL) {
        return NULL;
    }

    *task = (struct tachy_task) {
        .poll_fn = poll_fn,
        .future_size_bytes = future_size_bytes,
        .output_size_bytes = output_size_bytes,
        .status = TASK_RUNNABLE,
        .ref_count = 1,
        .consumer = NULL
    };
    memcpy(task->future_and_output, future, future_size_bytes);
    return task;
}

void tachy_task_ref_inc(struct tachy_task *self) {
    assert(self != NULL);
    assert(self->ref_count > 0);
    self->ref_count++;
}

void tachy_task_ref_dec(struct tachy_task *self) {
    assert(self != NULL);
    assert(self->ref_count > 0);

    self->ref_count--;
    if (self->ref_count < 1) {
        free(self);
    }
}

void *tachy_task_output(struct tachy_task *self) {
    assert(self != NULL);
    return (self->output_size_bytes > 0)
        ? (void *) ((size_t) self->future_and_output + self->output_size_bytes)
        : NULL;
}

void tachy_task_register_consumer(struct tachy_task *self, struct tachy_task *consumer) {
    assert(self != NULL);

    if (self->consumer == consumer) {
        return;
    }

    if (self->consumer == NULL) {
        tachy_task_ref_inc(self);
    } else {
        tachy_task_ref_dec(self->consumer);
    }

    if (consumer != NULL) {
        tachy_task_ref_inc(consumer);
        self->consumer = consumer;
    } else {
        tachy_task_ref_dec(self);
    }
}

bool tachy_task_runnable(struct tachy_task *self) {
    return (self->status & TASK_RUNNABLE) != 0;
}

void tachy_task_make_runnable(struct tachy_task *self) {
    assert(self != NULL);
    self->status |= TASK_RUNNABLE;
}

struct tachy_task_poll_result tachy_task_poll(struct tachy_task *self, void *output) {
    assert(self != NULL);
    assert(self->future_and_output != NULL);

    void *future = self->future_and_output;
    task_transition_to_running(self);

    enum tachy_poll poll_out = self->poll_fn(future, output);
    if (poll_out == TACHY_POLL_PENDING) {
        return tachy_task_runnable(self)
            ? (struct tachy_task_poll_result) {.poll_state = TACHY_TASK_POLL_AGAIN, .consumer = NULL}
            : (struct tachy_task_poll_result) {.poll_state = TACHY_TASK_POLL_LATER, .consumer = NULL};
    }

    task_transition_to_complete(self);
    tachy_task_ref_dec(self);
    return (struct tachy_task_poll_result) {.poll_state = TACHY_TASK_POLL_AGAIN, .consumer = self->consumer};
}

bool tachy_task_try_copy_output(struct tachy_task *self, void *output) {
    assert(self != NULL);
    assert(self->future_and_output != NULL);
    assert(self->consumer != NULL);

    if (!task_complete(self)) {
        return false;
    }

    void *task_output = tachy_task_output(self);
    memcpy(output, task_output, self->output_size_bytes);
    tachy_task_register_consumer(self, NULL);
    return true;
}
