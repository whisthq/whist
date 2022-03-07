/**
 * @copyright Copyright 2022 Whist Technologies, Inc.
 * @file linked_list.h
 * @brief API for a doubly-linked list.
 */
#ifndef WHIST_UTILS_LINKED_LIST_H
#define WHIST_UTILS_LINKED_LIST_H
/*
============================
Usage
============================
*/
/**
 * @defgroup linked_list Linked Lists
 *
 * Generic C API for linked lists of structures.
 *
 * Example:
 * @code{.c}
 * // Define a structure which can be used as a list item.
 * typedef struct {
 *     LINKED_LIST_HEADER;
 *     int id;
 *     // More Foo fields.
 * } Foo;
 *
 * // Create a list.
 * LinkedList list;
 * linked_list_init(&list);
 *
 * // Add an item on the stack.
 * Foo stack_item;
 * linked_list_add_head(&list, &stack_item);
 *
 * // Add some items on the heap.
 * for (int i = 0; i < 10; i++) {
 *     Foo *heap_item = malloc(sizeof(*heap_item));
 *     linked_list_add_tail(&list, heap_item);
 * }
 *
 * // Iterate over the list.
 * linked_list_for_each (&foo_list, Foo, iter) {
 *     printf("This is the foo with ID %d.\n", iter->id);
 *     do_something_with_a_foo(iter);
 * }
 *
 * // Move the second item in the list to be the second-to-last.
 * Foo *item = linked_list_next(linked_list_head(&list));
 * linked_list_remove(&list, item);
 * linked_list_add_before(&list, linked_list_tail(&list), item);
 * @endcode
 *
 * @{
 */

/*
============================
Defines
============================
*/

/**
 * Header for linked list elements.
 *
 * This must be placed as the first item in any structure which will be
 * used as a linked list element.
 */
#define LINKED_LIST_HEADER LinkedListHeader linked_list_header

/**
 * Linked list header.
 *
 * @private
 * See LINKED_LIST_HEADER
 */
typedef struct LinkedListHeader {
    struct LinkedListHeader *next;
    struct LinkedListHeader *prev;
} LinkedListHeader;

/**
 * Linked list structure.
 *
 * This should be treated as opaque, but its internals are provided here
 * because it is convenient to be able to declare it on the stack or as
 * part of another structure.
 *
 * Zero-initialisation will correctly make an empty list, or it can be
 * done explicitly with linked_list_init().
 */
typedef struct LinkedList {
    LinkedListHeader *head;
    LinkedListHeader *tail;
    int items;
} LinkedList;

/**
 * Get the head item of a linked list, or NULL if empty.
 *
 * @param list  The list.
 */
#define linked_list_head(list) ((void *)(list)->head)

/**
 * Get the tail item of a linked list, or NULL if empty.
 *
 * @param list  The list.
 */
#define linked_list_tail(list) ((void *)(list)->tail)

/**
 * Get the next item of a linked list, or NULL if this was the tail.
 *
 * @param item  The item before the one you want.
 */
#define linked_list_next(item) ((void *)(item)->linked_list_header.next)

/**
 * Get the previous item of a linked list, or NULL if this was the head.
 *
 * @param item  The item after the one you want.
 */
#define linked_list_prev(item) ((void *)(item)->linked_list_header.prev)

/**
 * Add a new item at the head of a linked list.
 *
 * @param list  The list.
 * @param item  The item to add.
 */
#define linked_list_add_head(list, item) \
    linked_list_add_head_internal(list, &(item)->linked_list_header)

/**
 * Add a new item at the tail of a linked list.
 *
 * @param list  The list.
 * @param item  The item to add.
 */
#define linked_list_add_tail(list, item) \
    linked_list_add_tail_internal(list, &(item)->linked_list_header)

/**
 * Add a new item after another one in a linked list.
 *
 * If the old item is not in the list then the behaviour is undefined.
 *
 * @param list      The list.
 * @param old_item  The old item to add the new one after.
 * @param new_item  The new item to add.
 */
#define linked_list_add_after(list, old_item, new_item)                   \
    linked_list_add_after_internal(list, &(old_item)->linked_list_header, \
                                   &(new_item)->linked_list_header)

/**
 * Add a new item before another one in a linked list.
 *
 * If the old item is not in the list then the behaviour is undefined.
 *
 * @param list      The list.
 * @param old_item  The old item to add the new one before.
 * @param new_item  The new item to add.
 */
#define linked_list_add_before(list, old_item, new_item)                   \
    linked_list_add_before_internal(list, &(old_item)->linked_list_header, \
                                    &(new_item)->linked_list_header)

/**
 * Remove an item from a linked list.
 *
 * The item MUST be in the list before calling this function.  If the
 * item is not in the list, the behaviour is undefined.
 *
 * @param list  The list.
 * @param item  The item to remove.
 */
#define linked_list_remove(list, item) \
    linked_list_remove_internal(list, &(item)->linked_list_header)

/**
 * Iterate over a linked list.
 *
 * Use in place of a normal for-loop to have the body run for every item
 * in the list.  You can add to or remove from the list while the loop is
 * running, but moving an item may result in unexpected behaviour.
 *
 * Example:
 * @code{.c}
 * // foo_list is a list of Foo objects.
 *
 * linked_list_for_each (&foo_list, Foo, iter) {
 *     printf("This is the foo with ID %d.\n", iter->id);
 *     do_something_with_a_foo(iter);
 * }
 * @endcode
 */
#define linked_list_for_each(list, type, item)             \
    for (type *item = (type *)(list)->head, *next_ = NULL; \
         (item) && (next_ = (type *)linked_list_next(item)), (item); (item) = next_)

#ifdef _WIN32
// MSVC complains about the unavoidable assignment within the condition.
// This could be expanded anywhere so suppressing one instance doesn't
// work; hence switch if off if this header is included.
#pragma warning(disable : 4706)
#endif

/*
============================
Public Functions
============================
*/

/**
 * Initialise a linked list to be empty.
 *
 * @param list  The list to initialise.
 */
void linked_list_init(LinkedList *list);

/**
 * Get the number of items in a linked list.
 *
 * @param list  The list to query.
 * @returns     The number of items in the list.
 */
int linked_list_size(const LinkedList *list);

/**
 * Extract the item at the head of a linked list.
 *
 * @param list  The list to get the item from.
 * @returns     The old head item of the list, or NULL if the list was empty.
 */
void *linked_list_extract_head(LinkedList *list);

/**
 * Extract the item at the tail of a linked list.
 *
 * @param list  The list to get the item from.
 * @returns     The old tail item of the list, or NULL if the list was empty.
 */
void *linked_list_extract_tail(LinkedList *list);

/**
 * Sort a linked list.
 *
 * For short lists - uses a O(N^2) insertion sort.
 *
 * @param list  The list to sort.
 * @param cmp   Comparator function.  It is given two arguments, and
 *              should return a negative number, zero, or a positive
 *              number if the first argument is less than, equal to, or
 *              greater than, respectively, the second argument.
 */
void linked_list_sort(LinkedList *list, int (*cmp)(const void *first, const void *second));

/**
 * Perform validity checks on a linked list.
 *
 * This ensures that all of the list pointers match up and that the item
 * count is correct.  This should not be used in production, but can be
 * useful in debugging if you believe you may have messed up the state
 * of your list.
 *
 * @param list  The list to check.
 */
bool linked_list_check_validity(const LinkedList *list);

/**
 * @privatesection
 * All functions after this point are internal, being implementation
 * details for the correspondingly-named macros above.
 */
void linked_list_add_head_internal(LinkedList *list, LinkedListHeader *item);
void linked_list_add_tail_internal(LinkedList *list, LinkedListHeader *item);
void linked_list_add_after_internal(LinkedList *list, LinkedListHeader *old_item,
                                    LinkedListHeader *new_item);
void linked_list_add_before_internal(LinkedList *list, LinkedListHeader *old_item,
                                     LinkedListHeader *new_item);
void linked_list_remove_internal(LinkedList *list, LinkedListHeader *item);

/** @} */

#endif /* WHIST_UTILS_LINKED_LIST_H */
