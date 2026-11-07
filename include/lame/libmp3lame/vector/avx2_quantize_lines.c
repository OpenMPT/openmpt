/*
 *      MP3 quantization of xr^(3/4), AVX2 intrinsics functions
 *
 *      Copyright (c) 2026 The LAME project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * \file
 * \internal
 * \brief The AVX2 form of the quantization loop (quantize_lines_xrpow_avx2()).
 */

/*
 *  Unlike the Huffman table lookups, this one has a gather worth having: the
 *  table is thirty-two-bit floats and the indices are thirty-two-bit, so no
 *  lane is wasted, and the instruction replaces four round trips through
 *  general registers and the shuffles that reassembled them.
 *
 *  Every value is computed exactly as the scalar loop computes it - a gather
 *  is a load, and the arithmetic around it is elementwise - so this tier and
 *  the SSE2 one and the C one all produce the same output.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "lame_intrin.h"


#ifdef HAVE_AVX2_INTRINSICS

#include <immintrin.h>

#include "xmm_quant_kernel.h"

AVX2_FUNCTION void
quantize_lines_xrpow_avx2(unsigned int l, FLOAT istep, const FLOAT * xr, int *ix,
                          const FLOAT * const adj)
{
    __m256 const vistep = _mm256_set1_ps(istep);
    __m128 const histep = _mm_set1_ps(istep);
    unsigned int remaining;

    /* Same accounting as the scalar loop: blocks of four, then an optional
       pair.  This tier takes two blocks at a time and hands the odd block
       back to the narrower form. */
    l = l >> 1;
    remaining = l % 2;
    l = l >> 1;

    while (l >= 2) {
        __m256 const x = _mm256_mul_ps(_mm256_loadu_ps(xr), vistep);
        __m256 const a = _mm256_i32gather_ps(adj, _mm256_cvttps_epi32(x), 4);

        _mm256_storeu_si256((__m256i *) ix, _mm256_cvttps_epi32(_mm256_add_ps(x, a)));
        xr += 8;
        ix += 8;
        l -= 2;
    }
    if (l) {
        __m128 const x = _mm_mul_ps(_mm_loadu_ps(xr), histep);
        __m128 const a = _mm_i32gather_ps(adj, _mm_cvttps_epi32(x), 4);

        _mm_storeu_si128((__m128i *) ix, _mm_cvttps_epi32(_mm_add_ps(x, a)));
        xr += 4;
        ix += 4;
    }
    if (remaining) {
        quant_pair_c(istep, xr, ix, adj);
    }
}

#endif /* HAVE_AVX2_INTRINSICS */
