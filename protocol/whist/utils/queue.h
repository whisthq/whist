/**
 * @copyright Copyright 2022 Whist Technologies, Inc.
 * @file queue.h
 * @brief API of thread-safe fifo queue.
 */
#ifndef WHIST_UTILS_QUEUE_H
#define WHIST_UTILS_QUEUE_H

typedef struct QueueContext QueueContext;

/**
 * @brief                          Create a FIFO queue
 *
 * @param item_size                Size of each item in the queue
 * @param max_items                Maximum number of items in the queue
 *
 * @returns                        Opaque context pointer for the queue
 */
QueueContext *fifo_queue_create(size_t item_size, int max_items);

/**
 * @brief                          Enqueue an item to the FIFO queue
 *
 * @param queue_context            Queue's context pointer
 * @param item                     Pointer to the item that needs to be enqueued
 *
 * @returns                        0 on success, -1 on failure
 */
int fifo_queue_enqueue_item(QueueContext *queue_context, const void *item);

/**
 * @brief                          Dequeue an item from the FIFO queue
 *
 * @param queue_context            Queue's context pointer
 * @param item                     Pointer to the memory where dequeued item will be stored
 *
 * @returns                        0 on success, -1 on failure
 */
int fifo_queue_dequeue_item(QueueContext *queue_context, void *item);

/**
 * @brief                          Destroys the FIFO queue
 *
 * @param queue_context            Queue's context pointer
 *
 */
void fifo_queue_destroy(QueueContext *queue_context);

#endif
