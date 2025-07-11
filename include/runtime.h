#pragma once

struct time_driver *rt_time_driver(void);
struct task *rt_cur_task(void);
void rt_wake_task(struct task *task);
void rt_defer_task(struct task *task);
