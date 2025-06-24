#pragma once

#include <stddef.h>

#define TACHY_TASK_QUEUE_CAPACITY 8

struct tachy_task_queue {
    struct tachy_task *tasks[TACHY_TASK_QUEUE_CAPACITY];
    size_t head;
    size_t tail;
};

static inline struct tachy_task_queue tachy_task_queue_new(void) {
    return (struct tachy_task_queue) {0};
}

static inline size_t tachy_task_queue_size(struct tachy_task_queue *self) {
    assert(self != NULL);

    return self->tail - self->head;
}

static inline void tachy_task_queue_push(struct tachy_task_queue *self, struct tachy_task *task) {
    assert(self != NULL);
    assert(tachy_task_queue_size(self) < TACHY_TASK_QUEUE_CAPACITY);
    assert(task != NULL);

    size_t next_tail = self->tail % TACHY_TASK_QUEUE_CAPACITY;
    self->tail++;
    self->tasks[next_tail] = task;
}

static inline struct tachy_task *tachy_task_queue_pop(struct tachy_task_queue *self) {
    assert(self != NULL);
    assert(tachy_task_queue_size(self) > 0);

    size_t cur_head = self->head % TACHY_TASK_QUEUE_CAPACITY;
    self->head++;
    return self->tasks[cur_head];
}
