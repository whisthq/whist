#ifndef RS_WRAPPER_H
#define RS_WRAPPER_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file rs_wrapper.h
 * @brief This file contains the FEC API Interface
 * @note See protocol_test.cpp for usage
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
RSWrapper * rs_wrapper_create(int num_real_buffers, int num_total_buffers);

void rs_wrapper_encode(RSWrapper * rs_wrapper, void **src, void**dst, int sz);

int rs_wrapper_decode(RSWrapper * rs_wrapper, void **pkt, int * index, int num_pkt, int sz);

void rs_wrapper_can_decode_reinit(RSWrapper * rs_wrapper);

void rs_wrapper_can_decode_register_index(RSWrapper * rs_wrapper,int index);

bool rs_wrapper_can_decode(RSWrapper * rs_wrapper);

void rs_wrapper_test(void);

void init_rs_wrapper(void);

#endif
