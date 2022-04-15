#pragma once
#include "../gf256_common.h"
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file gf256_ssse3.cpp
 * @brief Seperate out all ssse3 instructions of gf256 into a single file, in order to make the simd fallback work correctly.
 */

void gf256_mul_mem_inner_ssse3(GF256_M128 * GF256_RESTRICT &z16, const GF256_M128 * GF256_RESTRICT &x16, 
                                uint8_t &y, int &bytes);

void gf256_muladd_mem_inner_ssse3(GF256_M128 * GF256_RESTRICT &z16, const GF256_M128 * GF256_RESTRICT &x16, 
                                uint8_t &y, int &bytes);
