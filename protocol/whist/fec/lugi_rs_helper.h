#pragma once
#include "whist/fec/lugi_rs.h"

void lugi_rs_helper_init(void);

/**
 * @brief                          Gets an rs_code
 *
 * @param k                        The number of original packets
 * @param n                        The total number of packets
 *
 * @returns                        The RSCode for that (n, k) tuple
 */
RSCode *get_rs_code(int k, int n);

/**
 * @brief                          Frees an RSTable
 *
 * @param opaque                   The RSTable* to free
 */
void free_rs_code_table(void *opaque);
