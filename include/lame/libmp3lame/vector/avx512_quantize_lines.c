/*
 *      MP3 quantization of xr^(3/4), AVX-512 intrinsics functions
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
 * \brief The AVX-512 form of the quantization loop
 *        (quantize_lines_xrpow_avx512()).
 */

/*
 *  Sixteen values per pass, and the gather stays as wide as the arithmetic:
 *  the table is thirty-two-bit floats and the indices are thirty-two-bit, so
 *  every lane of it is used.
 *
 *  What this tier adds over the narrower ones is not only the width - it is
 *  that the leftovers need no separate code.  The mask registers make a short
 *  pass a first-class case, so where the other tiers step down through a
 *  narrower block and then a scalar pair, this reaches the end of the run in
 *  the same loop it started.  That matters here more than the width does:
 *  these runs are short, so the drain was a large part of the work rather
 *  than a rounding error at the end of it.
 *
 *  A masked load also reads no memory in its inactive lanes, so running off
 *  the end of the array is not merely harmless, it does not happen.
 *
 *  Every value is computed exactly as the scalar loop computes it - a gather
 *  is a load, and the arithmetic around it is elementwise - so this tier and
 *  the AVX2 one and the SSE2 one and the C one all produce the same output.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "lame_intrin.h"


#ifdef HAVE_AVX512_INTRINSICS

#include <immintrin.h>

AVX512_FUNCTION void
quantize_lines_xrpow_avx512(unsigned int l, FLOAT istep, const FLOAT * xr, int *ix,
                            const FLOAT * const adj)
{
    __m512 const vistep = _mm512_set1_ps(istep);
    /* The caller's accounting is fours and then an optional pair, which comes
       to every value but the last one of an odd run.  Since the loop body is
       elementwise, consuming exactly that many in one sweep computes the same
       values the block-by-block form does, and leaves the same last value of
       an odd run untouched. */
    unsigned int const n = l & ~1u;
    unsigned int i = 0;

    for (; i + 16 <= n; i += 16) {
        __m512 const x = _mm512_mul_ps(_mm512_loadu_ps(xr + i), vistep);
        __m512i const r = _mm512_cvttps_epi32(x);
        __m512 const a = _mm512_i32gather_ps(r, adj, 4);

        _mm512_storeu_si512((void *) (ix + i),
                            _mm512_cvttps_epi32(_mm512_add_ps(x, a)));
    }
    if (i < n) {
        __mmask16 const k = (__mmask16) ((1u << (n - i)) - 1u);
        __m512 const x = _mm512_mul_ps(_mm512_maskz_loadu_ps(k, xr + i), vistep);
        __m512i const r = _mm512_cvttps_epi32(x);
        __m512 const a = _mm512_mask_i32gather_ps(_mm512_setzero_ps(), k, r, adj, 4);

        _mm512_mask_storeu_epi32(ix + i, k,
                                 _mm512_cvttps_epi32(_mm512_add_ps(x, a)));
    }
}

#endif /* HAVE_AVX512_INTRINSICS */
