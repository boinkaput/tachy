#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../include/task.h"

enum task_state {
    TASK_RUNNABLE = 0b1,
    TASK_RUNNING  = 0b10,
    TASK_WAITING  = 0b100,
    TASK_COMPLETE = 0b1000,
};

struct tachy_task {
    tachy_poll_fn poll_fn;
    int ref_count;
    enum task_state state;
    struct tachy_task *consumer;
    size_t future_size_bytes;
    size_t output_size_bytes;
    char future_or_output[];
};

static bool running(struct tachy_task *task) {
    return (task->state & TASK_RUNNING) != 0;
}

static bool waiting(struct tachy_task *task) {
    return (task->state & TASK_WAITING) != 0;
}

static bool complete(struct tachy_task *task) {
    return (task->state & TASK_COMPLETE) != 0;
}

static void transition_to_running(struct tachy_task *task) {
    assert(tachy_task_runnable(task));
    task->state &= ~TASK_RUNNABLE;
    task->state |= TASK_RUNNING;
}

static void transition_to_waiting(struct tachy_task *task) {
    assert(running(task));
    task->state &= ~TASK_RUNNING;
    task->state |= TASK_WAITING;
}

static void transition_to_complete(struct tachy_task *task) {
    assert(running(task));
    task->state &= ~TASK_RUNNING;
    task->state |= TASK_COMPLETE;
}

static void *future(struct tachy_task *task) {
    assert(!complete(task));
    return task->future_or_output;
}

struct tachy_task *tachy_task_new(void *future, tachy_poll_fn poll_fn,
                              size_t future_size_bytes, size_t output_size_bytes)
{
    assert(future != NULL);
    assert(poll_fn != NULL);
    assert(future_size_bytes > 0);

    size_t future_or_output_bytes = TACHY_MAX(future_size_bytes, output_size_bytes);
    struct tachy_task *task = malloc(sizeof(struct tachy_task) + future_or_output_bytes);
    if (task == NULL) {
        return NULL;
    }

    *task = (struct tachy_task) {
        .poll_fn = poll_fn,
        .ref_count = 1,
        .state = TASK_RUNNABLE,
        .consumer = NULL,
        .future_size_bytes = future_size_bytes,
        .output_size_bytes = output_size_bytes,
    };
    memcpy(task->future_or_output, future, future_size_bytes);
    return task;
}

void tachy_task_ref_inc(struct tachy_task *task) {
    assert(task != NULL);
    assert(task->ref_count > 0);
    task->ref_count++;
}

void tachy_task_ref_dec(struct tachy_task *task) {
    assert(task != NULL);
    assert(task->ref_count > 0);

    task->ref_count--;
    if (task->ref_count < 1) {
        free(task);
    }
}

void *tachy_task_output(struct tachy_task *task) {
    assert(task != NULL);
    return (task->output_size_bytes > 0) ? task->future_or_output : NULL;
}

void tachy_task_register_consumer(struct tachy_task *task, struct tachy_task *consumer) {
    assert(task != NULL);

    if (task->consumer != NULL) {
        tachy_task_ref_dec(task->consumer);
    }

    if (consumer != NULL) {
        tachy_task_ref_inc(consumer);
    }
    task->consumer = consumer;
}

bool tachy_task_runnable(struct tachy_task *task) {
    return (task->state & TASK_RUNNABLE) != 0;
}

void tachy_task_make_runnable(struct tachy_task *task) {
    assert(task != NULL);
    assert(waiting(task));
    task->state &= ~TASK_WAITING;
    task->state |= TASK_RUNNABLE;
}

struct tachy_task_poll_result tachy_task_poll(struct tachy_task *task, void *output) {
    assert(task != NULL);
    assert(task->poll_fn != NULL);
    assert(task->future_or_output != NULL);

    transition_to_running(task);

    void *fut = future(task);
    enum tachy_poll poll_out = task->poll_fn(fut, output);
    if (poll_out == TACHY_POLL_PENDING) {
        transition_to_waiting(task);
    } else {
        transition_to_complete(task);
        tachy_task_ref_dec(task);
    }

    return (struct tachy_task_poll_result) {.state = poll_out, .consumer = task->consumer};
}

bool tachy_task_try_copy_output(struct tachy_task *task, void *output) {
    assert(task != NULL);
    assert(task->future_or_output != NULL);
    assert(task->consumer != NULL);

    if (!complete(task)) {
        return false;
    }

    void *out = tachy_task_output(task);
    memcpy(output, out, task->output_size_bytes);
    return true;
}
