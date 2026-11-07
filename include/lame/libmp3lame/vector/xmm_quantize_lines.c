/*
 *      MP3 quantization of xr^(3/4), intrinsics functions
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
 * \brief The SSE2 form of the quantization loop (quantize_lines_xrpow_sse2()).
 */

/*
 *  This loop is elementwise - a multiply, a truncation, a table lookup, an
 *  add and a second truncation, with no accumulator anywhere - so the vector
 *  form computes each value exactly as the scalar one does and the output
 *  cannot move.  That is the whole reason it is written out here rather than
 *  left to the compiler.
 *
 *  Left to the compiler is in fact what happens on some toolchains: gcc and
 *  clang already emit this sequence, down to reassembling the four looked-up
 *  values with unpcklps.  MSVC does not, and leaves the loop essentially
 *  scalar.  Writing it once means every toolchain runs the same code, which
 *  is the same rule this library already applies to CPU dispatch.
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

#include "xmm_quant_kernel.h"

SSE_FUNCTION void
quantize_lines_xrpow_sse2(unsigned int l, FLOAT istep, const FLOAT * xr, int *ix,
                          const FLOAT * const adj)
{
    __m128 const vistep = _mm_set1_ps(istep);
    unsigned int remaining;

    /* The caller's element accounting, reproduced exactly: four at a time,
       then an optional pair - and an odd l therefore leaves its last value
       untouched, as it always has. */
    l = l >> 1;
    remaining = l % 2;
    l = l >> 1;

    while (l--) {
        _mm_storeu_si128((__m128i *) ix, quant4_sse2(_mm_mul_ps(_mm_loadu_ps(xr), vistep), adj));
        xr += 4;
        ix += 4;
    }
    if (remaining) {
        quant_pair_c(istep, xr, ix, adj);
    }
}

#endif /* HAVE_SSE2_INTRINSICS */
