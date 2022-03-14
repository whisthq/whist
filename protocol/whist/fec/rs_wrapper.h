#pragma once
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file rs_wrapper.h
 * @brief This file contains a wrapp of the original rs lib without the <=256 limitation
 */

/*
============================
Defines
============================
*/

typedef struct RSWrapper RSWrapper;

/*
============================
Public Functions
============================
*/

// init the rs_wrapper subsystem
void init_rs_wrapper(void);

/*
the below 4 functions are wrappers of the original rs_lib, with a very similiar interface
*/

/**
 * @brief                          Create an RS wrapper, the rs_wapper provide an interface very
 * similar to the underlying RS lib but don't have the limitation of num_total_buffers<=256. when
 * num_total_buffers is too large, the orginal data are partition into groups, encoding and decoding
 * are done group-wise
 *
 * @param num_real_buffers         num of original buffers
 * @param num_total_buffers        num of buffers in total
 *
 * @returns                        The RS wrapper
 */
RSWrapper *rs_wrapper_create(int num_real_buffers, int num_total_buffers);

// destroy the rs_wrapper
void rs_wrapper_destory(RSWrapper *rs_wrapper);

// do encode with the RS wrapper
void rs_wrapper_encode(RSWrapper *rs_wrapper, void **src, void **dst, int sz);

// do decode with the RS wrapper
int rs_wrapper_decode(RSWrapper *rs_wrapper, void **pkt, int *index, int num_pkt, int sz);

// the below 3 functions are used to help detect if decode can happen, in O(1) time
// they are necessary bc num_real_buffers of buffers don't guarantee to recovery any more

// register an index as "received"
void rs_wrapper_decode_helper_register_index(RSWrapper *rs_wrapper, int index);

// detect if decode can happen
bool rs_wrapper_decode_helper_can_decode(RSWrapper *rs_wrapper);

// reset the counters inside rs_wrapper, so that you can use it from scratch
void rs_wrapper_decode_helper_reset(RSWrapper *rs_wrapper);

/*
the below functions are mainly for testing
*/

// set the inner rs_wrapper_max_group_size to the given value
// returns the old value
int rs_wrapper_set_max_group_size(int value);

double rs_wrapper_set_max_group_overhead(double value);

void rs_wrapper_set_verbose_log(int value);
/*
//warp up the rs_table inside, helpful for measuring performance
void rs_table_warmup(void);
*/
