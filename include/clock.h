#pragma once

#include <stdint.h>

void clock_init(void);
uint64_t clock_now(void);
uint64_t clock_timeout_ticks(uint64_t timeout_ms);
