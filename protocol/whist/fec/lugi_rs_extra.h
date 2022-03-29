#pragma once
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file lugi_rs_extra.h
 * @brief This file contains helper functions to make the lugi rs easier to use
 */

#include "whist/fec/lugi_rs.h"

/**
 * @brief                          init the lugi_rs_extra subsystem.
 *
 * @note                           this function calls init_rs() of lugi's RS. No need to call
 *                                 init_rs() again.
 */

void lugi_rs_extra_init(void);

/**
 * @brief                          Gets an rs_code from a cached table
 *
 * @param k                        The number of original packets
 * @param n                        The total number of packets
 *
 * @returns                        The RSCode for that (n, k) tuple
 */
RSCode *lugi_rs_extra_get_rs_code(int k, int n);

/**
 * @brief                          Frees an RSTable
 *
 * @param opaque                   The RSTable* to free
 */
void lugi_rs_extra_free_rs_code_table(void *opaque);
