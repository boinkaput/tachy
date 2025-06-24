#pragma once

#include <stdint.h>

#include "helpers.h"

typedef int32_t tachy_state;

enum tachy_poll {
    TACHY_POLL_PENDING,
    TACHY_POLL_READY
};

typedef enum tachy_poll (*tachy_poll_fn)(void *future, void *output);

// Internal stuff

#define TACHY_RETURN_CASE_0(out) *output = out;
#define TACHY_RETURN_CASE_1()
#define TACHY_RETURN(_0) TACHY_PASTE(TACHY_RETURN_CASE_, _0)

// External API

#define tachy_begin(state)                                                      \
    tachy_state *_tachy_state = state;                                          \
    switch (*_tachy_state) {                                                    \
        case 0:

#define tachy_end                                                               \
    }                                                                           \
    return TACHY_POLL_READY

#define tachy_yield                                                             \
    do {                                                                        \
        *_tachy_state = TACHY_LABEL;                                            \
        return TACHY_POLL_PENDING;                                              \
        case TACHY_LABEL:;                                                      \
    } while(0)


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
