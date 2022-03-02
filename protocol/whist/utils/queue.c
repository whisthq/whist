/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file queue.c
 * @brief Implementation of thread-safe fifo queue.
 */

#include "whist/logging/logging.h"
#include "whist/utils/queue.h"

typedef struct QueueContext {
    int read_idx;
    int write_idx;
    size_t item_size;
    int num_items;
    int max_items;
    WhistMutex mutex;
    void *data;
} QueueContext;

static void increment_idx(QueueContext *context, int *idx) {
    if (++*idx == context->max_items) {
        *idx = 0;
    }
}

QueueContext *fifo_queue_create(size_t item_size, int max_items) {
    QueueContext *context = calloc(1, sizeof(QueueContext));
    if (context == NULL) {
        return NULL;
    }

    context->data = malloc(item_size * max_items);
    if (context->data == NULL) {
        fifo_queue_destroy(context);
        return NULL;
    }

    context->mutex = whist_create_mutex();
    if (context->mutex == NULL) {
        fifo_queue_destroy(context);
        return NULL;
    }
    context->item_size = item_size;
    context->max_items = max_items;
    return context;
}

int fifo_queue_enqueue_item(QueueContext *context, const void *item) {
    if (context == NULL) {
        return -1;
    }
    whist_lock_mutex(context->mutex);
    if (context->num_items >= context->max_items) {
        whist_unlock_mutex(context->mutex);
        return -1;
    }
    context->num_items++;
    void *target_item = (uint8_t *)context->data + (context->item_size * context->write_idx);
    memcpy(target_item, item, context->item_size);
    increment_idx(context, &context->write_idx);
    whist_unlock_mutex(context->mutex);
    return 0;
}

int fifo_queue_dequeue_item(QueueContext *context, void *item) {
    if (context == NULL) {
        return -1;
    }
    whist_lock_mutex(context->mutex);
    if (context->num_items <= 0) {
        whist_unlock_mutex(context->mutex);
        return -1;
    }
    context->num_items--;
    void *source_item = (uint8_t *)context->data + (context->item_size * context->read_idx);
    memcpy(item, source_item, context->item_size);
    increment_idx(context, &context->read_idx);
    whist_unlock_mutex(context->mutex);
    return 0;
}

void fifo_queue_destroy(QueueContext *context) {
    if (context == NULL) {
        return;
    }
    if (context->data != NULL) {
        free(context->data);
    }
    if (context->mutex != NULL) {
        whist_destroy_mutex(context->mutex);
    }
    free(context);
}
