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

static inline size_t tachy_task_queue_size(struct tachy_task_queue *queue) {
    assert(queue != NULL);

    return queue->tail - queue->head;
}

static inline void tachy_task_queue_push(struct tachy_task_queue *queue, struct tachy_task *task) {
    assert(queue != NULL);
    assert(tachy_task_queue_size(queue) < TACHY_TASK_QUEUE_CAPACITY);
    assert(task != NULL);

    size_t next_tail = queue->tail % TACHY_TASK_QUEUE_CAPACITY;
    queue->tail++;
    queue->tasks[next_tail] = task;
}

static inline struct tachy_task *tachy_task_queue_pop(struct tachy_task_queue *queue) {
    assert(queue != NULL);
    assert(tachy_task_queue_size(queue) > 0);

    size_t cur_head = queue->head % TACHY_TASK_QUEUE_CAPACITY;
    queue->head++;
    return queue->tasks[cur_head];
}
