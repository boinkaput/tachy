#include <time.h>

#include "../include/clock.h"

#define S_TO_MS(sec) ((sec) * 1000)
#define NS_TO_MS(nsec) ((nsec) / 1000000)

static struct {
    uint64_t start_time;
} tachy_clock = {0};

static uint64_t now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return S_TO_MS(ts.tv_sec) + NS_TO_MS(ts.tv_nsec);
}

void tachy_clock_init(void) {
    tachy_clock.start_time = now();
}

uint64_t tachy_clock_now(void) {
    assert(tachy_clock.start_time > 0);
    return now() - tachy_clock.start_time;
}

uint64_t tachy_clock_timeout_ticks(uint64_t timeout_ms) {
    return tachy_clock_now() + timeout_ms;
}
