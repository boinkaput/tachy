#include <assert.h>
#include <stddef.h>
#include <sys/epoll.h>

#include "../include/clock.h"
#include "../include/join.h"
#include "../include/runtime.h"
#include "../include/tachy.h"
#include "../include/task.h"
#include "../include/time_driver.h"

static struct {
    struct task_list tasks;
    struct task_list deferred_tasks;
    struct time_driver time_driver;
    struct task *cur_task;
    struct task *blocked_task;
    int epoll_fd;
} runtime = {0};

bool tachy_init(void) {
    runtime.epoll_fd = epoll_create1(0);
    if (runtime.epoll_fd == -1) {
        return false;
    }

    clock_init();
    return true;
}

void tachy__block_on(void *future, tachy_poll_fn poll_fn,
                      size_t future_size_bytes, void *output)
{
    assert(future != NULL);
    assert(poll_fn != NULL);

    runtime.blocked_task = task_new(future, poll_fn, future_size_bytes, 0);
    while (1) {
        if (task_runnable(runtime.blocked_task)) {
            runtime.cur_task = runtime.blocked_task;
            if (task_poll(runtime.cur_task, output) == TACHY_POLL_READY) {
                runtime.blocked_task = NULL;
                runtime.cur_task = NULL;
                return;
            }
        }

        for (runtime.cur_task = task_list_pop_front(&runtime.tasks);
             runtime.cur_task != NULL;
             runtime.cur_task = task_list_pop_front(&runtime.tasks)) {
            void *output = task_output(runtime.cur_task);
            task_poll(runtime.cur_task, output);
        }

        for (struct task *task = task_list_pop_front(&runtime.deferred_tasks);
             task != NULL; task = task_list_pop_front(&runtime.deferred_tasks)) {
            rt_wake_task(task);
        }

        while (task_list_empty(&runtime.tasks) && !task_runnable(runtime.blocked_task)) {
            uint64_t now = clock_now();
            uint64_t deadline = time_next_expiration(&runtime.time_driver);
            if (now < deadline) {
                uint64_t timeout = deadline - now;
                int nfds = epoll_wait(runtime.epoll_fd, NULL, 0, timeout);
                if (nfds != -1) {
                    continue;
                }
            }

            now = clock_now();
            time_process_at(&runtime.time_driver, now);
            for (struct task *task = time_next_pending_task(&runtime.time_driver);
                 task != NULL; task = time_next_pending_task(&runtime.time_driver)) {
                rt_wake_task(task);
            }
        }
    }
}

struct tachy_join_handle tachy__spawn(void *future, tachy_poll_fn poll_fn,
                                     size_t future_size_bytes,
                                     size_t output_size_bytes)
{
    assert(future != NULL);
    assert(poll_fn != NULL);
    assert(future_size_bytes > 0);

    struct task *task = task_new(future, poll_fn, future_size_bytes, output_size_bytes);
    if (task == NULL) {
        return tachy_join(NULL, TACHY_OUT_OF_MEMORY_ERROR);
    }

    task_list_push_front(&runtime.tasks, task);
    return tachy_join(task, TACHY_NO_ERROR);
}

struct time_driver *rt_time_driver(void) {
    return &runtime.time_driver;
}

struct task *rt_cur_task(void) {
    assert(runtime.cur_task != NULL);
    return runtime.cur_task;
}

void rt_wake_task(struct task *task) {
    assert(task != NULL);

    if (task_runnable(task)) {
        return;
    }

    task_make_runnable(task);
    if (task != runtime.blocked_task) {
        task_list_push_front(&runtime.tasks, task);
    }
}

void rt_defer_task(struct task *task) {
    task_list_push_front(&runtime.deferred_tasks, task);
}
