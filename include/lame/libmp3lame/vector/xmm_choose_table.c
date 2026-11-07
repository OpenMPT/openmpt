/*
 *      MP3 Huffman table selection, intrinsics functions
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
 * \brief The SSE2 forms of the Huffman table search: the region maximum and
 *        the bit counts.
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "lame_intrin.h"



#ifdef HAVE_SSE2_INTRINSICS

#include <emmintrin.h>

/*
 *  All three routines below narrow the region to sixteen bits first.  The
 *  values are quantized magnitudes well under that, and the narrower form is
 *  what makes this worth doing at all: it puts eight of them in a register
 *  instead of four, and it brings within reach two instructions that have no
 *  32-bit counterpart before SSE4.1 - a signed maximum, and a multiply-add
 *  that turns a pair (x, y) into the table index x * xlen + y on its own.
 *
 *  The narrowing saturates at 32767.  Only the maximum can meet a value that
 *  large, and only the maximum has to say what it does about it.
 */

/* Sixteen bit lanes hold what the loop needs to distinguish; see the note in
 * lame_intrin.h for why the saturation is harmless to the one caller. */
SSE_FUNCTION int
ix_max_sse2(const int *ix, const int *const end)
{
    __m128i acc = _mm_setzero_si128();
    int     max;

    while (end - ix >= 8) {
        __m128i const a = _mm_loadu_si128((const __m128i *) ix);
        __m128i const b = _mm_loadu_si128((const __m128i *) (ix + 4));
        acc = _mm_max_epi16(acc, _mm_packs_epi32(a, b));
        ix += 8;
    }
    acc = _mm_max_epi16(acc, _mm_shuffle_epi32(acc, _MM_SHUFFLE(1, 0, 3, 2)));
    acc = _mm_max_epi16(acc, _mm_shuffle_epi32(acc, _MM_SHUFFLE(2, 3, 0, 1)));
    acc = _mm_max_epi16(acc, _mm_shufflelo_epi16(acc, _MM_SHUFFLE(2, 3, 0, 1)));
    max = _mm_extract_epi16(acc, 0);

    while (ix < end) {
        int const x = *ix++;
        if (max < x)
            max = x;
    }
    return max;
}


SSE_FUNCTION unsigned int
count_bit_esc_sse2(const int *ix, const int *const end,
                   const uint32_t * const largetbl, unsigned int *const nclamped)
{
    __m128i const v14 = _mm_set1_epi16(14);
    __m128i const v15 = _mm_set1_epi16(15);
    /* (x, y) -> x * 16 + y, over four pairs at once */
    __m128i const vmul = _mm_set_epi16(1, 16, 1, 16, 1, 16, 1, 16);
    __m128i clamps = _mm_setzero_si128();
    unsigned int sum = 0;
    unsigned int n;

    while (end - ix >= 8) {
        __m128i const a = _mm_loadu_si128((const __m128i *) ix);
        __m128i const b = _mm_loadu_si128((const __m128i *) (ix + 4));
        __m128i const p = _mm_packs_epi32(a, b);
        int     idx[4];

        /* a comparison yields -1 where it holds, so the counters run down */
        clamps = _mm_sub_epi16(clamps, _mm_cmpgt_epi16(p, v14));
        _mm_storeu_si128((__m128i *) idx, _mm_madd_epi16(_mm_min_epi16(p, v15), vmul));
        sum += largetbl[idx[0]] + largetbl[idx[1]] + largetbl[idx[2]] + largetbl[idx[3]];
        ix += 8;
    }
    /* eight lane counters into one; a region holds far too few values for a
       lane to overflow sixteen bits */
    clamps = _mm_madd_epi16(clamps, _mm_set1_epi16(1));
    clamps = _mm_add_epi32(clamps, _mm_shuffle_epi32(clamps, _MM_SHUFFLE(1, 0, 3, 2)));
    clamps = _mm_add_epi32(clamps, _mm_shuffle_epi32(clamps, _MM_SHUFFLE(2, 3, 0, 1)));
    n = (unsigned int) _mm_cvtsi128_si32(clamps);

    while (ix < end) {
        unsigned int x = *ix++;
        unsigned int y = *ix++;

        if (x >= 15u) {
            x = 15u;
            ++n;
        }
        if (y >= 15u) {
            y = 15u;
            ++n;
        }
        sum += largetbl[(x << 4u) + y];
    }
    *nclamped = n;
    return sum;
}


SSE_FUNCTION void
count_bit_noESC_from3_sse2(const int *ix, const int *const end, int const xlen,
                           const uint8_t * const hlen1, const uint8_t * const hlen2,
                           const uint8_t * const hlen3, unsigned int sums[3])
{
    /* (x, y) -> x * xlen + y */
    __m128i const vmul = _mm_set_epi16(1, (short) xlen, 1, (short) xlen,
                                       1, (short) xlen, 1, (short) xlen);
    unsigned int sum1 = 0, sum2 = 0, sum3 = 0;

    while (end - ix >= 8) {
        __m128i const a = _mm_loadu_si128((const __m128i *) ix);
        __m128i const b = _mm_loadu_si128((const __m128i *) (ix + 4));
        int     idx[4];

        _mm_storeu_si128((__m128i *) idx, _mm_madd_epi16(_mm_packs_epi32(a, b), vmul));
        sum1 += hlen1[idx[0]] + hlen1[idx[1]] + hlen1[idx[2]] + hlen1[idx[3]];
        sum2 += hlen2[idx[0]] + hlen2[idx[1]] + hlen2[idx[2]] + hlen2[idx[3]];
        sum3 += hlen3[idx[0]] + hlen3[idx[1]] + hlen3[idx[2]] + hlen3[idx[3]];
        ix += 8;
    }
    while (ix < end) {
        unsigned int const x0 = *ix++;
        unsigned int const x1 = *ix++;
        unsigned int const x = x0 * (unsigned int) xlen + x1;

        sum1 += hlen1[x];
        sum2 += hlen2[x];
        sum3 += hlen3[x];
    }
    sums[0] = sum1;
    sums[1] = sum2;
    sums[2] = sum3;
}

#endif /* HAVE_SSE2_INTRINSICS */
