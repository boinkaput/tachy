#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "async.h"

struct tachy_task_poll_result {
    enum {
        TACHY_TASK_POLL_LATER,
        TACHY_TASK_POLL_AGAIN,
        TACHY_TASK_POLL_COMPLETE
    } poll_state;
    struct tachy_task *consumer;
};

struct tachy_task *tachy_task_new(void *future, tachy_poll_fn poll_fn, size_t future_size_bytes, size_t output_size_bytes);

void tachy_task_ref_inc(struct tachy_task *self);

void tachy_task_ref_dec(struct tachy_task *self);

void *tachy_task_output(struct tachy_task *self);

bool tachy_task_runnable(struct tachy_task *self);

void tachy_task_make_runnable(struct tachy_task *self);

void tachy_task_register_consumer(struct tachy_task *self, struct tachy_task *consumer);

struct tachy_task_poll_result tachy_task_poll(struct tachy_task *self, void *output);

bool tachy_task_try_copy_output(struct tachy_task *self, void *output);
