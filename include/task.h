#pragma once

#include <stddef.h>

#include "tachy.h"

enum task_state {
    TASK_RUNNABLE = 0b1,
    TASK_RUNNING  = 0b10,
    TASK_WAITING  = 0b100,
    TASK_COMPLETE = 0b1000,
};

struct task {
    struct task *next;
    tachy_poll_fn poll_fn;
    int ref_count;
    enum task_state state;
    struct task *consumer;
    size_t future_size_bytes;
    size_t output_size_bytes;
    char future_or_output[];
};

struct task *task_new(void *future, tachy_poll_fn poll_fn, size_t future_size_bytes, size_t output_size_bytes);
enum tachy_poll task_poll(struct task *task, void *output);
void task_register_consumer(struct task *task, struct task *consumer);
bool task_try_copy_output(struct task *task, void *output);
void task_ref_inc(struct task *task);
void task_ref_dec(struct task *task);
bool task_runnable(struct task *task);
void task_make_runnable(struct task *task);
void *task_output(struct task *task);
