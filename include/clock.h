#pragma once

#include <stdint.h>

#define S_TO_MS(sec) ((sec) * 1000)
#define NS_TO_MS(nsec) ((nsec) / 1000000)

void clock_init(void);
uint64_t clock_now(void);
uint64_t clock_timeout_ticks(uint64_t timeout_ms);
