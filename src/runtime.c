#include <stddef.h>
#include <sys/epoll.h>

#include "../include/clock.h"
#include "../include/tachy.h"
#include "../include/task.h"
#include "../include/task_queue.h"
#include "../include/time.h"

static struct {
    struct tachy_task_queue task_queue;
    struct tachy_time_driver time_driver;
    struct tachy_task *cur_task;
    struct tachy_task *blocked_task;
    int epoll_fd;
} runtime = {0};

static void wake_task(struct tachy_task *task) {
    assert(task != NULL);

    if (tachy_task_runnable(task)) {
        return;
    }

    tachy_task_make_runnable(task);
    if (task != runtime.blocked_task) {
        tachy_task_queue_push(&runtime.task_queue, task);
    }
}

bool tachy_rt_init(void) {
    runtime.epoll_fd = epoll_create1(0);
    if (runtime.epoll_fd == -1) {
        return false;
    }

    tachy_clock_init();
    runtime.task_queue = tachy_task_queue_new();
    runtime.time_driver = tachy_time_driver_new();
    runtime.cur_task = NULL;
    runtime.blocked_task = NULL;
    return true;
}

void tachy_rt_block_on_(void *future, tachy_poll_fn poll_fn,
                      size_t future_size_bytes, void *output)
{
    assert(future != NULL);
    assert(poll_fn != NULL);

    runtime.blocked_task = tachy_task_new(future, poll_fn, future_size_bytes, 0);
    while (1) {
        if (tachy_task_runnable(runtime.blocked_task)) {
            runtime.cur_task = runtime.blocked_task;
            struct tachy_task_poll_result res = tachy_task_poll(runtime.cur_task, output);
            if (res.state == TACHY_POLL_READY) {
                assert(res.consumer == NULL);
                runtime.blocked_task = NULL;
                runtime.cur_task = NULL;
                return;
            }
        }

        size_t queue_size = tachy_task_queue_size(&runtime.task_queue);
        for (size_t i = 0; i < queue_size; i++) {
            runtime.cur_task = tachy_task_queue_pop(&runtime.task_queue);
            void *output = tachy_task_output(runtime.cur_task);
            struct tachy_task_poll_result res = tachy_task_poll(runtime.cur_task, output);
            if (res.state == TACHY_POLL_READY && res.consumer != NULL) {
                wake_task(res.consumer);
            }
        }
        runtime.cur_task = NULL;

        queue_size = tachy_task_queue_size(&runtime.task_queue);
        while (queue_size < 1 && !tachy_task_runnable(runtime.blocked_task)) {
            uint64_t now = tachy_clock_now();
            uint64_t deadline = tachy_time_next_expiration(&runtime.time_driver);
            if (now < deadline) {
                uint64_t timeout = deadline - now;
                int nfds = epoll_wait(runtime.epoll_fd, NULL, 0, timeout);
                if (nfds != -1) {
                    continue;
                }
            }

            now = tachy_clock_now();
            tachy_time_process_at(&runtime.time_driver, now);
            for (struct tachy_task *task = tachy_time_next_pending_task(&runtime.time_driver);
                 task != NULL; task = tachy_time_next_pending_task(&runtime.time_driver)) {
                wake_task(task);
            }
        }
    }
}

struct tachy_join_handle tachy_rt_spawn_(void *future, tachy_poll_fn poll_fn,
                                     size_t future_size_bytes,
                                     size_t output_size_bytes)
{
    assert(future != NULL);
    assert(poll_fn != NULL);
    assert(future_size_bytes > 0);

    struct tachy_task *task = tachy_task_new(future, poll_fn, future_size_bytes, output_size_bytes);
    if (task == NULL) {
        return tachy_join_handle(NULL, TACHY_OUT_OF_MEMORY_ERROR);
    }

    tachy_task_queue_push(&runtime.task_queue, task);
    return tachy_join_handle(task, TACHY_NO_ERROR);
}

struct tachy_time_driver *tachy_rt_time_driver(void) {
    return &runtime.time_driver;
}

struct tachy_task *tachy_rt_cur_task(void) {
    assert(runtime.cur_task != NULL);
    return runtime.cur_task;
}
