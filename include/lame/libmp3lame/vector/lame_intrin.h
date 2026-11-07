/*
 *      lame_intrin.h include file
 *
 *      Copyright (c) 2006 Gabriel Bouvigne
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
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
 * \brief Declarations of the vector routines and their compiler attributes.
 */


#ifndef LAME_INTRIN_H
#define LAME_INTRIN_H

#include "version.h"

/* These routines are compiled for instruction sets the rest of the library is
 * not, and where the compiler can express that per function it is said here
 * rather than given to a whole file.  Realigning the stack is part of the
 * same bargain: a caller compiled without these instructions is under no
 * obligation to align it the way they need.
 */
#if defined (__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 2)))
#define REALIGN __attribute__((force_align_arg_pointer))
#define TARGET(x) __attribute__((target(x)))
#else
#define REALIGN
#define TARGET(x)
#endif

#define SSE_FUNCTION  REALIGN TARGET("sse2")
#define AVX2_FUNCTION REALIGN TARGET("avx2")
#define AVX512_FUNCTION REALIGN TARGET("avx512f,avx512vl,avx512bw,avx512dq")

/* The ARM routine takes neither.  Advanced SIMD is the baseline on every
 * AArch64 target, so there is nothing to opt into per function; and where it
 * is optional - 32-bit ARM - the opt-in has to be a whole-file one anyway,
 * which configure supplies as VECTOR_NEON_FLAGS.  Stack realignment is an x86
 * concern and the attribute does not exist here.
 */
#define NEON_FUNCTION


void
init_xrpow_core_sse(gr_info * const cod_info, FLOAT xrpow[SAMPLES_PER_GRANULE], int upper, FLOAT * sum);

void
fht_SSE2(FLOAT* , int);


/*
 *  The Huffman table search.
 *
 *  Each of these takes a whole region and handles its own remainder, so a
 *  caller substitutes one call for one loop.  They read [ix, end), whose
 *  length is even, and they touch nothing else.
 */

/* The largest value in the region - exactly, as long as that is at most
 * IXMAX_VAL, and some larger number otherwise.  The vector form compares
 * sixteen bits at a time and so cannot tell 40000 from 32767, which is the
 * whole reason it is quick; the only caller asks "<= 15", "> IXMAX_VAL" and
 * a linbits bucket, and 32767 answers all three the way the true value
 * would.  A wider caller would need a different routine, hence the assertion
 * on IXMAX_VAL where these are used.
 */
int
ix_max_sse2(const int *ix, const int *end);

int
ix_max_avx2(const int *ix, const int *end);

/* Sums largetbl[] over the region's pairs and reports how many values had to
 * be clamped to 15.  The linbits each clamp costs are the caller's to add:
 * it is the caller that knows the two candidate tables.
 */
unsigned int
count_bit_esc_sse2(const int *ix, const int *end, const uint32_t *largetbl,
                   unsigned int *nclamped);

/* The ARM form of the same counter, under the same contract.  It is the only
 * ARM routine here: every other one in this header was either already being
 * vectorised by the compiler on AArch64, or was written and measured slower
 * than the scalar code it would have replaced.  Which one is which was
 * decided by measurement - see @ref vector_dispatch before adding a second.
 */
unsigned int
count_bit_esc_neon(const int *ix, const int *end, const uint32_t *largetbl,
                   unsigned int *nclamped);

/* The three code lengths of one region under three consecutive tables, which
 * share an index and are therefore worth computing together.  Every value
 * must be in [0, xlen), as it is whenever the region's maximum picked these
 * tables.
 */
void
count_bit_noESC_from3_sse2(const int *ix, const int *end, int xlen,
                           const uint8_t *hlen1, const uint8_t *hlen2,
                           const uint8_t *hlen3, unsigned int sums[3]);


/*
 *  Quantization of xr^(3/4).
 *
 *  Elementwise throughout - multiply, truncate, look up, add, truncate - so
 *  these compute value for value what the C loop computes, and substituting
 *  one cannot move the output.
 *
 *  Two things a caller owes them.  First, every xr[i] * istep must land in
 *  [0, PRECALC_SIZE) once truncated, because that is the subscript handed to
 *  adj[]; the truncating convert answers out-of-range input with
 *  INT_MIN where C leaves it undefined, so the two forms agree only inside
 *  the range.  count_bits() establishes this by rejecting a granule whose
 *  xrpow_max exceeds IXMAX_VAL/istep before it ever gets here.  Second, l is
 *  consumed exactly as the C loop consumes it: fours, then an optional pair,
 *  so an odd l leaves its last value untouched.
 */
void
quantize_lines_xrpow_sse2(unsigned int l, FLOAT istep, const FLOAT *xr, int *ix,
                          const FLOAT *adj);

void
quantize_lines_xrpow_avx2(unsigned int l, FLOAT istep, const FLOAT *xr, int *ix,
                          const FLOAT *adj);

void
quantize_lines_xrpow_avx512(unsigned int l, FLOAT istep, const FLOAT *xr, int *ix,
                            const FLOAT *adj);


/*
 *  VBR scalefactor-band noise: quantize a band and return the summed squared
 *  quantization error.
 *
 *  The quantization is the same elementwise kernel as above, so the same index
 *  contract holds: every xr34[i]*sfpow34, and every value it becomes after the
 *  adj[] step, must truncate into [0, PRECALC_SIZE) - which the VBR search
 *  guarantees by only ever asking for scalefactors with sfpow34*xr34 <=
 *  IXMAX_VAL.  The sum is a reduction, and this computes it with the
 *  (x0^2+x2^2)+(x1^2+x3^2) association - the one gcc and clang already emit -
 *  accumulated a four-block at a time; a build whose compiler uses another
 *  association gets a different last bit, which is a quality question and not a
 *  bitstream-identity one.  bw is the band width in values; adj is adj43[] and
 *  pw43 is pow43[].
 *
 *  SSE2 only, and it is the widest form this routine carries on any
 *  architecture: neither an AVX2 nor an AVX-512 tier repaid itself here, both
 *  having been written and measured - see @ref vector_dispatch for the numbers
 *  and for why the width buys so little on this shape.
 */
FLOAT
calc_sfb_noise_x34_sse2(const FLOAT *xr, const FLOAT *xr34, unsigned int bw,
                        FLOAT sfpow, FLOAT sfpow34, const FLOAT *adj,
                        const FLOAT *pw43);


#endif
