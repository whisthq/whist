/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file linked_list.c
 * @brief Implementation of linked list API.
 */
#include "whist/logging/logging.h"
#include "linked_list.h"

void linked_list_init(LinkedList *list) {
    list->items = 0;
    list->head = NULL;
    list->tail = NULL;
}

void linked_list_add_head_internal(LinkedList *list, LinkedListHeader *item) {
    ++list->items;
    item->next = list->head;
    item->prev = NULL;
    if (list->head)
        list->head->prev = item;
    else
        list->tail = item;
    list->head = item;
}

void linked_list_add_tail_internal(LinkedList *list, LinkedListHeader *item) {
    ++list->items;
    item->prev = list->tail;
    item->next = NULL;
    if (list->tail)
        list->tail->next = item;
    else
        list->head = item;
    list->tail = item;
}

void linked_list_add_after_internal(LinkedList *list, LinkedListHeader *old_item,
                                    LinkedListHeader *new_item) {
    if (old_item->next == NULL) {
        linked_list_add_tail_internal(list, new_item);
    } else {
        ++list->items;
        new_item->next = old_item->next;
        new_item->prev = old_item;
        old_item->next->prev = new_item;
        old_item->next = new_item;
    }
}

void linked_list_add_before_internal(LinkedList *list, LinkedListHeader *old_item,
                                     LinkedListHeader *new_item) {
    if (old_item->prev == NULL) {
        linked_list_add_head_internal(list, new_item);
    } else {
        ++list->items;
        new_item->next = old_item;
        new_item->prev = old_item->prev;
        old_item->prev->next = new_item;
        old_item->prev = new_item;
    }
}

void linked_list_remove_internal(LinkedList *list, LinkedListHeader *item) {
    // Do not remove an item from an empty list.
    FATAL_ASSERT(list->items > 0);

    if (item->next)
        item->next->prev = item->prev;
    else
        list->tail = item->prev;

    if (item->prev)
        item->prev->next = item->next;
    else
        list->head = item->next;

    item->next = item->prev = NULL;
    --list->items;
}

int linked_list_size(const LinkedList *list) { return list->items; }

void *linked_list_extract_head(LinkedList *list) {
    LinkedListHeader *item = list->head;
    if (item) linked_list_remove_internal(list, item);
    return item;
}

void *linked_list_extract_tail(LinkedList *list) {
    LinkedListHeader *item = list->tail;
    if (item) linked_list_remove_internal(list, item);
    return item;
}

bool linked_list_check_validity(const LinkedList *list) {
    int count = 0;
    for (const LinkedListHeader *iter = list->head; iter; iter = iter->next) {
        ++count;
        if (iter->prev) {
            if (iter->prev->next != iter) return false;
        } else {
            if (list->head != iter) return false;
        }
        if (iter->next) {
            if (iter->next->prev != iter) return false;
        } else {
            if (list->tail != iter) return false;
        }
    }
    if (count != list->items) return false;
    return true;
}
