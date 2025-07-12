#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/task.h"
#include "../include/time_driver.h"

#define NUM_TIMERS 1000

void test_insert_timeout_and_expiration(void) {
    struct time_driver driver = {0};
    struct task task = {.ref_count = 1};

    struct time_entry *entry = time_entry_new(&task, 500);
    assert(entry);

    time_insert_timeout(&driver, entry);
    uint64_t exp = time_next_expiration(&driver);
    assert(exp == 448);

    free(entry);
}

void test_insert_expired(void) {
    struct time_driver driver = {0};
    time_process_at(&driver, 1000);
    assert(driver.elapsed == 1000);

    struct task task = {.ref_count = 1};
    struct time_entry *entry = time_entry_new(&task, 500);
    assert(entry);

    time_insert_timeout(&driver, entry);
    assert(time_entry_fired(entry));
    uint64_t exp = time_next_expiration(&driver);
    assert(exp == 0);

    free(entry);
}

void test_process_at(void) {
    struct time_driver driver = {0};
    struct task task1 = {.ref_count = 1, .state = TASK_WAITING};
    struct task task2 = {.ref_count = 1, .state = TASK_WAITING};

    struct time_entry *entry1 = time_entry_new(&task1, 100);
    struct time_entry *entry2 = time_entry_new(&task2, 200);

    time_insert_timeout(&driver, entry1);
    time_insert_timeout(&driver, entry2);

    time_process_at(&driver, 150);
    assert(task1.state == TASK_RUNNABLE);
    assert(task2.state == TASK_WAITING);

    time_process_at(&driver, 250);
    assert(task1.state == TASK_RUNNABLE);
    assert(task2.state == TASK_RUNNABLE);

    free(entry1);
    free(entry2);
}

void test_level_cascade(void) {
    struct time_driver driver = {0};
    struct task task = {.ref_count = 1, .state = TASK_WAITING};
    struct time_entry *entry = time_entry_new(&task, 5000);
    assert(entry);

    time_insert_timeout(&driver, entry);

    uint64_t next_exp = time_next_expiration(&driver);
    assert(next_exp == 4096);
    time_process_at(&driver, 4096);
    assert(task.state == TASK_WAITING);

    next_exp = time_next_expiration(&driver);
    assert(next_exp == 4992);
    time_process_at(&driver, 5000);
    assert(task.state == TASK_RUNNABLE);

    free(entry);
}

void test_multiple_same_slot(void) {
    struct time_driver driver = {0};
    struct task task1 = {.ref_count = 1, .state = TASK_WAITING};
    struct task task2 = {.ref_count = 1, .state = TASK_WAITING};

    struct time_entry *e1 = time_entry_new(&task1, 4100);
    struct time_entry *e2 = time_entry_new(&task2, 4500);

    time_insert_timeout(&driver, e1);
    time_insert_timeout(&driver, e2);

    assert(time_next_expiration(&driver) == 4096);
    time_process_at(&driver, 4096);
    assert(task1.state == TASK_WAITING);
    assert(task2.state == TASK_WAITING);

    assert(time_next_expiration(&driver) == 4100);
    time_process_at(&driver, 4100);
    assert(task1.state == TASK_RUNNABLE);
    assert(task2.state == TASK_WAITING);

    assert(time_next_expiration(&driver) == 4480);
    time_process_at(&driver, 4500);
    assert(task1.state == TASK_RUNNABLE);
    assert(task2.state == TASK_RUNNABLE);

    free(e1);
    free(e2);
}

void test_multiple_levels_next_expiration(void) {
    struct time_driver driver = {0};
    struct task task0 = {.ref_count = 1, .state = TASK_WAITING};
    struct task task1 = {.ref_count = 1, .state = TASK_WAITING};
    struct task task2 = {.ref_count = 1, .state = TASK_WAITING};

    struct time_entry *e0 = time_entry_new(&task0, 100);
    struct time_entry *e1 = time_entry_new(&task1, 500);
    struct time_entry *e2 = time_entry_new(&task2, 5000);

    time_insert_timeout(&driver, e0);
    time_insert_timeout(&driver, e1);
    time_insert_timeout(&driver, e2);

    assert(time_next_expiration(&driver) == 64);
    time_process_at(&driver, 100);
    assert(task0.state == TASK_RUNNABLE);
    assert(task1.state == TASK_WAITING);
    assert(task2.state == TASK_WAITING);

    assert(time_next_expiration(&driver) == 448);
    time_process_at(&driver, 500);
    assert(task0.state == TASK_RUNNABLE);
    assert(task1.state == TASK_RUNNABLE);
    assert(task2.state == TASK_WAITING);

    assert(time_next_expiration(&driver) == 4096);
    time_process_at(&driver, 4096);
    assert(task0.state == TASK_RUNNABLE);
    assert(task1.state == TASK_RUNNABLE);
    assert(task2.state == TASK_WAITING);

    assert(time_next_expiration(&driver) == 4992);
    time_process_at(&driver, 5000);
    assert(task0.state == TASK_RUNNABLE);
    assert(task1.state == TASK_RUNNABLE);
    assert(task2.state == TASK_RUNNABLE);

    free(e0);
    free(e1);
    free(e2);
}

void test_max_timeout_boundary(void) {
    struct time_driver driver = {0};
    struct task task = {.ref_count = 1, .state = TASK_WAITING};
    struct time_entry *entry = time_entry_new(&task, TIME_MAX_TIMEOUT_MS);
    assert(entry);
    time_insert_timeout(&driver, entry);

    uint64_t next_exp = time_next_expiration(&driver);
    assert(next_exp - 1 == TIME_MAX_TIMEOUT_MS - (((uint64_t) 1) << (6 * (TIME_WHEEL_LEVELS - 1))));

    time_process_at(&driver, next_exp);
    assert(task.state == TASK_WAITING);

    time_process_at(&driver, TIME_MAX_TIMEOUT_MS);
    assert(task.state == TASK_RUNNABLE);

    free(entry);
}

int test_huge_number_of_timers(void) {
    time_t seed = time(NULL);
    printf("test_huge_number_of_timers(): Using seed %ld\n", seed);
    srand(seed);
    for (int s = 0; s < NUM_TIMERS; s++) {
        struct time_driver driver = {0};
        struct task tasks[NUM_TIMERS] = {0};
        uint64_t deadlines[NUM_TIMERS];
        struct time_entry *entries[NUM_TIMERS];

        int indices[NUM_TIMERS];
        int expected_ref_count[NUM_TIMERS];
        uint64_t max_dead = TIME_MAX_TIMEOUT_MS / 10;
        for (int i = 0; i < NUM_TIMERS; i++) {
            indices[i] = i;
            tasks[i].state = TASK_WAITING;
            tasks[i].ref_count = i + 1;
            expected_ref_count[i] = i + 1;
            deadlines[i] = (uint64_t)(rand() % max_dead) + 1;
        }

        for (int i = 1; i < NUM_TIMERS; i++) {
            int key = indices[i];
            int j = i - 1;
            while (j >= 0 && deadlines[indices[j]] > deadlines[key]) {
                indices[j + 1] = indices[j];
                j--;
            }
            indices[j + 1] = key;
        }

        for (int i = 0; i < NUM_TIMERS; i++) {
            struct time_entry *entry = time_entry_new(&tasks[i], deadlines[i]);
            assert(entry);
            expected_ref_count[i] += 1;
            assert(entry->task->ref_count == expected_ref_count[i]);
            entries[i] = entry;
            time_insert_timeout(&driver, entry);
        }

        uint64_t fired_deadline = 0;
        for (int i = 0; i < NUM_TIMERS; i++) {
            uint64_t next_exp = time_next_expiration(&driver);
            assert(next_exp >= fired_deadline);
            time_process_at(&driver, deadlines[indices[i]]);

            struct task task = tasks[indices[i]];
            assert(task.state == TASK_RUNNABLE);

            assert(task.ref_count == expected_ref_count[indices[i]]);
            fired_deadline = next_exp;
        }

        for (int i = 0; i < NUM_TIMERS; i++) {
            struct task *task = entries[i]->task;
            time_entry_free(entries[i]);
            expected_ref_count[i] -= 1;
            assert(task->ref_count == expected_ref_count[i]);
        }
    }
    return 0;
}

int main(void) {
    test_insert_timeout_and_expiration();
    printf("✅ test_insert_timeout_and_expiration()\n");
    test_insert_expired();
    printf("✅ test_insert_expired()\n");
    test_process_at();
    printf("✅ test_process_at()\n");
    test_level_cascade();
    printf("✅ test_level_cascade()\n");
    test_multiple_same_slot();
    printf("✅ test_multiple_same_slot()\n");
    test_multiple_levels_next_expiration();
    printf("✅ test_multiple_levels_next_expiration()\n");
    test_max_timeout_boundary();
    printf("✅ test_max_timeout_boundary()\n");

    printf("\n");
    test_huge_number_of_timers();
    printf("✅ All %d timers fired in correct order. Stress test passed.\n", NUM_TIMERS);
    return 0;
}

