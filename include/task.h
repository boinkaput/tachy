#pragma once

#include <stddef.h>

#include "tachy.h"

struct task *task_new(void *future, tachy_poll_fn poll_fn, size_t future_size_bytes, size_t output_size_bytes);
void task_ref_inc(struct task *task);
void task_ref_dec(struct task *task);
void *task_output(struct task *task);
bool task_runnable(struct task *task);
void task_make_runnable(struct task *task);
void task_register_consumer(struct task *task, struct task *consumer);
enum tachy_poll task_poll(struct task *task, void *output);
bool task_try_copy_output(struct task *task, void *output);
