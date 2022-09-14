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
    WhistCondition avail_items_cond;
    WhistCondition avail_space_cond;
    void *data;
    bool destroying;
} QueueContext;

static void increment_idx(QueueContext *context, int *idx) {
    if (++*idx == context->max_items) {
        *idx = 0;
    }
}

static void dequeue_item(QueueContext *context, void *item) {
    context->num_items--;
    void *source_item = (uint8_t *)context->data + (context->item_size * context->read_idx);
    memcpy(item, source_item, context->item_size);
    increment_idx(context, &context->read_idx);
    whist_broadcast_cond(context->avail_space_cond);
}

static void enqueue_item(QueueContext *context, void *item) {
    context->num_items++;
    void *target_item = (uint8_t *)context->data + (context->item_size * context->write_idx);
    memcpy(target_item, item, context->item_size);
    increment_idx(context, &context->write_idx);
    whist_broadcast_cond(context->avail_items_cond);
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

    context->avail_items_cond = whist_create_cond();
    if (context->avail_items_cond == NULL) {
        fifo_queue_destroy(context);
        return NULL;
    }

    context->avail_space_cond = whist_create_cond();
    if (context->avail_space_cond == NULL) {
        fifo_queue_destroy(context);
        return NULL;
    }

    context->item_size = item_size;
    context->max_items = max_items;
    context->destroying = false;
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
    enqueue_item(context, item);
    whist_unlock_mutex(context->mutex);
    return 0;
}

int fifo_queue_enqueue_item_timeout(QueueContext *context, const void *item, int timeout_ms) {
    if (context == NULL) {
        return -1;
    }
    WhistTimer timer;
    start_timer(&timer);
    int current_timeout_ms = timeout_ms;
    whist_lock_mutex(context->mutex);
    while (context->num_items >= context->max_items) {
        if (context->destroying) {
            whist_unlock_mutex(context->mutex);
            return -1;
        }
        if (timeout_ms >= 0) {
            bool res = whist_timedwait_cond(context->avail_space_cond, context->mutex, current_timeout_ms);
            if (res == false) {  // In case of a timeout simply exit
                whist_unlock_mutex(context->mutex);
                return -1;
            }
            int elapsed_ms = (int)(get_timer(&timer) * MS_IN_SECOND);
            current_timeout_ms = max(timeout_ms - elapsed_ms, 0);
        } else {
            // Negative timeout_ms indicates block until available, not timeout
            whist_wait_cond(context->avail_space_cond, context->mutex);
        }
    }
    enqueue_item(context, item);
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
    dequeue_item(context, item);
    whist_unlock_mutex(context->mutex);
    whist_post_semaphore(context->sem);
    return 0;
}

int fifo_queue_dequeue_item_timeout(QueueContext *context, void *item, int timeout_ms) {
    if (context == NULL) {
        return -1;
    }
    WhistTimer timer;
    start_timer(&timer);
    int current_timeout_ms = timeout_ms;
    whist_lock_mutex(context->mutex);
    while (context->num_items <= 0) {
        if (context->destroying) {
            whist_unlock_mutex(context->mutex);
            return -1;
        }
        if (timeout_ms >= 0) {
            bool res = whist_timedwait_cond(context->avail_items_cond, context->mutex, current_timeout_ms);
            if (res == false) {  // In case of a timeout simply exit
                whist_unlock_mutex(context->mutex);
                return -1;
            }
            int elapsed_ms = (int)(get_timer(&timer) * MS_IN_SECOND);
            current_timeout_ms = max(timeout_ms - elapsed_ms, 0);
        } else {
            // Negative timeout_ms indicates block until available, not timeout
            whist_wait_cond(context->avail_items_cond, context->mutex);
        }
    }
    dequeue_item(context, item);
    whist_unlock_mutex(context->mutex);
    return 0;
}

void fifo_queue_destroy(QueueContext *context) {
    if (context == NULL) {
        return;
    }

    // Make sure that all blocking calls release
    context->destroying = true;
    whist_broadcast_cond(context->avail_items_cond);
    whist_broadcast_cond(context->avail_space_cond);

    if (context->data != NULL) {
        free(context->data);
    }
    if (context->mutex != NULL) {
        whist_destroy_mutex(context->mutex);
    }
    if (context->cond != NULL) {
        whist_destroy_cond(context->cond);
    }
    free(context);
}
