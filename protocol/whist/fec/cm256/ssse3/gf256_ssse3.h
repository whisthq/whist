#pragma once
#include "../gf256_common.h"

void gf256_mul_mem_inner_ssse3(GF256_M128 * GF256_RESTRICT &z16, const GF256_M128 * GF256_RESTRICT &x16, 
                                uint8_t &y, int &bytes);

void gf256_muladd_mem_inner_ssse3(GF256_M128 * GF256_RESTRICT &z16, const GF256_M128 * GF256_RESTRICT &x16, 
                                uint8_t &y, int &bytes);
