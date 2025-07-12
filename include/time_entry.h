#pragma once

#include <stdbool.h>
#include <stdint.h>

struct time_entry {
    struct time_entry *prev;
    struct time_entry *next;
    struct task *task;
    uint64_t deadline;
};

struct time_entry_list {
    struct time_entry *head;
};

struct time_entry *time_entry_new(struct task *task, uint64_t deadline);
void time_entry_free(struct time_entry *entry);
bool time_entry_fired(struct time_entry *entry);
void time_entry_make_fired(struct time_entry *entry);

bool time_entry_list_empty(struct time_entry_list *list);
void time_entry_list_push_front(struct time_entry_list *list, struct time_entry *entry);
struct time_entry *time_entry_list_pop_front(struct time_entry_list *list);
void time_entry_list_remove(struct time_entry_list *list, struct time_entry *entry);
