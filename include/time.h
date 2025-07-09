#pragma once

#include <assert.h>
#include <stdint.h>

#include "../include/task.h"

#define TACHY_TIME_WHEEL_LEVELS 6
#define TACHY_TIME_SLOTS_PER_LEVEL 64
#define TACHY_TIME_MAX_TIMEOUT_MS ((((uint64_t) 1) << (6 * TACHY_TIME_WHEEL_LEVELS)) - 1)

struct tachy_time_wheel_level {
    struct tachy_time_entry *slots[TACHY_TIME_SLOTS_PER_LEVEL];
};

struct tachy_time_driver {
    uint64_t elapsed;
    struct tachy_time_wheel_level wheel_levels[TACHY_TIME_WHEEL_LEVELS];
    uint64_t active_slot_bitmap[TACHY_TIME_WHEEL_LEVELS];
    struct tachy_time_entry *pending_list_head;
};

static inline struct tachy_time_driver tachy_time_driver_new(void) {
    return (struct tachy_time_driver) {0};
}

struct tachy_time_entry *tachy_time_entry_new(struct tachy_task *task, uint64_t deadline);

void tachy_time_entry_free(struct tachy_time_entry *entry);

bool tachy_time_entry_fired(struct tachy_time_entry *entry);

void tachy_time_insert_timeout(struct tachy_time_driver *driver, struct tachy_time_entry *entry);

void tachy_time_remove_timeout(struct tachy_time_driver *driver, struct tachy_time_entry *entry);

uint64_t tachy_time_next_expiration(struct tachy_time_driver *driver);

void tachy_time_process_at(struct tachy_time_driver *driver, uint64_t now_ms);

struct tachy_task *tachy_time_next_pending_task(struct tachy_time_driver *driver);
