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

// the RSwrapper object
typedef struct RSWrapper RSWrapper;

/*
============================
Public Functions
============================
*/

// init the rs_wrapper subsystem

/**
 * @brief                          init the rs wrapper
 *
 * @note                           this fuction calls the initialization of underlying rs lib,
 *                                 no need and shouldn't call the underlying functions again
 */
void init_rs_wrapper(void);

/*
the below 4 functions are wrappers of the original rs_lib, with a very similiar interface
*/

/**
 * @brief                          Create an RS wrapper, the rs_wapper provide an interface very
 *                                 similar to the lugi's RS lib but don't have the limitation of
 *                                 num_total_buffers<=256. when num_total_buffers is too large,
 *                                 the orginal data are partition into groups, encoding and
 *                                 decoding are done group-wise.
 *
 * @param num_real_buffers         num of original buffers
 * @param num_total_buffers        num of buffers in total
 *
 * @returns                        The RS wrapper
 */
RSWrapper *rs_wrapper_create(int num_real_buffers, int num_total_buffers);

/**
 * @brief                          do RS encode with the RS wrapper
 *
 * @param rs_wrapper               RSwrapper object created by rs_wrapper_create()
 * @param src                      an arrary of original buffers
 * @param dst                      an arrary of redundant buffers, the memeroy should be already
 *                                 allocated before passing here
 * @param sz                       size of buffers
 */
void rs_wrapper_encode(RSWrapper *rs_wrapper, void **src, void **dst, int sz);

/**
 * @brief                          do RS decode with the RS wrapper
 *
 * @param rs_wrapper               RSwrapper object created by rs_wrapper_create()
 * @param pkt                      an array of input buffers, may contain orginal buffer and
 *                                 redundant buffer in any order. used as output as well
 * @param index                    an array of index of buffers
 * @param num_pkt                  num of input buffers
 * @param sz                       size of buffers
 *
 * @returns                        zero on success
 *
 * @note                           after success, all orginal buffers are store inside the
 *                                 first num_real_buffers position of pkt, in correct
 *                                 order
 */
int rs_wrapper_decode(RSWrapper *rs_wrapper, void **pkt, int *index, int num_pkt, int sz);

/**
 * @brief                          destroy the rs_wrapper
 *
 * @param  rs_wrapper              RSwrapper object created by rs_wrapper_create()
 */
void rs_wrapper_destory(RSWrapper *rs_wrapper);

/*
the below 3 functions are used for help detect if decode can happen, in O(1) time
they are necessary bc num_real_buffers of buffers don't guarantee to recovery any more
*/

/**
 * @brief                          part of the decoder helper, register an index as "received"
 *
 * @param  rs_wrapper              RSwrapper object created by rs_wrapper_create()
 * @param  index                   the index of packet that is considered as receviced
 */
void rs_wrapper_decode_helper_register_index(RSWrapper *rs_wrapper, int index);

/**
 * @brief                          part of the decoder helper, detect if decode can happen
 *
 * @param  rs_wrapper              RSwrapper object created by rs_wrapper_create()
 */
bool rs_wrapper_decode_helper_can_decode(RSWrapper *rs_wrapper);

/**
 * @brief                          part of the decoder helper, reset the counters inside rs_wrapper,
 * so that you can use it from scratch
 *
 * @param  rs_wrapper              RSwrapper object created by rs_wrapper_create()
 */
void rs_wrapper_decode_helper_reset(RSWrapper *rs_wrapper);

/*
============================
Public Functions, only for testing
============================
*/

// set the inner rs_wrapper_max_group_size to the given value
// returns the old value
int rs_wrapper_set_max_group_size(int value);

// set the inner rs_wrapper_set_max_group_overhead to the given value
// returns the old value
double rs_wrapper_set_max_group_overhead(double value);

// enable the internal verbose logging
void rs_wrapper_set_verbose_log(int value);
