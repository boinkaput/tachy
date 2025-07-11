#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "../include/time_driver.h"

#define SLOT_MASK ((1 << 6) - 1)
#define BIT_SET(bit_idx) (((uint64_t) 1) << (bit_idx))

static int clz64(uint64_t num) {
    int count = 0;
    for (int i = 63; i >= 0; i--) {
        if ((num >> i) & 1) {
            break;
        }
        count++;
    }
    return count;
}

static int ctz64(uint64_t num) {
    if (num == 0) {
        return 64;
    }

    int count = 0;
    while ((num & 1) == 0) {
        num >>= 1;
        count++;
    }
    return count;
}

static uint64_t rotate_r64(uint64_t num, int n) {
    uint64_t mask = BIT_SET(n) - 1;
    return (num >> n) | ((num & mask) << (64 - n));
}

static int level_for(uint64_t timeout) {
    size_t masked = timeout | SLOT_MASK;
    if (masked >= TIME_MAX_TIMEOUT_MS) {
        masked = TIME_MAX_TIMEOUT_MS - 1;
    }

    size_t leading_zeros = clz64(masked);
    size_t significant = 63 - leading_zeros;
    return significant / TIME_WHEEL_LEVELS;
}

static uint64_t level_resolution(uint64_t slot_res) {
    return TIME_SLOTS_PER_LEVEL * slot_res;
}

static int slot_for(int level, uint64_t deadline) {
    return (deadline >> (6 * level)) & SLOT_MASK;
}

static uint64_t slot_resolution(int level) {
    return ((uint64_t) 1) << (6 * level);
}

static int slot_next_occupied(int level, uint64_t now, uint64_t slot_bitmap) {
    if (slot_bitmap == 0) {
        return -1;
    }

    int now_slot = slot_for(level, now);
    uint64_t adjusted_slot_bitmap = rotate_r64(slot_bitmap, now_slot);
    int trailing_zeros = ctz64(adjusted_slot_bitmap);
    return (now_slot + trailing_zeros) & SLOT_MASK;
}

void time_insert_timeout(struct time_driver *driver, struct time_entry *entry) {
    if (driver->elapsed >= entry->deadline) {
        time_entry_make_fired(entry);
        return;
    }

    uint64_t timeout = entry->deadline - driver->elapsed;
    int l = level_for(timeout);
    int s = slot_for(l, entry->deadline);

    struct time_wheel_level *level = &driver->wheel_levels[l];
    time_entry_list_push_front(&level->slots[s], entry);
    driver->active_slot_bitmap[l] |= BIT_SET(s);
}

void time_remove_timeout(struct time_driver *driver, struct time_entry *entry) {
    if (!time_entry_fired(entry)) {
        if (time_entry_pending(entry)) {
            time_entry_list_remove(&driver->pending, entry);
        } else {
            uint64_t timeout = entry->deadline - driver->elapsed;
            int l = level_for(timeout);
            int s = slot_for(l, entry->deadline);
    
            struct time_wheel_level *level = &driver->wheel_levels[l];
            time_entry_list_remove(&level->slots[s], entry);
            if (time_entry_list_empty(&level->slots[s])) {
                driver->active_slot_bitmap[l] &= ~BIT_SET(s);
            }
        }
    }
    time_entry_free(entry);
}

uint64_t time_next_expiration(struct time_driver *driver) {
    for (int level = 0; level < TIME_WHEEL_LEVELS; level++) {
        int slot = slot_next_occupied(level, driver->elapsed, driver->active_slot_bitmap[level]);
        if (slot != -1) {
            uint64_t slot_res = slot_resolution(level);
            uint64_t level_res = level_resolution(slot_res);
            uint64_t level_start = driver->elapsed & ~(level_res - 1);
            return level_start + (slot * slot_res);
        }
    }
    return 0;
}

static void slot_process_expiration(struct time_driver *driver,
                                    int level, int slot, uint64_t now)
{
    struct time_entry_list *entry_list = &driver->wheel_levels[level].slots[slot];
    for (struct time_entry *entry = time_entry_list_pop_front(entry_list);
         entry != NULL; entry = time_entry_list_pop_front(entry_list)) {
        if (now >= entry->deadline) {
            time_entry_make_pending(entry);
            time_entry_list_push_front(&driver->pending, entry);
        } else {
            time_insert_timeout(driver, entry);
        }
    }
    driver->active_slot_bitmap[level] &= ~BIT_SET(slot);
}

void time_process_at(struct time_driver *driver, uint64_t now) {
    for (int level = 0; level < TIME_WHEEL_LEVELS; level++) {
        uint64_t slot_res = slot_resolution(level);
        uint64_t level_res = level_resolution(slot_res);
        uint64_t level_start = driver->elapsed & ~(level_res - 1);

        int now_slot = slot_for(level, driver->elapsed);
        uint64_t adjusted_slot_bitmap = rotate_r64(driver->active_slot_bitmap[level], now_slot);
        while (adjusted_slot_bitmap != 0) {
            int trailing_zeros = ctz64(adjusted_slot_bitmap);
            now_slot += trailing_zeros;

            int next_slot = now_slot & SLOT_MASK;
            uint64_t slot_start = level_start + (next_slot * slot_res);
            if (slot_start > now) {
                break;
            }

            driver->elapsed = slot_start;
            slot_process_expiration(driver, level, next_slot, now);
            if (trailing_zeros < 63) {
                adjusted_slot_bitmap >>= (trailing_zeros + 1);
            } else {
                adjusted_slot_bitmap = 0;
            }
        }
    }
    driver->elapsed = now;
}

struct task *time_next_pending_task(struct time_driver *driver) {
    struct time_entry *entry = time_entry_list_pop_front(&driver->pending);
    if (entry == NULL) {
        return NULL;
    }

    time_entry_make_fired(entry);
    return entry->task;
}
