#pragma once

/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file rs_common.h
 * @brief This file contains some defines that are used across RS(Reed-Solomon) related codes
 */

// the size of GF^8, which is the field size almost all RS(Reed-Solomon) libs use. (except FFT based
// RS)
//
#define RS_FIELD_SIZE 256

// temp workaround of for cm256 lib's CPU dispatching
// TODO: fix cm256
#define WHIST_FEC_ERROR_AVX2_NOT_SUPPORTED -100
