/*
 * This file contains the headers of a simple generic doubly-linked list
 * used by the hole punching server to keep track of incoming peer requests for
 * pairing with a VM.
 *
 * Hole Punching Server version: 1.0
 *
 * Last modified: 12/27/2019
 *
 * By: Philippe Noël
 *
 * Copyright Fractal Computers, Inc. 2019
**/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// generic linked list node type
struct gll_node_t {
  void *data;
  struct gll_node_t *prev;
  struct gll_node_t *next;
};

// generic linked list type
struct gll_t {
  int size;
  struct gll_node_t *first;
  struct gll_node_t *last;
};

// @brief initializes a new linked list
// @details returns a pointer to the new list
struct gll_t *gll_init(void);

// @brief initialize a new node
// @details takes in a pointer to the data and returns a pointer to the new node
struct gll_node_t *gll_init_node(void *data);

// @brief find node at a given position
// @details takes in a pointer to a list and a position and returns a pointer to
// the node or NULL on failure
struct gll_node_t *gll_find_node(struct gll_t *list, int pos);

// @brief add an element to the end of a list
// @details takes in a pointer to a list and a pointer to the data returns 0 on
// success, -1 on failure
int gll_push_end(struct gll_t *list, void *data);

// @bief remove a node from an arbitrary position
// @details takes in a pointer to a list and a poiner to data and return 0 on
// success, -1 on failure
void *gll_remove(struct gll_t *list, int pos);

// @brief destroys a list and frees all related memory, but not data stored
// @details takes in a pointer to a list
void gll_destroy(struct gll_t *list);
