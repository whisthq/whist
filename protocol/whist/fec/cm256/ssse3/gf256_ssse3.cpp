#include "../gf256_common.h"
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * file gf256_ssse3.cpp
 * @brief Seperate out all ssse3 instructions of gf256 into a single file, in order to make the simd fallback work correctly.
 */

void gf256_mul_mem_inner_ssse3(GF256_M128 * GF256_RESTRICT &z16, const GF256_M128 * GF256_RESTRICT &x16, 
                                uint8_t &y, int &bytes)
{
        // Partial product tables; see above
        const GF256_M128 table_lo_y = _mm_loadu_si128(GF256Ctx.MM128.TABLE_LO_Y + y);
        const GF256_M128 table_hi_y = _mm_loadu_si128(GF256Ctx.MM128.TABLE_HI_Y + y);

        // clr_mask = 0x0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f
        const GF256_M128 clr_mask = _mm_set1_epi8(0x0f);

        // Handle multiples of 16 bytes
        do
        {
            // See above comments for details
            GF256_M128 x0 = _mm_loadu_si128(x16);
            GF256_M128 l0 = _mm_and_si128(x0, clr_mask);
            x0 = _mm_srli_epi64(x0, 4);
            GF256_M128 h0 = _mm_and_si128(x0, clr_mask);
            l0 = _mm_shuffle_epi8(table_lo_y, l0);
            h0 = _mm_shuffle_epi8(table_hi_y, h0);
            _mm_storeu_si128(z16, _mm_xor_si128(l0, h0));

            bytes -= 16, ++x16, ++z16;
        } while (bytes >= 16);
}

void gf256_muladd_mem_inner_ssse3(GF256_M128 * GF256_RESTRICT &z16, const GF256_M128 * GF256_RESTRICT &x16, 
                                uint8_t &y, int &bytes)
{
        // Partial product tables; see above
        const GF256_M128 table_lo_y = _mm_loadu_si128(GF256Ctx.MM128.TABLE_LO_Y + y);
        const GF256_M128 table_hi_y = _mm_loadu_si128(GF256Ctx.MM128.TABLE_HI_Y + y);

        // clr_mask = 0x0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f
        const GF256_M128 clr_mask = _mm_set1_epi8(0x0f);

        // This unroll seems to provide about 7% speed boost when AVX2 is disabled
        while (bytes >= 32)
        {
            bytes -= 32;

            GF256_M128 x1 = _mm_loadu_si128(x16 + 1);
            GF256_M128 l1 = _mm_and_si128(x1, clr_mask);
            x1 = _mm_srli_epi64(x1, 4);
            GF256_M128 h1 = _mm_and_si128(x1, clr_mask);
            l1 = _mm_shuffle_epi8(table_lo_y, l1);
            h1 = _mm_shuffle_epi8(table_hi_y, h1);
            const GF256_M128 z1 = _mm_loadu_si128(z16 + 1);

            GF256_M128 x0 = _mm_loadu_si128(x16);
            GF256_M128 l0 = _mm_and_si128(x0, clr_mask);
            x0 = _mm_srli_epi64(x0, 4);
            GF256_M128 h0 = _mm_and_si128(x0, clr_mask);
            l0 = _mm_shuffle_epi8(table_lo_y, l0);
            h0 = _mm_shuffle_epi8(table_hi_y, h0);
            const GF256_M128 z0 = _mm_loadu_si128(z16);

            const GF256_M128 p1 = _mm_xor_si128(l1, h1);
            _mm_storeu_si128(z16 + 1, _mm_xor_si128(p1, z1));

            const GF256_M128 p0 = _mm_xor_si128(l0, h0);
            _mm_storeu_si128(z16, _mm_xor_si128(p0, z0));

            x16 += 2, z16 += 2;
        }

        // Handle multiples of 16 bytes
        while (bytes >= 16)
        {
            // See above comments for details
            GF256_M128 x0 = _mm_loadu_si128(x16);
            GF256_M128 l0 = _mm_and_si128(x0, clr_mask);
            x0 = _mm_srli_epi64(x0, 4);
            GF256_M128 h0 = _mm_and_si128(x0, clr_mask);
            l0 = _mm_shuffle_epi8(table_lo_y, l0);
            h0 = _mm_shuffle_epi8(table_hi_y, h0);
            const GF256_M128 p0 = _mm_xor_si128(l0, h0);
            const GF256_M128 z0 = _mm_loadu_si128(z16);
            _mm_storeu_si128(z16, _mm_xor_si128(p0, z0));

            bytes -= 16, ++x16, ++z16;
        }
}
