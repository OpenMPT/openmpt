/*
 *      The quantization steps that the SSE2 and AVX2 forms share
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
 * \brief The quantization of one block of four values, and of one trailing
 *        pair, for the vector forms of quantize_lines_xrpow() and
 *        calc_sfb_noise_x34().
 *
 * Each function here is compiled into the file that includes it, with the
 * target of that file.
 */

#ifndef LAME_XMM_QUANT_KERNEL_H
#define LAME_XMM_QUANT_KERNEL_H

#include <emmintrin.h>

#include "machine.h"
#include "lame_intrin.h"

/**
 * \internal
 * \brief Quantizes one pair of values in scalar code: multiply, truncate,
 *        look up the rounding offset, add, truncate.
 * \param istep  the inverse quantizer step.
 * \param xr     the two values.
 * \param ix     receives the two indices.
 * \param adj    the rounding offset of each index.
 */
static inline void
quant_pair_c(FLOAT istep, const FLOAT * xr, int *ix, const FLOAT * const adj)
{
    FLOAT   x0, x1;
    int     rx0, rx1;

    x0 = xr[0] * istep;
    x1 = xr[1] * istep;
    rx0 = (int) x0;
    rx1 = (int) x1;
    x0 += adj[rx0];
    x1 += adj[rx1];
    ix[0] = (int) x0;
    ix[1] = (int) x1;
}

/**
 * \internal
 * \brief Quantizes four scaled values: truncate, look up the rounding offset
 *        of each, add, truncate.
 *
 * The indices have to reach general registers to subscript the table. Each
 * lane is shuffled down and converted on its own, which looks like more work
 * than converting all four at once and reading them back from memory. But
 * four narrow loads cannot all forward from one wide store, and that stall
 * costs more than the shuffles do. Measured: the memory form was slower than
 * the scalar loop it replaced. It is also the sequence gcc and clang emit
 * unaided.
 *
 * \param x    the four values, already multiplied by the inverse step.
 * \param adj  the rounding offset of each index.
 * \return the four indices.
 */
static inline SSE_FUNCTION __m128i
quant4_sse2(__m128 x, const FLOAT * const adj)
{
    int const r0 = _mm_cvttss_si32(x);
    int const r1 = _mm_cvttss_si32(_mm_shuffle_ps(x, x, _MM_SHUFFLE(1, 1, 1, 1)));
    int const r2 = _mm_cvttss_si32(_mm_unpackhi_ps(x, x));
    int const r3 = _mm_cvttss_si32(_mm_shuffle_ps(x, x, _MM_SHUFFLE(3, 3, 3, 3)));

    return _mm_cvttps_epi32(_mm_add_ps(x, _mm_set_ps(adj[r3], adj[r2], adj[r1], adj[r0])));
}

#endif /* LAME_XMM_QUANT_KERNEL_H */
