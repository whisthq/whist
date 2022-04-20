#include "../gf256_common.h"
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * file gf256_avx2.cpp
 * @brief Seperate out all avx2 instructions of gf256 into a single file, in order to make the simd fallback work correctly.
 */

/** \file
    \brief GF(256) Main C API Source
    \copyright Copyright (c) 2017 Christopher A. Taylor.  All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of GF256 nor the names of its contributors may be
      used to endorse or promote products derived from this software without
      specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/

void gf256_mul_mem_init_inner_avx2(const GF256_M128 &table_lo, const GF256_M128 &table_hi,int &y)
{
        const GF256_M256 table_lo2 = _mm256_broadcastsi128_si256(table_lo);
        const GF256_M256 table_hi2 = _mm256_broadcastsi128_si256(table_hi);
        _mm256_storeu_si256(GF256Ctx.MM256.TABLE_LO_Y + y, table_lo2);
        _mm256_storeu_si256(GF256Ctx.MM256.TABLE_HI_Y + y, table_hi2);
}

void gf256_add_mem_inner_avx2(GF256_M128 * GF256_RESTRICT &x16,
                              const GF256_M128 * GF256_RESTRICT &y16, int & bytes)
{
        GF256_M256 * GF256_RESTRICT x32 = reinterpret_cast<GF256_M256 *>(x16);
        const GF256_M256 * GF256_RESTRICT y32 = reinterpret_cast<const GF256_M256 *>(y16);

        while (bytes >= 128)
        {
            GF256_M256 x0 = _mm256_loadu_si256(x32);
            GF256_M256 y0 = _mm256_loadu_si256(y32);
            x0 = _mm256_xor_si256(x0, y0);
            GF256_M256 x1 = _mm256_loadu_si256(x32 + 1);
            GF256_M256 y1 = _mm256_loadu_si256(y32 + 1);
            x1 = _mm256_xor_si256(x1, y1);
            GF256_M256 x2 = _mm256_loadu_si256(x32 + 2);
            GF256_M256 y2 = _mm256_loadu_si256(y32 + 2);
            x2 = _mm256_xor_si256(x2, y2);
            GF256_M256 x3 = _mm256_loadu_si256(x32 + 3);
            GF256_M256 y3 = _mm256_loadu_si256(y32 + 3);
            x3 = _mm256_xor_si256(x3, y3);

            _mm256_storeu_si256(x32, x0);
            _mm256_storeu_si256(x32 + 1, x1);
            _mm256_storeu_si256(x32 + 2, x2);
            _mm256_storeu_si256(x32 + 3, x3);

            bytes -= 128, x32 += 4, y32 += 4;
        }

        // Handle multiples of 32 bytes
        while (bytes >= 32)
        {
            // x[i] = x[i] xor y[i]
            _mm256_storeu_si256(x32,
                _mm256_xor_si256(
                    _mm256_loadu_si256(x32),
                    _mm256_loadu_si256(y32)));

            bytes -= 32, ++x32, ++y32;
        }

        x16 = reinterpret_cast<GF256_M128 *>(x32);
        y16 = reinterpret_cast<const GF256_M128 *>(y32);
}

void gf256_add2_mem_inner_avx2(GF256_M128 * GF256_RESTRICT &z16, const GF256_M128 * GF256_RESTRICT &x16, const GF256_M128 * GF256_RESTRICT &y16, int &bytes)
{
        GF256_M256 * GF256_RESTRICT z32 = reinterpret_cast<GF256_M256 *>(z16);
        const GF256_M256 * GF256_RESTRICT x32 = reinterpret_cast<const GF256_M256 *>(x16);
        const GF256_M256 * GF256_RESTRICT y32 = reinterpret_cast<const GF256_M256 *>(y16);

        const unsigned count = bytes / 32;
        for (unsigned i = 0; i < count; ++i)
        {
            _mm256_storeu_si256(z32 + i,
                _mm256_xor_si256(
                    _mm256_loadu_si256(z32 + i),
                    _mm256_xor_si256(
                        _mm256_loadu_si256(x32 + i),
                        _mm256_loadu_si256(y32 + i))));
        }

        bytes -= count * 32;
        z16 = reinterpret_cast<GF256_M128 *>(z32 + count);
        x16 = reinterpret_cast<const GF256_M128 *>(x32 + count);
        y16 = reinterpret_cast<const GF256_M128 *>(y32 + count);
}

void gf256_addset_mem_inner_avx2(GF256_M128 * GF256_RESTRICT &z16, const GF256_M128 * GF256_RESTRICT &x16, const GF256_M128 * GF256_RESTRICT &y16, int &bytes)
{
        GF256_M256 * GF256_RESTRICT z32 = reinterpret_cast<GF256_M256 *>(z16);
        const GF256_M256 * GF256_RESTRICT x32 = reinterpret_cast<const GF256_M256 *>(x16);
        const GF256_M256 * GF256_RESTRICT y32 = reinterpret_cast<const GF256_M256 *>(y16);

        const unsigned count = bytes / 32;
        for (unsigned i = 0; i < count; ++i)
        {
            _mm256_storeu_si256(z32 + i,
                _mm256_xor_si256(
                    _mm256_loadu_si256(x32 + i),
                    _mm256_loadu_si256(y32 + i)));
        }

        bytes -= count * 32;
        z16 = reinterpret_cast<GF256_M128 *>(z32 + count);
        x16 = reinterpret_cast<const GF256_M128 *>(x32 + count);
        y16 = reinterpret_cast<const GF256_M128 *>(y32 + count);
}

void gf256_mul_mem_inner_avx2(void * GF256_RESTRICT &vz, const void * GF256_RESTRICT &vx,
                                GF256_M128 * GF256_RESTRICT &z16, const GF256_M128 * GF256_RESTRICT &x16, 
                                uint8_t &y, int &bytes)
{
            // Partial product tables; see above
        const GF256_M256 table_lo_y = _mm256_loadu_si256(GF256Ctx.MM256.TABLE_LO_Y + y);
        const GF256_M256 table_hi_y = _mm256_loadu_si256(GF256Ctx.MM256.TABLE_HI_Y + y);

        // clr_mask = 0x0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f
        const GF256_M256 clr_mask = _mm256_set1_epi8(0x0f);

        GF256_M256 * GF256_RESTRICT z32 = reinterpret_cast<GF256_M256 *>(vz);
        const GF256_M256 * GF256_RESTRICT x32 = reinterpret_cast<const GF256_M256 *>(vx);

        // Handle multiples of 32 bytes
        do
        {
            // See above comments for details
            GF256_M256 x0 = _mm256_loadu_si256(x32);
            GF256_M256 l0 = _mm256_and_si256(x0, clr_mask);
            x0 = _mm256_srli_epi64(x0, 4);
            GF256_M256 h0 = _mm256_and_si256(x0, clr_mask);
            l0 = _mm256_shuffle_epi8(table_lo_y, l0);
            h0 = _mm256_shuffle_epi8(table_hi_y, h0);
            _mm256_storeu_si256(z32, _mm256_xor_si256(l0, h0));

            bytes -= 32, ++x32, ++z32;
        } while (bytes >= 32);

        z16 = reinterpret_cast<GF256_M128 *>(z32);
        x16 = reinterpret_cast<const GF256_M128 *>(x32);
}

void gf256_muladd_mem_inner_avx2(GF256_M128 * GF256_RESTRICT &z16, const GF256_M128 * GF256_RESTRICT &x16, 
                                uint8_t &y, int &bytes)
{
            // Partial product tables; see above
        const GF256_M256 table_lo_y = _mm256_loadu_si256(GF256Ctx.MM256.TABLE_LO_Y + y);
        const GF256_M256 table_hi_y = _mm256_loadu_si256(GF256Ctx.MM256.TABLE_HI_Y + y);

        // clr_mask = 0x0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f
        const GF256_M256 clr_mask = _mm256_set1_epi8(0x0f);

        GF256_M256 * GF256_RESTRICT z32 = reinterpret_cast<GF256_M256 *>(z16);
        const GF256_M256 * GF256_RESTRICT x32 = reinterpret_cast<const GF256_M256 *>(x16);

        // On my Reed Solomon codec, the encoder unit test runs in 640 usec without and 550 usec with the optimization (86% of the original time)
        const unsigned count = bytes / 64;
        for (unsigned i = 0; i < count; ++i)
        {
            // See above comments for details
            GF256_M256 x0 = _mm256_loadu_si256(x32 + i * 2);
            GF256_M256 l0 = _mm256_and_si256(x0, clr_mask);
            x0 = _mm256_srli_epi64(x0, 4);
            const GF256_M256 z0 = _mm256_loadu_si256(z32 + i * 2);
            GF256_M256 h0 = _mm256_and_si256(x0, clr_mask);
            l0 = _mm256_shuffle_epi8(table_lo_y, l0);
            h0 = _mm256_shuffle_epi8(table_hi_y, h0);
            const GF256_M256 p0 = _mm256_xor_si256(l0, h0);
            _mm256_storeu_si256(z32 + i * 2, _mm256_xor_si256(p0, z0));

            GF256_M256 x1 = _mm256_loadu_si256(x32 + i * 2 + 1);
            GF256_M256 l1 = _mm256_and_si256(x1, clr_mask);
            x1 = _mm256_srli_epi64(x1, 4);
            const GF256_M256 z1 = _mm256_loadu_si256(z32 + i * 2 + 1);
            GF256_M256 h1 = _mm256_and_si256(x1, clr_mask);
            l1 = _mm256_shuffle_epi8(table_lo_y, l1);
            h1 = _mm256_shuffle_epi8(table_hi_y, h1);
            const GF256_M256 p1 = _mm256_xor_si256(l1, h1);
            _mm256_storeu_si256(z32 + i * 2 + 1, _mm256_xor_si256(p1, z1));
        }
        bytes -= count * 64;
        z32 += count * 2;
        x32 += count * 2;

        if (bytes >= 32)
        {
            GF256_M256 x0 = _mm256_loadu_si256(x32);
            GF256_M256 l0 = _mm256_and_si256(x0, clr_mask);
            x0 = _mm256_srli_epi64(x0, 4);
            GF256_M256 h0 = _mm256_and_si256(x0, clr_mask);
            l0 = _mm256_shuffle_epi8(table_lo_y, l0);
            h0 = _mm256_shuffle_epi8(table_hi_y, h0);
            const GF256_M256 p0 = _mm256_xor_si256(l0, h0);
            const GF256_M256 z0 = _mm256_loadu_si256(z32);
            _mm256_storeu_si256(z32, _mm256_xor_si256(p0, z0));

            bytes -= 32;
            z32++;
            x32++;
        }

        z16 = reinterpret_cast<GF256_M128 *>(z32);
        x16 = reinterpret_cast<const GF256_M128 *>(x32);
}
