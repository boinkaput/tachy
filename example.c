#include <stdio.h>
#include <time.h>

#include "include/tachy.h"

typedef struct {
    int a;
    int d;
    struct tachy_yield_handle yield_handle;
    tachy_state state;
} FuncFrame;

static inline FuncFrame func(int a) {
    return (FuncFrame) {.a = a, .state = 0};
}

enum tachy_poll func_poll(FuncFrame *self, int *output) {
    tachy_begin(&self->state);

    self->d = 22;
    printf("Hello world: a = %d\n", self->a);

    self->yield_handle = tachy_yield();
    tachy_await(tachy_yield_poll(&self->yield_handle, NULL));

    printf("Bye world: d = %d\n", self->d);
    tachy_return(21);

    tachy_end;
}

typedef struct {
    int a;
    int b;
    int c;
    struct tachy_yield_handle yield_handle;
    FuncFrame f;
    tachy_state state;
} FuncFrame1;

static inline FuncFrame1 func1(int a, int b) {
    return (FuncFrame1) {.a = a, .b = b, .state = 0};
}

enum tachy_poll func1_poll(FuncFrame1 *self, int *output) {
    tachy_begin(&self->state);

    self->c = 4;
    printf("Hello world: a = %d, b = %d\n", self->a, self->b);
    self->yield_handle = tachy_yield();
    tachy_await(tachy_yield_poll(&self->yield_handle, NULL));

    int i;
    self->f = func(13);
    tachy_await(func_poll(&self->f, &i));

    printf("Bye world: c = %d, i = %d\n", self->c, i);
    tachy_return(64);

    tachy_end;
}

typedef struct {
    struct tachy_yield_handle yield_handle;
    tachy_state state;
} FuncFrame2;

static inline FuncFrame2 func2(void) {
    return (FuncFrame2) {.state = 0};
}

enum tachy_poll func2_poll(FuncFrame2 *self, void *output) {
    (void) output;
    tachy_begin(&self->state);

    printf("yielding from future\n");
    self->yield_handle = tachy_yield();
    tachy_await(tachy_yield_poll(&self->yield_handle, NULL));

    printf("returning from future\n");
    tachy_return();

    tachy_end;
}

typedef struct {
    bool joining;
    struct tachy_yield_handle yield_handle;
    tachy_state state;
} FuncFrame3;

static inline FuncFrame3 func3(bool joining) {
    return (FuncFrame3) {.joining = joining, .state = 0};
}

enum tachy_poll func3_poll(FuncFrame3 *self, int *output) {
    tachy_begin(&self->state);

    printf("(joining: %s) yielding from future 3 - 1\n", (self->joining) ? "true" : "false");
    self->yield_handle = tachy_yield();
    tachy_await(tachy_yield_poll(&self->yield_handle, NULL));

    printf("(joining: %s) yielding from future 3 - 2\n", (self->joining) ? "true" : "false");
    self->yield_handle = tachy_yield();
    tachy_await(tachy_yield_poll(&self->yield_handle, NULL));

    printf("(joining: %s) returning from future 3 - 3\n", (self->joining) ? "true" : "false");
    tachy_return(3);

    tachy_end;
}

#define S_TO_MS(sec) ((sec) * 1000)
#define NS_TO_MS(nsec) ((nsec) / 1000000)

static uint64_t now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return S_TO_MS(ts.tv_sec) + NS_TO_MS(ts.tv_nsec);
}

typedef struct {
    int argc;
    char **argv;
    tachy_state state;
    FuncFrame1 fut1;
    FuncFrame2 fut2;
    struct tachy_join_handle j;
    uint64_t begin;
    struct tachy_sleep_handle sleep_handle;
} MainFrame;

static inline MainFrame async_main(int argc, char *argv[]) {
    return (MainFrame) {.argc = argc, .argv = argv, .state = 0};
}

enum tachy_poll async_main_poll(MainFrame *self, int *output) {
    tachy_begin(&self->state);

    printf("argv: [");
    for (int i = 0; i < self->argc - 1; i++) {
        printf("%s, ", self->argv[i]);
    }
    printf("%s]\n", self->argv[self->argc - 1]);

    printf("spawning future 3\n");
    FuncFrame3 fut3 = func3(true);
    self->j = tachy_spawn(&fut3, (tachy_poll_fn) &func3_poll, sizeof(int));

    printf("spawning future 3 - no join\n");
    fut3 = func3(false);
    int err = tachy_spawn_no_join(&fut3, (tachy_poll_fn) &func3_poll, sizeof(int));
    printf("spawned future 3 no join with status %d\n", err);

    int i;
    self->fut1 = func1(28, 841);
    printf("initialised future\n");
    tachy_await(func1_poll(&self->fut1, &i));
    printf("finished polling future: %d\n", i);

    self->fut2 = func2();
    tachy_await(func2_poll(&self->fut2, &i));
    printf("finished polling future\n");

    tachy_await(tachy_join_poll(&self->j, &i));
    printf("future 3 joined with status %d\n", self->j.state);
    printf("future 3 joined with output %d\n", i);

    self->sleep_handle = tachy_sleep((struct tachy_sleep_duration) {.secs = 1});
    self->begin = now();
    printf("Sleeping now for 1sec\n");
    tachy_await(tachy_sleep_poll(&self->sleep_handle, NULL));
    uint64_t end = now();
    printf("Time diff = %lums\n", end - self->begin);

    tachy_return(0);

    tachy_end;
}

int main(int argc, char *argv[]) {
    int ret;
    MainFrame fut = async_main(argc, argv);
    if (!tachy_init()) {
        printf("Failed to initialise runtime\n");
        return 1;
    }
    tachy_block_on(&fut, (tachy_poll_fn) &async_main_poll, &ret);
    printf("Exited with return value: %d\n", ret);

    return ret;
}
