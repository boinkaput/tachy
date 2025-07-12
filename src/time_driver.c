#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "../include/runtime.h"
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
        uint64_t timeout = entry->deadline - driver->elapsed;
        int l = level_for(timeout);
        int s = slot_for(l, entry->deadline);

        struct time_wheel_level *level = &driver->wheel_levels[l];
        time_entry_list_remove(&level->slots[s], entry);
        if (time_entry_list_empty(&level->slots[s])) {
            driver->active_slot_bitmap[l] &= ~BIT_SET(s);
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
            time_entry_make_fired(entry);
            rt_wake_task(entry->task);
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

#ifdef TACHY_TEST
#include <stdio.h>

#include "../include/task.h"

static void test_clz64(void) {
    assert(clz64(0) == 64);
    assert(clz64(1ULL << 63) == 0);
    assert(clz64(1ULL << 0) == 63);
    assert(clz64((1ULL << 62) | (1ULL << 10)) == 1);
    assert(clz64((1ULL << 63)) == 0);
}

static void test_ctz64(void) {
    assert(ctz64(0) == 64);
    assert(ctz64(1ULL << 0) == 0);
    assert(ctz64(1ULL << 10) == 10);
    assert(ctz64((1ULL << 5) | (1ULL << 10)) == 5);
}

static void test_rotate_r64(void) {
    assert(rotate_r64(0b1ULL, 1) == (1ULL << 63));
    assert(rotate_r64(0b1ULL << 5, 5) == 1ULL);
    assert(rotate_r64(0xFFFFFFFFFFFFFFFFULL, 10) == 0xFFFFFFFFFFFFFFFFULL);
}

static void test_level_for(void) {
    assert(level_for(1) == 0);
    assert(level_for(63) == 0);
    assert(level_for(64) == 1);
    assert(level_for(4095) == 1);
    assert(level_for(4096) == 2);
    assert(level_for(262143) == 2);
}

static void test_level_resolution(void) {
    assert(level_resolution(1) == 64);
    assert(level_resolution(64) == 4096);
    assert(level_resolution(4096) == 262144);
}

static void test_slot_for(void) {
    assert(slot_for(0, 0) == 0);
    assert(slot_for(0, 63) == 63);
    assert(slot_for(1, 64) == 1);
    assert(slot_for(1, 4095) == 63);
}

static void test_slot_resolution(void) {
    assert(slot_resolution(0) == 1);
    assert(slot_resolution(1) == 64);
    assert(slot_resolution(2) == 4096);
    assert(slot_resolution(3) == 262144);
}

static void test_slot_next_occupied(void) {
    uint64_t bitmap = (1ULL << 5) | (1ULL << 10) | (1ULL << 20);

    int next = slot_next_occupied(0, 0, bitmap);
    assert(next == 5);

    next = slot_next_occupied(0, 6, bitmap);
    assert(next == 10);

    next = slot_next_occupied(0, 21, bitmap);
    assert(next == 5);
}

static void test_slot_process_expiration(void) {
    struct time_driver driver = {0};
    struct task task1 = {.state = TASK_WAITING};
    struct task task2 = {.state = TASK_WAITING};
    struct time_entry e1 = {.task = &task1, .deadline = 1000};
    struct time_entry e2 = {.task = &task2, .deadline = 1500};

    int level = 0;
    int slot = 5;

    driver.wheel_levels[level].slots[slot] = (struct time_entry_list) {};
    driver.active_slot_bitmap[level] = 0;
    time_entry_list_push_front(&driver.wheel_levels[level].slots[slot], &e1);
    time_entry_list_push_front(&driver.wheel_levels[level].slots[slot], &e2);
    driver.active_slot_bitmap[level] |= BIT_SET(slot);

    slot_process_expiration(&driver, level, slot, 1200);
    assert(task1.state = TASK_RUNNABLE);
    assert(task2.state = TASK_WAITING);
}

void time_driver_tests(void) {
    test_clz64();
    printf("✅ Passed test_clz64()\n");
    test_ctz64();
    printf("✅ Passed test_ctz64()\n");
    test_rotate_r64();
    printf("✅ Passed test_rotate_r64()\n");
    test_level_for();
    printf("✅ Passed test_level_for()\n");
    test_level_resolution();
    printf("✅ Passed test_level_resolution()\n");
    test_slot_for();
    printf("✅ Passed test_slot_for()\n");
    test_slot_resolution();
    printf("✅ Passed test_slot_resolution()\n");
    test_slot_next_occupied();
    printf("✅ Passed test_slot_next_occupied()\n");
    test_slot_process_expiration();
    printf("✅ Passed test_slot_process_expiration()\n");
}
#endif
