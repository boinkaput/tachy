#include <assert.h>

#include "../include/task.h"
#include "../include/task_queue.h"

struct task_queue task_queue(void) {
    return (struct task_queue) {0};
}

bool task_queue_empty(struct task_queue *queue) {
    assert(queue != NULL);
    return queue->head == NULL;
}

void task_queue_push(struct task_queue *queue, struct task *task) {
    assert(queue != NULL);
    assert(task != NULL);

    task->next = queue->head;
    queue->head = task;
}

struct task *task_queue_pop(struct task_queue *queue) {
    assert(queue != NULL);

    struct task *head = queue->head;
    if (head != NULL) {
        queue->head = head->next;
    }
    return head;
}
