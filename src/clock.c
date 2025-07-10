#include <assert.h>
#include <time.h>

#include "../include/clock.h"

#define S_TO_MS(sec) ((sec) * 1000)
#define NS_TO_MS(nsec) ((nsec) / 1000000)

static struct {
    uint64_t start_time;
} linux_clock = {0};

static uint64_t now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return S_TO_MS(ts.tv_sec) + NS_TO_MS(ts.tv_nsec);
}

void clock_init(void) {
    linux_clock.start_time = now();
}

uint64_t clock_now(void) {
    assert(linux_clock.start_time > 0);
    return now() - linux_clock.start_time;
}

uint64_t clock_timeout_ticks(uint64_t timeout_ms) {
    return clock_now() + timeout_ms;
}
