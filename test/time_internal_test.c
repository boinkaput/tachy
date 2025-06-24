#include <assert.h>
#include <stdio.h>

#include "./src/time.c"

struct tachy_task {
    int id;
};

void test_clz64() {
    assert(clz64(0) == 64);
    assert(clz64(1ULL << 63) == 0);
    assert(clz64(1ULL << 0) == 63);
    assert(clz64((1ULL << 62) | (1ULL << 10)) == 1);
    assert(clz64((1ULL << 63)) == 0);
}

void test_ctz64() {
    assert(ctz64(0) == 64);
    assert(ctz64(1ULL << 0) == 0);
    assert(ctz64(1ULL << 10) == 10);
    assert(ctz64((1ULL << 5) | (1ULL << 10)) == 5);
}

void test_rotate_r64() {
    assert(rotate_r64(0b1ULL, 1) == (1ULL << 63));
    assert(rotate_r64(0b1ULL << 5, 5) == 1ULL);
    assert(rotate_r64(0xFFFFFFFFFFFFFFFFULL, 10) == 0xFFFFFFFFFFFFFFFFULL);
}

void test_level_for() {
    assert(level_for(1) == 0);
    assert(level_for(63) == 0);
    assert(level_for(64) == 1);
    assert(level_for(4095) == 1);
    assert(level_for(4096) == 2);
    assert(level_for(262143) == 2);
}

void test_level_resolution() {
    assert(level_resolution(1) == 64);
    assert(level_resolution(64) == 4096);
    assert(level_resolution(4096) == 262144);
}

void test_slot_for() {
    assert(slot_for(0, 0) == 0);
    assert(slot_for(0, 63) == 63);
    assert(slot_for(1, 64) == 1);
    assert(slot_for(1, 4095) == 63);
}

void test_slot_resolution() {
    assert(slot_resolution(0) == 1);
    assert(slot_resolution(1) == 64);
    assert(slot_resolution(2) == 4096);
    assert(slot_resolution(3) == 262144);
}

void test_slot_push_front_and_pop() {
    struct tachy_time_entry *head = NULL;
    struct tachy_task task = {.id = 42};
    struct tachy_time_entry e1 = {.task = &task};
    struct tachy_time_entry e2 = {.task = &task};
    
    slot_push_front(&head, &e1);
    assert(head == &e1);
    slot_push_front(&head, &e2);
    assert(head == &e2);
    assert(head->next == &e1);
    assert(e1.prev == &e2);

    struct tachy_time_entry *popped = slot_pop_front(&head);
    assert(popped == &e2);
    assert(head == &e1);
    assert(e1.prev == NULL);

    popped = slot_pop_front(&head);
    assert(popped == &e1);
    assert(head == NULL);
}

void test_slot_next_occupied() {
    uint64_t bitmap = (1ULL << 5) | (1ULL << 10) | (1ULL << 20);

    int next = slot_next_occupied(0, 0, bitmap);
    assert(next == 5);

    next = slot_next_occupied(0, 6, bitmap);
    assert(next == 10);

    next = slot_next_occupied(0, 21, bitmap);
    assert(next == 5);
}

void test_slot_process_expiration() {
    struct tachy_time_driver driver = tachy_time_driver_new();
    struct tachy_task task = {.id = 1};
    struct tachy_time_entry e1 = {.task = &task, .deadline = 1000};
    struct tachy_time_entry e2 = {.task = &task, .deadline = 1500};

    int level = 0;
    int slot = 5;

    driver.wheel_levels[level].slots[slot] = NULL;
    driver.active_slot_bitmap[level] = 0;
    slot_push_front(&driver.wheel_levels[level].slots[slot], &e1);
    slot_push_front(&driver.wheel_levels[level].slots[slot], &e2);
    driver.active_slot_bitmap[level] |= BIT_SET(slot);

    slot_process_expiration(&driver, level, slot, 1200);
    assert(driver.pending_list_head == &e1 || driver.pending_list_head == &e2);
}

int main() {
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
    test_slot_push_front_and_pop();
    printf("✅ Passed test_slot_push_front_and_pop()\n");
    test_slot_next_occupied();
    printf("✅ Passed test_slot_next_occupied()\n");
    test_slot_process_expiration();
    printf("✅ Passed test_slot_process_expiration()\n");
    return 0;
}
