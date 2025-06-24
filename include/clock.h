#pragma once

#include <stdint.h>

void tachy_clock_init(void);

uint64_t tachy_clock_now(void);

uint64_t tachy_clock_timeout_ticks(uint64_t timeout_ms);
