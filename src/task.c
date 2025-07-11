#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../include/task.h"
#include "../include/tachy.h"

static bool is_runnable(struct task *task) {
    return (task->state & TASK_RUNNABLE) != 0;
}

static bool is_running(struct task *task) {
    return (task->state & TASK_RUNNING) != 0;
}

static bool is_waiting(struct task *task) {
    return (task->state & TASK_WAITING) != 0;
}

static bool is_complete(struct task *task) {
    return (task->state & TASK_COMPLETE) != 0;
}

static void transition_to_runnable(struct task *task) {
    assert(is_waiting(task));
    task->state &= ~TASK_WAITING;
    task->state |= TASK_RUNNABLE;
}

static void transition_to_running(struct task *task) {
    assert(is_runnable(task));
    task->state &= ~TASK_RUNNABLE;
    task->state |= TASK_RUNNING;
}

static void transition_to_waiting(struct task *task) {
    assert(is_running(task));
    task->state &= ~TASK_RUNNING;
    task->state |= TASK_WAITING;
}

static void transition_to_complete(struct task *task) {
    assert(is_running(task));
    task->state &= ~TASK_RUNNING;
    task->state |= TASK_COMPLETE;
}

static void *future(struct task *task) {
    assert(!is_complete(task));
    return task->future_or_output;
}

struct task *task_new(void *future, tachy_poll_fn poll_fn,
                      size_t future_size_bytes, size_t output_size_bytes)
{
    assert(future != NULL);
    assert(poll_fn != NULL);
    assert(future_size_bytes > 0);

    size_t future_or_output_bytes = TACHY_MAX(future_size_bytes, output_size_bytes);
    struct task *task = malloc(sizeof(struct task) + future_or_output_bytes);
    if (task == NULL) {
        return NULL;
    }

    *task = (struct task) {
        .next = NULL,
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

void task_ref_inc(struct task *task) {
    assert(task != NULL);
    assert(task->ref_count > 0);
    task->ref_count++;
}

void task_ref_dec(struct task *task) {
    assert(task != NULL);
    assert(task->ref_count > 0);

    task->ref_count--;
    if (task->ref_count < 1) {
        free(task);
    }
}

void *task_output(struct task *task) {
    assert(task != NULL);
    return (task->output_size_bytes > 0) ? task->future_or_output : NULL;
}

void task_register_consumer(struct task *task, struct task *consumer) {
    assert(task != NULL);

    if (task->consumer != NULL) {
        task_ref_dec(task->consumer);
    }

    if (consumer != NULL) {
        task_ref_inc(consumer);
    }
    task->consumer = consumer;
}

bool task_runnable(struct task *task) {
    assert(task != NULL);
    return is_runnable(task);
}

void task_make_runnable(struct task *task) {
    assert(task != NULL);
    transition_to_runnable(task);
}

enum tachy_poll task_poll(struct task *task, void *output) {
    assert(task != NULL);
    assert(task->poll_fn != NULL);
    assert(task->future_or_output != NULL);

    transition_to_running(task);

    void *fut = future(task);
    enum tachy_poll poll_out = task->poll_fn(fut, output);
    if (poll_out == TACHY_POLL_PENDING) {
        transition_to_waiting(task);
    } else {
        if (task->consumer != NULL) {
            rt_wake_task(task->consumer);
        }
        transition_to_complete(task);
        task_ref_dec(task);
    }

    return poll_out;
}

bool task_try_copy_output(struct task *task, void *output) {
    assert(task != NULL);
    assert(task->future_or_output != NULL);
    assert(task->consumer != NULL);

    if (!is_complete(task)) {
        return false;
    }

    void *out = task_output(task);
    memcpy(output, out, task->output_size_bytes);
    return true;
}
