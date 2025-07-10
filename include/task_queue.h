#pragma once

#include <assert.h>
#include <stddef.h>

#define TASK_QUEUE_CAPACITY 8

struct task_queue {
    struct task *tasks[TASK_QUEUE_CAPACITY];
    size_t head;
    size_t tail;
};

static inline struct task_queue task_queue_new(void) {
    return (struct task_queue) {0};
}

static inline size_t task_queue_size(struct task_queue *queue) {
    assert(queue != NULL);

    return queue->tail - queue->head;
}

static inline void task_queue_push(struct task_queue *queue, struct task *task) {
    assert(queue != NULL);
    assert(task_queue_size(queue) < TASK_QUEUE_CAPACITY);
    assert(task != NULL);

    size_t next_tail = queue->tail % TASK_QUEUE_CAPACITY;
    queue->tail++;
    queue->tasks[next_tail] = task;
}

static inline struct task *task_queue_pop(struct task_queue *queue) {
    assert(queue != NULL);
    assert(task_queue_size(queue) > 0);

    size_t cur_head = queue->head % TASK_QUEUE_CAPACITY;
    queue->head++;
    return queue->tasks[cur_head];
}
