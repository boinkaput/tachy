#pragma once

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
    #error Tachy requires c99 or later
#endif

#include <assert.h>
#include <stdbool.h>

#include "async.h"
#include "helpers.h"

enum tachy_error_kind {
    TACHY_NO_ERROR,
    TACHY_OUT_OF_MEMORY_ERROR
};

struct tachy_join_handle {
    struct tachy_task *task;
    enum tachy_error_kind error;
};

struct tachy_sleep {
    struct tachy_time_entry *entry;
    bool registered;
};

struct tachy_sleep_result {
    enum tachy_error_kind status;
};

// Runtime

#define tachy_rt_block_on(future, poll_fn, output) \
    tachy_rt_block_on_(future, poll_fn, sizeof(*(future)), output)

#define tachy_rt_spawn(future, poll_fn, output_size_bytes) \
    tachy_rt_spawn_(future, poll_fn, sizeof(*(future)), output_size_bytes)

bool tachy_rt_init(void);

void tachy_rt_block_on_(void *future, tachy_poll_fn poll_fn, size_t future_size_bytes, void *output);

struct tachy_join_handle tachy_rt_spawn_(void *future, tachy_poll_fn poll_fn, size_t future_size_bytes, size_t output_size_bytes);

struct tachy_time_driver *tachy_rt_time_driver(void);

struct tachy_task *tachy_rt_cur_task(void);

// Join

struct tachy_join_handle tachy_join_handle(struct tachy_task *task, enum tachy_error_kind error);

void tachy_join_handle_detach(struct tachy_join_handle *handle);

enum tachy_poll tachy_join_handle_poll(struct tachy_join_handle *handle, void *output);

// Sleep

struct tachy_sleep tachy_sleep(uint64_t timeout_ms);

enum tachy_poll sleep_poll(struct tachy_sleep *self, struct tachy_sleep_result *result);
