/*
 * This file contains the implementations of a simple generic doubly-linked list
 * used by the hole punching server to keep track of incoming peer requests for
 * pairing with a VM.
 *
 * Hole Punching Server version: 1.0
 *
 * Last modified: 12/24/2019
 *
 * By: Philippe Noël
 *
 * Copyright Fractal Computers, Inc. 2019
**/

#include "linkedlist.h" // header file for this file

// @brief initializes a new linked list
// @details returns a pointer to the new list
struct gll_t *gll_init() {
  struct gll_t *list = (struct gll_t *) malloc(sizeof(struct gll_t));
  list->size = 0; // empty list
  list->first = NULL;
  list->last = NULL;
  return list;
}

// @brief initialize a new node
// @details takes in a pointer to the data and returns a pointer to the new node
struct gll_node_t *gll_init_node(void *data) {
  struct gll_node_t *node = (struct gll_node_t *) malloc(sizeof(struct gll_node_t));
  node->data = data; // set data
  node->prev = NULL;
  node->next = NULL;
  return node;
}

// @brief find node at a given position
// @details takes in a pointer to a list and a position and returns a pointer to
// the node or NULL on failure
struct gll_node_t *gll_find_node(struct gll_t *list, int pos) {
  // sanity check
  if (pos > list->size) {
    return NULL;
  }

  struct gll_node_t *currNode;
  int currPos;
  int reverse;

  // decide where to start iterating from (font or back of the list)
  if (pos > ((list->size - 1) / 2)) {
    reverse = 1;
    currPos = list->size - 1;
    currNode = list->last;
  } else {
    reverse = 0;
    currPos = 0;
    currNode = list->first;
  }

  while (currNode != NULL) {
    if (currPos == pos)
      break;

    // find node
    currNode = (reverse ? (currNode->prev) : (currNode->next));
    currPos  = (reverse ? (currPos - 1) : (currPos + 1));
  }
  return currNode;
}

// @brief add an element to the end of a list
// @details takes in a pointer to a list and a pointer to the data returns 0 on
// success, -1 on failure
int gll_push_end(struct gll_t *list, void *data) {
  // initialize new node
  struct gll_node_t *newNode = gll_init_node(data);

  // if list is empty
  if (list->size == 0) {
    list->first = newNode;
  }
  else {
    // if there is at least one element
    list->last->next = newNode;
    newNode->prev = list->last;
  }
  list->last = newNode;
  list->size++;
  return 0;
}

// @bief remove a node from an arbitrary position
// @details takes in a pointer to a list and a poiner to data and return 0 on
// success, -1 on failure
void *gll_remove(struct gll_t *list, int pos) {
  struct gll_node_t *currNode = gll_find_node(list, pos);
  void *data = NULL;

  if (currNode == NULL) {
    return NULL;
  }

  // keep iterating to remove
  data = currNode->data;

  if (currNode->prev == NULL) {
    list->first = currNode->next;
  }
  else {
    currNode->prev->next = currNode->next;
  }

  if (currNode->next == NULL) {
    list->last = currNode->prev;
  }
  else {
    currNode->next->prev = currNode->prev;
  }

  list->size--;
  free(currNode);
  return data;
}

// @brief destroys a list and frees all related memory, but not data stored
// @details takes in a pointer to a list
void gll_destroy(struct gll_t *list) {
  struct gll_node_t *currNode = list->first;
  struct gll_node_t *nextNode;

  // walk the list and free
  while (currNode != NULL) {
    nextNode = currNode->next;
    free(currNode);
    currNode = nextNode;
  }
  free(list);
}
