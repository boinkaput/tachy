#pragma once

struct task_queue {
    struct task *head;
};

struct task_queue task_queue(void);
bool task_queue_empty(struct task_queue *queue);
void task_queue_push(struct task_queue *queue, struct task *task);
struct task *task_queue_pop(struct task_queue *queue);
