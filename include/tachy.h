#pragma once

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
    #error Tachy requires c99 or later
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "macros.h"

typedef int32_t tachy_state;

enum tachy_poll {
    TACHY_POLL_PENDING,
    TACHY_POLL_READY
};

typedef enum tachy_poll (*tachy_poll_fn)(void *future, void *output);

enum tachy_future_state {
    TACHY_FUTURE_CREATED,

    TACHY_YIELDED,

    TACHY_JOIN_REGISTERED,
    TACHY_JOIN_COMPLETED,
    TACHY_JOIN_DETACHED,

    TACHY_SLEEP_REGISTERED,
    TACHY_SLEEP_COMPLETED,
    TACHY_SLEEP_CANCELED,

    TACHY_PLACEHOLDER_STATE,
};

enum tachy_error {
    TACHY_NO_ERROR = 0,
    TACHY_OUT_OF_MEMORY_ERROR = TACHY_PLACEHOLDER_STATE,
};

struct tachy_yield_handle {
    tachy_state state;
};

struct tachy_join_handle {
    struct task *task;
    tachy_state state;
};

struct tachy_sleep_handle {
    struct time_entry *entry;
    tachy_state state;
};

// Coroutines

#define TACHY_RETURN_CASE_0(out) *output = out;
#define TACHY_RETURN_CASE_1()
#define TACHY_RETURN(_0) TACHY_PASTE(TACHY_RETURN_CASE_, _0)

#define tachy_begin(state)                                                      \
    tachy_state *_tachy_state = state;                                          \
    switch (*_tachy_state) {                                                    \
        case 0:

#define tachy_end                                                               \
    }                                                                           \
    return TACHY_POLL_READY


#define tachy_return(...)                                                       \
    do {                                                                        \
        *_tachy_state = TACHY_LABEL;                                            \
        TACHY_RETURN(TACHY_ISEMPTY(__VA_ARGS__))(__VA_ARGS__)                   \
        return TACHY_POLL_READY;                                                \
    } while(0)

#define tachy_await(poll)                                                       \
    do {                                                                        \
        *_tachy_state = TACHY_LABEL;                                            \
        TACHY_FALLTHROUGH;                                                      \
        case TACHY_LABEL:                                                       \
        if ((poll) == TACHY_POLL_PENDING) return TACHY_POLL_PENDING;            \
    } while(0)

// Task

#define tachy_block_on(future, poll_fn, output)                                 \
    tachy__block_on(future, poll_fn, sizeof(*(future)), output)

#define tachy_spawn(future, poll_fn, output_size_bytes)                         \
    tachy__spawn(future, poll_fn, sizeof(*(future)), output_size_bytes)

#define tachy_spawn_no_join(future, poll_fn, output_size_bytes)                 \
    tachy__spawn_no_join(future, poll_fn, sizeof(*(future)), output_size_bytes)

bool tachy_init(void);
void tachy__block_on(void *future, tachy_poll_fn poll_fn, size_t future_size_bytes, void *output);
struct tachy_join_handle tachy__spawn(void *future, tachy_poll_fn poll_fn, size_t future_size_bytes, size_t output_size_bytes);
int tachy__spawn_no_join(void *future, tachy_poll_fn poll_fn, size_t future_size_bytes, size_t output_size_bytes);

// Yield

struct tachy_yield_handle tachy_yield(void);
enum tachy_poll tachy_yield_poll(struct tachy_yield_handle *handle, TACHY_UNUSED void *output);

// Join

enum tachy_poll tachy_join_poll(struct tachy_join_handle *handle, void *output);
void tachy_join_detach(struct tachy_join_handle *handle);

// Sleep

struct tachy_sleep_handle tachy_sleep(uint64_t timeout_ms);
enum tachy_poll tachy_sleep_poll(struct tachy_sleep_handle *handle, TACHY_UNUSED void *output);
void tachy_cancel_sleep(struct tachy_sleep_handle *handle);
