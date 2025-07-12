#include <assert.h>
#include <stdlib.h>

#include "../include/task.h"
#include "../include/time_entry.h"

#define TIMEOUT_FIRED (UINT64_MAX)
#define TIMEOUT_PENDING (UINT64_MAX - 1)

struct time_entry *time_entry_new(struct task *task, uint64_t deadline) {
    assert(task != NULL);

    struct time_entry *entry = malloc(sizeof(struct time_entry));
    if (entry == NULL) {
        return NULL;
    }

    task_ref_inc(task);
    *entry = (struct time_entry) {
        .prev = NULL,
        .next = NULL,
        .task = task,
        .deadline = deadline
    };
    return entry;
}

void time_entry_free(struct time_entry *entry) {
    task_ref_dec(entry->task);
    free(entry);
}

bool time_entry_pending(struct time_entry *entry) {
    return entry->deadline == TIMEOUT_PENDING;
}

bool time_entry_fired(struct time_entry *entry) {
    return entry->deadline == TIMEOUT_FIRED;
}

void time_entry_make_pending(struct time_entry *entry) {
    assert(!time_entry_pending(entry));
    assert(!time_entry_fired(entry));
    entry->deadline = TIMEOUT_PENDING;
}

void time_entry_make_fired(struct time_entry *entry) {
    assert(!time_entry_fired(entry));
    entry->deadline = TIMEOUT_FIRED;
}

bool time_entry_list_empty(struct time_entry_list *list) {
    return list->head == NULL;
}

void time_entry_list_push_front(struct time_entry_list *list, struct time_entry *entry) {
    struct time_entry *cur_head = list->head;
    entry->next = cur_head;
    if (cur_head != NULL) {
        cur_head->prev = entry;
    }
    list->head = entry;
}

struct time_entry *time_entry_list_pop_front(struct time_entry_list *list) {
    struct time_entry *entry = list->head;
    if (entry == NULL) {
        return NULL;
    }

    list->head = entry->next;
    if (entry->next != NULL) {
        entry->next->prev = NULL;
    }
    return entry;
}

static bool entry_in_list(struct time_entry_list *list, struct time_entry *entry) {
    for (struct time_entry *cur = list->head; cur != NULL; cur = cur->next) {
        if (cur == entry) {
            return true;
        }
    }
    return false;
}

void time_entry_list_remove(struct time_entry_list *list, struct time_entry *entry) {
    assert(list != NULL);
    assert(entry != NULL);
    assert(entry_in_list(list, entry));

    if (list->head == entry) {
        list->head = NULL;
        return;
    }

    if (entry->prev != NULL) {
        entry->prev->next = entry->next;
    }

    if (entry->next != NULL) {
        entry->next->prev = entry->prev;
    }
}
