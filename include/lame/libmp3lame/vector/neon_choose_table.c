/*
 *      MP3 Huffman table selection, ARM NEON intrinsics
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
 * \brief The NEON form of the Huffman bit count for the escape tables
 *        (count_bit_esc_neon()).
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "lame_intrin.h"


#ifdef HAVE_NEON_INTRINSICS

#include <arm_neon.h>

/*
 *  The one routine of the x86 vector tier that pays for itself on AArch64.
 *
 *  Three of the seven x86 routines were never written here: clang already
 *  emits the equivalent, including a native 32-bit signed maximum, which is
 *  the only reason the SSE2 sibling of the region maximum exists at all.  Two
 *  more were written, measured and retired - the FFT and the no-escape
 *  counting loop - because both came out slower than the scalar code clang
 *  produces; see @ref vector_dispatch.  What is left is the escape-coded
 *  counting loop, which is worth +3.5 % on a 320 kbit/s encode and +1 % on
 *  VBR.
 *
 *  It builds a table index from a pair of values, x * 16 + y.  The x86
 *  form narrows the region to sixteen bits so that one pmaddwd turns four
 *  pairs into four indices.  Advanced SIMD has no multiply-add-adjacent-pairs
 *  at any width, so the same thing is spelt as a multiply by (xlen, 1) per
 *  lane followed by a pairwise add.  That needs no narrowing, and so none of
 *  the saturation reasoning the x86 file has to carry comes with it.
 *
 *  vpadd_s32 over the halves, rather than vpaddq_s32 or vuzp1q/vuzp2q: those
 *  are AArch64-only spellings, while this one is in the ACLE for both A32 and
 *  A64 - and 32-bit ARM is the whole reason this tier is probed for at run
 *  time rather than assumed.
 *
 *  It reads [ix, end), whose length is even, and touches nothing else; the
 *  remainder is consumed pair by pair exactly as the scalar loop consumes it.
 */

/* Adjacent lanes summed: {a0+a1, a2+a3}.  This is the pmaddwd of the x86
 * file, minus the multiply, which has already happened. */
static int32x2_t
fold_pairs(int32x4_t v)
{
    return vpadd_s32(vget_low_s32(v), vget_high_s32(v));
}


NEON_FUNCTION unsigned int
count_bit_esc_neon(const int *ix, const int *const end,
                   const uint32_t * const largetbl, unsigned int *const nclamped)
{
    /* Written as an array and loaded, not as a braced vector initialiser:
       the latter is a GCC/clang extension and this file has to compile with
       MSVC on ARM64 as well. */
    static const int32_t mul16[4] = { 16, 1, 16, 1 };
    int32x4_t const vmul = vld1q_s32(mul16);
    int32x4_t const v14 = vdupq_n_s32(14);
    int32x4_t const v15 = vdupq_n_s32(15);
    int32x4_t clamps = vdupq_n_s32(0);
    int32x2_t clamps2;
    unsigned int sum = 0;
    unsigned int n;

    while (end - ix >= 8) {
        int32x4_t const a = vld1q_s32(ix);
        int32x4_t const b = vld1q_s32(ix + 4);
        int32x2_t const ia = fold_pairs(vmulq_s32(vminq_s32(a, v15), vmul));
        int32x2_t const ib = fold_pairs(vmulq_s32(vminq_s32(b, v15), vmul));

        /* A comparison yields all-ones where it holds, which read as a signed
           lane is -1, so the counters run down and are read back as the
           negation.  The same trick the SSE2 form uses, for the same reason:
           a subtract instead of a select. */
        clamps = vsubq_s32(clamps, vreinterpretq_s32_u32(vcgtq_s32(a, v14)));
        clamps = vsubq_s32(clamps, vreinterpretq_s32_u32(vcgtq_s32(b, v14)));

        sum += largetbl[vget_lane_s32(ia, 0)];
        sum += largetbl[vget_lane_s32(ia, 1)];
        sum += largetbl[vget_lane_s32(ib, 0)];
        sum += largetbl[vget_lane_s32(ib, 1)];
        ix += 8;
    }
    /* Four lane counters into one.  A region holds far too few values for a
       32-bit lane to come near overflowing. */
    clamps2 = vadd_s32(vget_low_s32(clamps), vget_high_s32(clamps));
    clamps2 = vpadd_s32(clamps2, clamps2);
    n = (unsigned int) vget_lane_s32(clamps2, 0);

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


#endif /* HAVE_NEON_INTRINSICS */
