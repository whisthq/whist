#pragma once
#include "../gf256_common.h"

void gf256_mul_mem_init_inner_avx2(const GF256_M128 &table_lo, const GF256_M128 &table_hi,int &y);

void gf256_add_mem_inner_avx2(GF256_M128 * GF256_RESTRICT &x16,
                              const GF256_M128 * GF256_RESTRICT &y16, int & bytes);

void gf256_add2_mem_innder_avx2(GF256_M128 * GF256_RESTRICT &z16, const GF256_M128 * GF256_RESTRICT &x16, const GF256_M128 * GF256_RESTRICT &y16, int &bytes);

void gf256_addset_mem_innder_avx2(GF256_M128 * GF256_RESTRICT &z16, const GF256_M128 * GF256_RESTRICT &x16, const GF256_M128 * GF256_RESTRICT &y16, int &bytes);

void gf256_mul_mem_innder_avx2(void * GF256_RESTRICT &vz, const void * GF256_RESTRICT &vx, 
                                GF256_M128 * GF256_RESTRICT &z16, const GF256_M128 * GF256_RESTRICT &x16, 
                                uint8_t &y, int &bytes);

void gf256_muladd_mem_inner_avx2(GF256_M128 * GF256_RESTRICT &z16, const GF256_M128 * GF256_RESTRICT &x16, 
                                uint8_t &y, int &bytes);