/*
 *      MP3 Huffman table selection, AVX2 intrinsics functions
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
 * \brief The AVX2 region maximum of the Huffman table search (ix_max_avx2()).
 */

/*
 *  Only the region maximum is here.  The table lookups this file might be
 *  expected to hold as well are faster in their SSE2 form: the gather
 *  instruction has to service sixteen-bit indices from a thirty-two-bit
 *  table, which wastes half of every gather, and it did not repay that in
 *  measurement.  They are therefore left where they are, and this file is
 *  what remains of the wider tier - a maximum over sixteen values at a time.
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

AVX2_FUNCTION int
ix_max_avx2(const int *ix, const int *const end)
{
    __m256i acc = _mm256_setzero_si256();
    __m128i r;
    int     max;

    while (end - ix >= 16) {
        __m256i const a = _mm256_loadu_si256((const __m256i *) ix);
        __m256i const b = _mm256_loadu_si256((const __m256i *) (ix + 8));
        /* the narrowing works within each half, so the sixteen values come
           out interleaved; a maximum is indifferent to their order */
        acc = _mm256_max_epi16(acc, _mm256_packs_epi32(a, b));
        ix += 16;
    }
    r = _mm_max_epi16(_mm256_castsi256_si128(acc), _mm256_extracti128_si256(acc, 1));
    r = _mm_max_epi16(r, _mm_shuffle_epi32(r, _MM_SHUFFLE(1, 0, 3, 2)));
    r = _mm_max_epi16(r, _mm_shuffle_epi32(r, _MM_SHUFFLE(2, 3, 0, 1)));
    r = _mm_max_epi16(r, _mm_shufflelo_epi16(r, _MM_SHUFFLE(2, 3, 0, 1)));
    max = _mm_extract_epi16(r, 0);

    /* Up to fifteen values are left.  They are counted here rather than
       handed to the SSE2 routine, so that no call sits between the wide
       registers and the end of the function. */
    while (ix < end) {
        int const x = *ix++;
        if (max < x)
            max = x;
    }
    return max;
}

#endif /* HAVE_AVX2_INTRINSICS */
