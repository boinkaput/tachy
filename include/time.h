#pragma once

#include <stdint.h>

#include "task.h"

#define TIME_WHEEL_LEVELS 6
#define TIME_SLOTS_PER_LEVEL 64
#define TIME_MAX_TIMEOUT_MS ((((uint64_t) 1) << (6 * TIME_WHEEL_LEVELS)) - 1)

struct time_wheel_level {
    struct time_entry *slots[TIME_SLOTS_PER_LEVEL];
};

struct time_driver {
    uint64_t elapsed;
    struct time_wheel_level wheel_levels[TIME_WHEEL_LEVELS];
    uint64_t active_slot_bitmap[TIME_WHEEL_LEVELS];
    struct time_entry *pending_list_head;
};

static inline struct time_driver time_driver_new(void) {
    return (struct time_driver) {0};
}

struct time_entry *time_entry_new(struct task *task, uint64_t deadline);
void time_entry_free(struct time_entry *entry);
bool time_entry_fired(struct time_entry *entry);
void time_insert_timeout(struct time_driver *driver, struct time_entry *entry);
void time_remove_timeout(struct time_driver *driver, struct time_entry *entry);
uint64_t time_next_expiration(struct time_driver *driver);
void time_process_at(struct time_driver *driver, uint64_t now_ms);
struct task *time_next_pending_task(struct time_driver *driver);
