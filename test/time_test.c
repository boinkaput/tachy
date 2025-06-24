#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/time.h"

#define NUM_TIMERS 1000

struct tachy_task {
    int id;
};

static struct tachy_task make_task(int id) {
    struct tachy_task task;
    memset(&task, 0, sizeof(task));
    task.id = id;
    return task;
}

void test_driver_new(void) {
    struct tachy_time_driver driver = tachy_time_driver_new();
    assert(driver.elapsed == 0);
    assert(driver.pending_list_head == NULL);
    for (int i = 0; i < TACHY_TIME_WHEEL_LEVELS; ++i) {
        assert(driver.active_slot_bitmap[i] == 0);
        for (int j = 0; j < TACHY_TIME_SLOTS_PER_LEVEL; ++j) {
            assert(driver.wheel_levels[i].slots[j] == NULL);
        }
    }
}

void test_entry_new(void) {
    struct tachy_time_driver driver = tachy_time_driver_new();
    struct tachy_task task = make_task(42);
    uint64_t deadline = 1234;

    struct tachy_time_entry *entry = tachy_time_entry_new(&task, deadline);
    assert(entry);
    free(entry);
}

void test_insert_timeout_and_expiration(void) {
    struct tachy_time_driver driver = tachy_time_driver_new();
    struct tachy_task task = make_task(1);

    struct tachy_time_entry *entry = tachy_time_entry_new(&task, 500);
    assert(entry);

    tachy_time_insert_timeout(&driver, entry);

    uint64_t exp = tachy_time_next_expiration(&driver);
    assert(exp == 448);

    free(entry);
}

void test_insert_expired(void) {
    struct tachy_time_driver driver = tachy_time_driver_new();
    tachy_time_process_at(&driver, 1000);
    assert(driver.elapsed == 1000);

    struct tachy_task task = make_task(2);
    struct tachy_time_entry *entry = tachy_time_entry_new(&task, 500);
    assert(entry);

    tachy_time_insert_timeout(&driver, entry);
    free(entry);
}

void test_process_at_and_pending(void) {
    struct tachy_time_driver driver = tachy_time_driver_new();
    struct tachy_task task1 = make_task(1);
    struct tachy_task task2 = make_task(2);

    struct tachy_time_entry *entry1 = tachy_time_entry_new(&task1, 100);
    struct tachy_time_entry *entry2 = tachy_time_entry_new(&task2, 200);

    tachy_time_insert_timeout(&driver, entry1);
    tachy_time_insert_timeout(&driver, entry2);

    tachy_time_process_at(&driver, 150);
    struct tachy_task *pending = tachy_time_next_pending_task(&driver);
    assert(pending == &task1);
    assert(tachy_time_next_pending_task(&driver) == NULL);

    tachy_time_process_at(&driver, 250);
    pending = tachy_time_next_pending_task(&driver);
    assert(pending == &task2);
    assert(tachy_time_next_pending_task(&driver) == NULL);


    free(entry1);
    free(entry2);
}

void test_next_pending_task_empty(void) {
    struct tachy_time_driver driver = tachy_time_driver_new();
    assert(tachy_time_next_pending_task(&driver) == NULL);
}

void test_coarse_level_cascade(void) {
    struct tachy_time_driver d = tachy_time_driver_new();
    struct tachy_task tk = make_task(10);
    struct tachy_time_entry *e = tachy_time_entry_new(&tk, 5000);
    assert(e);

    tachy_time_insert_timeout(&d, e);
    uint64_t ne = tachy_time_next_expiration(&d);
    assert(ne == 4096);

    tachy_time_process_at(&d, 4096);
    assert(tachy_time_next_pending_task(&d) == NULL);

    ne = tachy_time_next_expiration(&d);
    assert(ne == 4992);

    tachy_time_process_at(&d, 5000);
    struct tachy_task *f = tachy_time_next_pending_task(&d);
    assert(f == &tk);
    assert(tachy_time_next_pending_task(&d) == NULL);

    free(e);
}

void test_multiple_same_coarse_slot(void) {
    struct tachy_time_driver d = tachy_time_driver_new();
    struct tachy_task t1 = make_task(1), t2 = make_task(2);

    struct tachy_time_entry *e1 = tachy_time_entry_new(&t1, 4100);
    struct tachy_time_entry *e2 = tachy_time_entry_new(&t2, 4500);

    tachy_time_insert_timeout(&d, e1);
    tachy_time_insert_timeout(&d, e2);

    assert(tachy_time_next_expiration(&d) == 4096);
    tachy_time_process_at(&d, 4096);
    assert(tachy_time_next_pending_task(&d) == NULL);

    assert(tachy_time_next_expiration(&d) == 4100);
    tachy_time_process_at(&d, 4100);
    struct tachy_task *f = tachy_time_next_pending_task(&d);
    assert(f == &t1);

    assert(tachy_time_next_expiration(&d) == 4480);
    tachy_time_process_at(&d, 4500);
    f = tachy_time_next_pending_task(&d);
    assert(f == &t2);

    free(e1);
    free(e2);
}

void test_multiple_levels_next_expiration(void) {
    struct tachy_time_driver d = tachy_time_driver_new();
    struct tachy_task t0 = make_task(0), t1 = make_task(1), t2 = make_task(2);

    struct tachy_time_entry *e0 = tachy_time_entry_new(&t0, 100);
    struct tachy_time_entry *e1 = tachy_time_entry_new(&t1, 500);
    struct tachy_time_entry *e2 = tachy_time_entry_new(&t2, 5000);

tachy_time_insert_timeout(&d, e0);
tachy_time_insert_timeout(&d, e1);
tachy_time_insert_timeout(&d, e2);

    assert(tachy_time_next_expiration(&d) == 64);
    tachy_time_process_at(&d, 100);
    assert(tachy_time_next_pending_task(&d) == &t0);

    assert(tachy_time_next_expiration(&d) == 448);
    tachy_time_process_at(&d, 500);
    assert(tachy_time_next_pending_task(&d) == &t1);

    assert(tachy_time_next_expiration(&d) == 4096);
    tachy_time_process_at(&d, 4096);
    assert(tachy_time_next_pending_task(&d) == NULL);

    assert(tachy_time_next_expiration(&d) == 4992);
    tachy_time_process_at(&d, 5000);
    assert(tachy_time_next_pending_task(&d) == &t2);

    free(e0); free(e1); free(e2);
}

void test_max_timeout_boundary(void) {
    struct tachy_time_driver d = tachy_time_driver_new();
    struct tachy_task t = make_task(9);
    uint64_t mx = TACHY_TIME_MAX_TIMEOUT_MS;
    struct tachy_time_entry *e = tachy_time_entry_new(&t, mx);
    assert(e);
    tachy_time_insert_timeout(&d, e);

    uint64_t ne = tachy_time_next_expiration(&d);
    assert(ne == 67645734912);

    tachy_time_process_at(&d, ne);
    assert(tachy_time_next_pending_task(&d) == NULL);

    tachy_time_process_at(&d, mx);
    struct tachy_task *f = tachy_time_next_pending_task(&d);
    assert(f == &t);

    free(e);
}

int test_huge_number_of_timers(void) {
    time_t seed = time(NULL);
    printf("test_huge_number_of_timers(): Using seed %ld\n", seed);
    srand(seed);
    for (int s = 0; s < 100; s++) {
        struct tachy_time_driver driver = tachy_time_driver_new();
        struct tachy_task tasks[NUM_TIMERS];
        uint64_t deadlines[NUM_TIMERS];
        struct tachy_time_entry *entries[NUM_TIMERS];

        uint64_t max_dead = TACHY_TIME_MAX_TIMEOUT_MS / 10;
        for (int i = 0; i < NUM_TIMERS; ++i) {
            tasks[i].id = i;
            deadlines[i] = (uint64_t)(rand() % max_dead) + 1;
        }

        int indices[NUM_TIMERS];
        for (int i = 0; i < NUM_TIMERS; ++i) {
            indices[i] = i;
        }

        for (int i = 1; i < NUM_TIMERS; ++i) {
            int key = indices[i];
            int j = i - 1;
            while (j >= 0 && deadlines[indices[j]] > deadlines[key]) {
                indices[j+1] = indices[j];
                j--;
            }
            indices[j+1] = key;
        }

        for (int i = 0; i < NUM_TIMERS; ++i) {
            struct tachy_time_entry *e = tachy_time_entry_new(&tasks[i], deadlines[i]);
            assert(e);
            entries[i] = e;
            tachy_time_insert_timeout(&driver, e);
        }

        uint64_t fired_deadline = 0;
        for (int k = 0; k < NUM_TIMERS; ++k) {
            uint64_t next_exp = tachy_time_next_expiration(&driver);
            assert(next_exp >= fired_deadline);
            tachy_time_process_at(&driver, deadlines[indices[k]]);

            struct tachy_task *t = tachy_time_next_pending_task(&driver);
            assert(t != NULL);

            int expected_idx = indices[k];
            assert(t->id == expected_idx);
            fired_deadline = next_exp;
        }

        assert(tachy_time_next_pending_task(&driver) == NULL);
        for (int i = 0; i < NUM_TIMERS; ++i) {
            free(entries[i]);
        }
    }
    return 0;
}

int main(void) {
    test_driver_new();
    printf("✅ test_driver_new\n");
    test_entry_new();
    printf("✅ test_entry_new\n");
    test_insert_timeout_and_expiration();
    printf("✅ test_insert_timeout_and_expiration\n");
    test_insert_expired();
    printf("✅ test_insert_expired\n");
    test_process_at_and_pending();
    printf("✅ test_process_at_and_pending\n");
    test_next_pending_task_empty();
    printf("✅ test_next_pending_task_empty\n");
    test_coarse_level_cascade();
    printf("✅ test_coarse_level_cascade\n");
    test_multiple_same_coarse_slot();
    printf("✅ test_multiple_same_coarse_slot\n");
    test_multiple_levels_next_expiration();
    printf("✅ test_multiple_levels_next_expiration\n");
    test_max_timeout_boundary();
    printf("✅ test_max_timeout_boundary\n");

    test_huge_number_of_timers();
    printf("✅ All %d timers fired in correct order. Stress test passed.\n", NUM_TIMERS);

    printf("\nAll tests passed!\n");
    return 0;
}

