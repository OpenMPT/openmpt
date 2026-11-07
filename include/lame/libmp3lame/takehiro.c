/*
 *	MP3 huffman table selecting and bit counting
 *
 *	Copyright (c) 1999-2005 Takehiro TOMINAGA
 *	Copyright (c) 2002-2005 Gabriel Bouvigne
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "quantize_pvt.h"
#include "tables.h"

#if defined( HAVE_SSE2_INTRINSICS ) || defined( HAVE_NEON_INTRINSICS )
#include "vector/lame_intrin.h"

/* Regions shorter than this are quicker scalar: the vector routines take
 * eight values per pass, and their setup and final reduction do not pay for
 * themselves below roughly four passes.  Measured over a real encode, a
 * third of the regions are shorter than this, but they hold only about five
 * percent of the values - so this test is here to avoid a regression on the
 * short ones, not to win anything on them.
 *
 * Shared by both architectures deliberately: the pass width is eight values
 * on either, the ARM routine having been written to the same shape for that
 * reason.
 */
#define TABLE_SEARCH_VECTOR_MIN 32
#endif

#if defined( HAVE_SSE2_INTRINSICS )
/** @internal
 * The x86 vector routines compare 16 bits at a time, so a region maximum is
 * exact only while it fits into 16 bits.  This is enough, because the table
 * search rejects any value above IXMAX_VAL, and every value above the
 * saturation point is above IXMAX_VAL too.  This stops being true if
 * IXMAX_VAL is ever raised above that point.  See @ref vector_dispatch for
 * why 16-bit SSE2 is faster than the 32-bit SSE4.1 path here.
 *
 * The ARM routine uses 32-bit lanes and never narrows, so this assertion is
 * only about the x86 tier.
 */
enum { static_assert_ixmax_val_fits_in_16_bits = 1 / (IXMAX_VAL < 32767 ? 1 : 0) };
#endif


static const struct {
    const int region0_count;
    const int region1_count;
} subdv_table[23] = {
    {
    0, 0},              /* 0 bands */
    {
    0, 0},              /* 1 bands */
    {
    0, 0},              /* 2 bands */
    {
    0, 0},              /* 3 bands */
    {
    0, 0},              /* 4 bands */
    {
    0, 1},              /* 5 bands */
    {
    1, 1},              /* 6 bands */
    {
    1, 1},              /* 7 bands */
    {
    1, 2},              /* 8 bands */
    {
    2, 2},              /* 9 bands */
    {
    2, 3},              /* 10 bands */
    {
    2, 3},              /* 11 bands */
    {
    3, 4},              /* 12 bands */
    {
    3, 4},              /* 13 bands */
    {
    3, 4},              /* 14 bands */
    {
    4, 5},              /* 15 bands */
    {
    4, 5},              /* 16 bands */
    {
    4, 6},              /* 17 bands */
    {
    5, 6},              /* 18 bands */
    {
    5, 6},              /* 19 bands */
    {
    5, 7},              /* 20 bands */
    {
    6, 7},              /* 21 bands */
    {
    6, 7},              /* 22 bands */
};





/*********************************************************************
 * nonlinear quantization of xr 
 * More accurate formula than the ISO formula.  Takes into account
 * the fact that we are quantizing xr -> ix, but we want ix^4/3 to be 
 * as close as possible to x^4/3.  (taking the nearest int would mean
 * ix is as close as possible to xr, which is different.)
 *
 * From Segher Boessenkool <segher@eastsite.nl>  11/1999
 *
 * 09/2000: ASM code removed in favor of IEEE754 hack by Takehiro
 * Tominaga. If you need the ASM code, check CVS circa Aug 2000.
 *
 * 01/2004: Optimizations by Gabriel Bouvigne
 *********************************************************************/





static void
quantize_lines_xrpow_01(unsigned int l, FLOAT istep, const FLOAT * xr, int *ix)
{
    const FLOAT compareval0 = (1.0f - 0.4054f) / istep;
    unsigned int i;

    assert(l > 0);
    assert(l % 2 == 0);
    for (i = 0; i < l; i += 2) {
        FLOAT const xr_0 = xr[i+0];
        FLOAT const xr_1 = xr[i+1];
        int const ix_0 = (compareval0 > xr_0) ? 0 : 1;
        int const ix_1 = (compareval0 > xr_1) ? 0 : 1;
        ix[i+0] = ix_0;
        ix[i+1] = ix_1;
    }
}



#if TAKEHIRO_IEEE754_HACK

typedef union {
    float   f;
    int     i;
} fi_union;

#define MAGIC_FLOAT (65536*(128))
#define MAGIC_INT 0x4b000000


static void
quantize_lines_xrpow(unsigned int l, FLOAT istep, const FLOAT * xp, int *pi)
{
    fi_union *fi;
    unsigned int remaining;

    assert(l > 0);

    fi = (fi_union *) pi;

    l = l >> 1;
    remaining = l % 2;
    l = l >> 1;
    while (l--) {
        double  x0 = istep * xp[0];
        double  x1 = istep * xp[1];
        double  x2 = istep * xp[2];
        double  x3 = istep * xp[3];

        x0 += MAGIC_FLOAT;
        fi[0].f = x0;
        x1 += MAGIC_FLOAT;
        fi[1].f = x1;
        x2 += MAGIC_FLOAT;
        fi[2].f = x2;
        x3 += MAGIC_FLOAT;
        fi[3].f = x3;

        fi[0].f = x0 + adj43asm[fi[0].i - MAGIC_INT];
        fi[1].f = x1 + adj43asm[fi[1].i - MAGIC_INT];
        fi[2].f = x2 + adj43asm[fi[2].i - MAGIC_INT];
        fi[3].f = x3 + adj43asm[fi[3].i - MAGIC_INT];

        fi[0].i -= MAGIC_INT;
        fi[1].i -= MAGIC_INT;
        fi[2].i -= MAGIC_INT;
        fi[3].i -= MAGIC_INT;
        fi += 4;
        xp += 4;
    };
    if (remaining) {
        double  x0 = istep * xp[0];
        double  x1 = istep * xp[1];

        x0 += MAGIC_FLOAT;
        fi[0].f = x0;
        x1 += MAGIC_FLOAT;
        fi[1].f = x1;

        fi[0].f = x0 + adj43asm[fi[0].i - MAGIC_INT];
        fi[1].f = x1 + adj43asm[fi[1].i - MAGIC_INT];

        fi[0].i -= MAGIC_INT;
        fi[1].i -= MAGIC_INT;
    }

}


#else

/*********************************************************************
 * XRPOW_FTOI is a macro to convert floats to ints.  
 * if XRPOW_FTOI(x) = nearest_int(x), then QUANTFAC(x)=adj43asm[x]
 *                                         rounding offset -0.0946
 *
 * if XRPOW_FTOI(x) = floor(x), then QUANTFAC(x)=asj43[x]   
 *                                   rounding offset 0.4054
 *
 * Note: using floor() or (int) is extremely slow. On machines where
 * the TAKEHIRO_IEEE754_HACK code above does not work, it is worthwile
 * to write some ASM for XRPOW_FTOI().  
 *********************************************************************/
#define XRPOW_FTOI(src,dest) ((dest) = (int)(src))
#define QUANTFAC(rx)  adj43[rx]


static void
quantize_lines_xrpow(unsigned int l, FLOAT istep, const FLOAT * xr, int *ix)
{
    unsigned int remaining;

    assert(l > 0);

    l = l >> 1;
    remaining = l % 2;
    l = l >> 1;
    while (l--) {
        FLOAT   x0, x1, x2, x3;
        int     rx0, rx1, rx2, rx3;

        x0 = *xr++ * istep;
        x1 = *xr++ * istep;
        XRPOW_FTOI(x0, rx0);
        x2 = *xr++ * istep;
        XRPOW_FTOI(x1, rx1);
        x3 = *xr++ * istep;
        XRPOW_FTOI(x2, rx2);
        x0 += QUANTFAC(rx0);
        XRPOW_FTOI(x3, rx3);
        x1 += QUANTFAC(rx1);
        XRPOW_FTOI(x0, *ix++);
        x2 += QUANTFAC(rx2);
        XRPOW_FTOI(x1, *ix++);
        x3 += QUANTFAC(rx3);
        XRPOW_FTOI(x2, *ix++);
        XRPOW_FTOI(x3, *ix++);
    };
    if (remaining) {
        FLOAT   x0, x1;
        int     rx0, rx1;

        x0 = *xr++ * istep;
        x1 = *xr++ * istep;
        XRPOW_FTOI(x0, rx0);
        XRPOW_FTOI(x1, rx1);
        x0 += QUANTFAC(rx0);
        x1 += QUANTFAC(rx1);
        XRPOW_FTOI(x0, *ix++);
        XRPOW_FTOI(x1, *ix++);
    }

}



#endif


/* Runs shorter than this are quicker scalar.  Unlike the table search this
 * loop has almost no fixed cost to amortise - one broadcast of istep - so the
 * threshold sits at the width of a single pass rather than at several.  The
 * wider tier pays for entering and leaving the wide registers and so asks for
 * more.  Measured over a real CBR encode, runs below eight values are a fifth
 * of the calls but under one percent of the values, and below sixteen a third
 * of the calls and under two percent: these tests are here to avoid a
 * regression on short runs, not to win anything on them.
 */
#define QUANTIZE_LINES_VECTOR_MIN      8
#define QUANTIZE_LINES_VECTOR_MIN_AVX2 16
#define QUANTIZE_LINES_VECTOR_MIN_AVX512 32

/* The vector forms compute the default formulation - truncation toward zero
 * against adj43[].  TAKEHIRO_IEEE754_HACK is a different formulation with its
 * own rounding and its own table, reaching the same answer by another route;
 * it is off by default and may be retired, so it is left entirely alone and a
 * hack build keeps its own scalar path.
 */
static void
quantize_lines_xrpow_v(unsigned int l, FLOAT istep, const FLOAT * xr, int *ix,
                       LAME_UNUSED vector_impl_t impl)
{
#if !TAKEHIRO_IEEE754_HACK
#if defined( HAVE_AVX512_INTRINSICS )
    if (impl >= VECTOR_IMPL_AVX512 && l >= QUANTIZE_LINES_VECTOR_MIN_AVX512) {
        quantize_lines_xrpow_avx512(l, istep, xr, ix, adj43);
        return;
    }
#endif
#if defined( HAVE_AVX2_INTRINSICS )
    if (impl >= VECTOR_IMPL_AVX2 && l >= QUANTIZE_LINES_VECTOR_MIN_AVX2) {
        quantize_lines_xrpow_avx2(l, istep, xr, ix, adj43);
        return;
    }
#endif
#if defined( HAVE_SSE2_INTRINSICS )
    if (impl >= VECTOR_IMPL_SSE2 && l >= QUANTIZE_LINES_VECTOR_MIN) {
        quantize_lines_xrpow_sse2(l, istep, xr, ix, adj43);
        return;
    }
#endif
#endif
    quantize_lines_xrpow(l, istep, xr, ix);
}


/*********************************************************************
 * Quantization function
 * This function will select which lines to quantize and call the
 * proper quantization function
 *********************************************************************/

static void
quantize_xrpow(const FLOAT * xp, int *pi, FLOAT istep, gr_info const *const cod_info,
               calc_noise_data const *prev_noise, vector_impl_t impl)
{
    /* quantize on xr^(3/4) instead of xr */
    int     sfb;
    int     sfbmax;
    int     j = 0;
    int     prev_data_use;
    int    *iData;
    int     accumulate = 0;
    int     accumulate01 = 0;
    int    *acc_iData;
    const FLOAT *acc_xp;

    iData = pi;
    acc_xp = xp;
    acc_iData = iData;


    /* Reusing previously computed data does not seems to work if global gain
       is changed. Finding why it behaves this way would allow to use a cache of 
       previously computed values (let's 10 cached values per sfb) that would 
       probably provide a noticeable speedup */
    prev_data_use = (prev_noise && (cod_info->quant.global_gain == prev_noise->global_gain));

    if (cod_info->granule.block_type == SHORT_TYPE)
        sfbmax = SFBMAX - 1;
    else
        sfbmax = SBMAX_l - 1;

    for (sfb = 0; sfb <= sfbmax; sfb++) {
        int     step = -1;

        if (prev_data_use || cod_info->granule.block_type == NORM_TYPE) {
            step = sfb_quant_step(cod_info, sfb);
        }
        assert(cod_info->granule.width[sfb] >= 0);
        if (prev_data_use && (prev_noise->step[sfb] == step)) {
            /* do not recompute this part,
               but compute accumulated lines */
            if (accumulate) {
                quantize_lines_xrpow_v(accumulate, istep, acc_xp, acc_iData, impl);
                accumulate = 0;
            }
            if (accumulate01) {
                quantize_lines_xrpow_01(accumulate01, istep, acc_xp, acc_iData);
                accumulate01 = 0;
            }
        }
        else {          /*should compute this part */
            int     l;
            l = cod_info->granule.width[sfb];

            if ((j + cod_info->granule.width[sfb]) > cod_info->granule.max_nonzero_coeff) {
                /*do not compute upper zero part */
                int     usefullsize;
                usefullsize = cod_info->granule.max_nonzero_coeff - j + 1;
                memset(&pi[cod_info->granule.max_nonzero_coeff], 0,
                       sizeof(int) * (SAMPLES_PER_GRANULE - cod_info->granule.max_nonzero_coeff));
                l = usefullsize;

                if (l < 0) {
                    l = 0;
                }

                /* no need to compute higher sfb values */
                sfb = sfbmax + 1;
            }

            /*accumulate lines to quantize */
            if (!accumulate && !accumulate01) {
                acc_iData = iData;
                acc_xp = xp;
            }
            /* the "upper zero part" shortcut above puts sfb past sfbmax, and
               the previous-noise cache has no entry for that index */
            if (prev_noise && sfb <= sfbmax &&
                prev_noise->sfb_count1 > 0 &&
                sfb >= prev_noise->sfb_count1 &&
                prev_noise->step[sfb] > 0 && step >= prev_noise->step[sfb]) {

                if (accumulate) {
                    quantize_lines_xrpow_v(accumulate, istep, acc_xp, acc_iData, impl);
                    accumulate = 0;
                    acc_iData = iData;
                    acc_xp = xp;
                }
                accumulate01 += l;
            }
            else {
                if (accumulate01) {
                    quantize_lines_xrpow_01(accumulate01, istep, acc_xp, acc_iData);
                    accumulate01 = 0;
                    acc_iData = iData;
                    acc_xp = xp;
                }
                accumulate += l;
            }

            if (l <= 0) {
                /*  rh: 20040215
                 *  may happen due to "prev_data_use" optimization 
                 */
                if (accumulate01) {
                    quantize_lines_xrpow_01(accumulate01, istep, acc_xp, acc_iData);
                    accumulate01 = 0;
                }
                if (accumulate) {
                    quantize_lines_xrpow_v(accumulate, istep, acc_xp, acc_iData, impl);
                    accumulate = 0;
                }

                break;  /* ends for-loop */
            }
        }
        if (sfb <= sfbmax) {
            iData += cod_info->granule.width[sfb];
            xp += cod_info->granule.width[sfb];
            j += cod_info->granule.width[sfb];
        }
    }
    if (accumulate) {   /*last data part */
        quantize_lines_xrpow_v(accumulate, istep, acc_xp, acc_iData, impl);
        accumulate = 0;
    }
    if (accumulate01) { /*last data part */
        quantize_lines_xrpow_01(accumulate01, istep, acc_xp, acc_iData);
        accumulate01 = 0;
    }

}




/*************************************************************************/
/*	      ix_max							 */
/*************************************************************************/

static int
ix_max(const int *ix, const int *end)
{
    int     max1 = 0, max2 = 0;

    do {
        int const x1 = *ix++;
        int const x2 = *ix++;
        if (max1 < x1)
            max1 = x1;

        if (max2 < x2)
            max2 = x2;
    } while (ix < end);
    if (max1 < max2)
        max1 = max2;
    return max1;
}


/* The tiers are tried highest first, and each is only offered a region long
 * enough to repay it.  Everything below falls through to the scalar loop,
 * which is also what an architecture without any of this gets.
 */
static int
ix_max_v(const int *ix, const int *const end, LAME_UNUSED vector_impl_t impl)
{
#if defined( HAVE_SSE2_INTRINSICS )
    if (end - ix >= TABLE_SEARCH_VECTOR_MIN) {
# if defined( HAVE_AVX2_INTRINSICS )
        if (impl >= VECTOR_IMPL_AVX2)
            return ix_max_avx2(ix, end);
# endif
        if (impl >= VECTOR_IMPL_SSE2)
            return ix_max_sse2(ix, end);
    }
#endif
    return ix_max(ix, end);
}








static int
count_bit_ESC(const int *ix, const int *const end, int t1, const int t2,
              unsigned int *const s, LAME_UNUSED vector_impl_t impl)
{
    /* ESC-table is used */
    unsigned int const linbits = ht[t1].xlen * 65536u + ht[t2].xlen;
    unsigned int sum = 0, sum2;

#if defined( HAVE_SSE2_INTRINSICS )
    if (impl >= VECTOR_IMPL_SSE2 && end - ix >= TABLE_SEARCH_VECTOR_MIN) {
        unsigned int nclamped;

        sum = count_bit_esc_sse2(ix, end, largetbl, &nclamped);
        sum += nclamped * linbits;
    }
    else
#elif defined( HAVE_NEON_INTRINSICS )
    if (impl >= VECTOR_IMPL_NEON && end - ix >= TABLE_SEARCH_VECTOR_MIN) {
        unsigned int nclamped;

        sum = count_bit_esc_neon(ix, end, largetbl, &nclamped);
        sum += nclamped * linbits;
    }
    else
#endif
    do {
        unsigned int x = *ix++;
        unsigned int y = *ix++;

        if (x >= 15u) {
            x = 15u;
            sum += linbits;
        }
        if (y >= 15u) {
            y = 15u;
            sum += linbits;
        }
        x <<= 4u;
        x += y;
        sum += largetbl[x];
    } while (ix < end);

    sum2 = sum & 0xffffu;
    sum >>= 16u;

    if (sum > sum2) {
        sum = sum2;
        t1 = t2;
    }

    *s += sum;
    return t1;
}


static int
count_bit_noESC(const int *ix, const int *end, LAME_UNUSED int mx, unsigned int *s,
                LAME_UNUSED vector_impl_t impl)
{
    /* No ESC-words */
    unsigned int sum1 = 0;
    const uint8_t *const hlen1 = ht[1].hlen;

    do {
        unsigned int const x0 = *ix++;
        unsigned int const x1 = *ix++;
        sum1 += hlen1[ x0+x0 + x1 ];
    } while (ix < end);

    *s += sum1;
    return 1;
}


static const int huf_tbl_noESC[] = {
    1, 2, 5, 7, 7, 10, 10, 13, 13, 13, 13, 13, 13, 13, 13
};


static int
count_bit_noESC_from2(const int *ix, const int *end, int max, unsigned int *s,
                      LAME_UNUSED vector_impl_t impl)
{
    int t1 = huf_tbl_noESC[max - 1];
    /* No ESC-words */
    const unsigned int xlen = ht[t1].xlen;
    uint32_t const* table = (t1 == 2) ? &table23[0] : &table56[0];
    unsigned int sum = 0, sum2;

    do {
        unsigned int const x0 = *ix++;
        unsigned int const x1 = *ix++;
        sum += table[ x0 * xlen + x1 ];
    } while (ix < end);

    sum2 = sum & 0xffffu;
    sum >>= 16u;

    if (sum > sum2) {
        sum = sum2;
        t1++;
    }

    *s += sum;
    return t1;
}


inline static int
count_bit_noESC_from3(const int *ix, const int *end, int max, unsigned int * s,
                      LAME_UNUSED vector_impl_t impl)
{
    int t1 = huf_tbl_noESC[max - 1];
    /* No ESC-words */
    unsigned int sum1 = 0;
    unsigned int sum2 = 0;
    unsigned int sum3 = 0;
    const unsigned int xlen = ht[t1].xlen;
    const uint8_t *const hlen1 = ht[t1].hlen;
    const uint8_t *const hlen2 = ht[t1 + 1].hlen;
    const uint8_t *const hlen3 = ht[t1 + 2].hlen;
    int     t;

#if defined( HAVE_SSE2_INTRINSICS )
    if (impl >= VECTOR_IMPL_SSE2 && end - ix >= TABLE_SEARCH_VECTOR_MIN) {
        unsigned int sums[3];

        count_bit_noESC_from3_sse2(ix, end, (int) xlen, hlen1, hlen2, hlen3, sums);
        sum1 = sums[0];
        sum2 = sums[1];
        sum3 = sums[2];
    }
    else
#endif
    do {
        unsigned int x0 = *ix++;
        unsigned int x1 = *ix++;
        unsigned int x = x0 * xlen + x1;
        sum1 += hlen1[x];
        sum2 += hlen2[x];
        sum3 += hlen3[x];
    } while (ix < end);

    t = t1;
    if (sum1 > sum2) {
        sum1 = sum2;
        t++;
    }
    if (sum1 > sum3) {
        sum1 = sum3;
        t = t1 + 2;
    }
    *s += sum1;

    return t;  
}


/*************************************************************************/
/*	      choose table						 */
/*************************************************************************/

/*
  Choose the Huffman table that will encode ix[begin..end] with
  the fewest bits.

  Note: This code contains knowledge about the sizes and characteristics
  of the Huffman tables as defined in the IS (Table B.7), and will not work
  with any arbitrary tables.
*/
static int count_bit_null(LAME_UNUSED const int* ix, LAME_UNUSED const int* end,
                          LAME_UNUSED int max, LAME_UNUSED unsigned int* s,
                          LAME_UNUSED vector_impl_t impl)
{
    return 0;
}

/* The counting routines take the implementation to run rather than being
 * selected by it, so that the map from a region's maximum to its routine
 * stays the single table below however many tiers exist.
 */
typedef int (*count_fnc)(const int* ix, const int* end, int max, unsigned int* s,
                         vector_impl_t impl);

static const count_fnc count_fncs[] = 
{ &count_bit_null
, &count_bit_noESC
, &count_bit_noESC_from2
, &count_bit_noESC_from2
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
};

static int
choose_table_x(const int *ix, const int *const end, int *const _s, vector_impl_t impl)
{
    unsigned int* s = (unsigned int*)_s;
    unsigned int  max;
    int     choice, choice2;
    max = ix_max_v(ix, end, impl);

    if (max <= 15) {
      return count_fncs[max](ix, end, max, s, impl);
    }
    /* try tables with linbits */
    if (max > IXMAX_VAL) {
        *s = LARGE_BITS;
        return -1;
    }
    max -= 15u;
    for (choice2 = 24; choice2 < 32; choice2++) {
        if (ht[choice2].linmax >= max) {
            break;
        }
    }

    for (choice = choice2 - 8; choice < 24; choice++) {
        if (ht[choice].linmax >= max) {
            break;
        }
    }
    return count_bit_ESC(ix, end, choice, choice2, s, impl);
}


/* One entry point per implementation, so the choice is made once when the
 * encoder starts rather than at every region.
 */
static int
choose_table_c(const int *ix, const int *const end, int *const _s)
{
    return choose_table_x(ix, end, _s, VECTOR_IMPL_NONE);
}

#if defined( HAVE_SSE2_INTRINSICS )
static int
choose_table_sse2(const int *ix, const int *const end, int *const _s)
{
    return choose_table_x(ix, end, _s, VECTOR_IMPL_SSE2);
}
#endif

#if defined( HAVE_AVX2_INTRINSICS )
static int
choose_table_avx2(const int *ix, const int *const end, int *const _s)
{
    return choose_table_x(ix, end, _s, VECTOR_IMPL_AVX2);
}
#endif

#if defined( HAVE_NEON_INTRINSICS )
static int
choose_table_neon(const int *ix, const int *const end, int *const _s)
{
    return choose_table_x(ix, end, _s, VECTOR_IMPL_NEON);
}
#endif



/*************************************************************************/
/*	      count_bit							 */
/*************************************************************************/
int
noquant_count_bits(lame_internal_flags const *const gfc,
                   gr_info * const gi, calc_noise_data * prev_noise)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    int     bits = 0;
    int     i, a1, a2;
    int const *const ix = gi->quant.l3_enc;

    i = Min(SAMPLES_PER_GRANULE, ((gi->granule.max_nonzero_coeff + 2) >> 1) << 1);

    if (prev_noise)
        prev_noise->sfb_count1 = 0;

    /* Determine count1 region */
    for (; i > 1; i -= 2)
        if (ix[i - 1] | ix[i - 2])
            break;
    gi->huffman.count1 = i;

    /* Determines the number of bits to encode the quadruples. */
    a1 = a2 = 0;
    for (; i > 3; i -= 4) {
        int x4 = ix[i-4];
        int x3 = ix[i-3];
        int x2 = ix[i-2];
        int x1 = ix[i-1];
        int     p;
        /* hack to check if all values <= 1 */
        if ((unsigned int) (x4 | x3 | x2 | x1) > 1)
            break;

        p = ((x4 * 2 + x3) * 2 + x2) * 2 + x1;
        a1 += t32l[p];
        a2 += t33l[p];
    }

    bits = a1;
    gi->huffman.count1table_select = 0;
    if (a1 > a2) {
        bits = a2;
        gi->huffman.count1table_select = 1;
    }

    gi->huffman.count1bits = bits;
    gi->huffman.big_values = i;
    if (i == 0)
        return bits;

    if (gi->granule.block_type == SHORT_TYPE) {
        a1 = 3 * gfc->scalefac_band.s[3];
        if (a1 > gi->huffman.big_values)
            a1 = gi->huffman.big_values;
        a2 = gi->huffman.big_values;

    }
    else if (gi->granule.block_type == NORM_TYPE) {
        assert(i >= 2 && i <= SAMPLES_PER_GRANULE); /* reads bv_scf[i-2] and bv_scf[i-1] of SAMPLES_PER_GRANULE */
        a1 = gi->huffman.region0_count = gfc->sv_qnt.bv_scf[i - 2];
        a2 = gi->huffman.region1_count = gfc->sv_qnt.bv_scf[i - 1];

        assert(a1 + a2 + 2 < SBPSY_l);
        a2 = gfc->scalefac_band.l[a1 + a2 + 2];
        a1 = gfc->scalefac_band.l[a1 + 1];
        if (a2 < i)
            gi->huffman.table_select[2] = gfc->choose_table(ix + a2, ix + i, &bits);

    }
    else {
        gi->huffman.region0_count = SWITCHED_REGION0_COUNT;
        /*gi->region1_count = SBPSY_l - 7 - 1; */
        gi->huffman.region1_count = SBMAX_l - 1 - SWITCHED_REGION0_COUNT - 1;
        a1 = gfc->scalefac_band.l[SWITCHED_REGION0_COUNT + 1];
        a2 = i;
        if (a1 > a2) {
            a1 = a2;
        }
    }


    /* have to allow for the case when bigvalues < region0 < region1 */
    /* (and region0, region1 are ignored) */
    a1 = Min(a1, i);
    a2 = Min(a2, i);

    assert(a1 >= 0);
    assert(a2 >= 0);

    /* Count the number of bits necessary to code the bigvalues region. */
    if (0 < a1)
        gi->huffman.table_select[0] = gfc->choose_table(ix, ix + a1, &bits);
    if (a1 < a2)
        gi->huffman.table_select[1] = gfc->choose_table(ix + a1, ix + a2, &bits);
    if (cfg->use_best_huffman == 2) {
        gi->huffman.part2_3_length = bits;
        best_huffman_divide(gfc, gi);
        bits = gi->huffman.part2_3_length;
    }


    if (prev_noise) {
        if (gi->granule.block_type == NORM_TYPE) {
            int     sfb = 0;
            while (gfc->scalefac_band.l[sfb] < gi->huffman.big_values) {
                sfb++;
            }
            prev_noise->sfb_count1 = sfb;
        }
    }

    return bits;
}

int
count_bits(lame_internal_flags const *const gfc,
           const FLOAT * const xr, gr_info * const gi, calc_noise_data * prev_noise)
{
    int    *const ix = gi->quant.l3_enc;

    /* since quantize_xrpow uses table lookup, we need to check this first: */
    FLOAT const w = (IXMAX_VAL) / IPOW20(gi->quant.global_gain);

    if (gi->quant.xrpow_max > w)
        return LARGE_BITS;

    quantize_xrpow(xr, ix, IPOW20(gi->quant.global_gain), gi, prev_noise,
                   vector_implementation(gfc));

    if (gfc->sv_qnt.substep_shaping & SUBSTEP_HALF_STEP) {
        int     sfb, j = 0;
        /* 0.634521682242439 = 0.5946*2**(.5*0.1875) */
        int const gain = gi->quant.global_gain + gi->quant.scalefac_scale;
        const FLOAT roundfac = 0.634521682242439 / IPOW20(gain);
        for (sfb = 0; sfb < gi->granule.sfbmax; sfb++) {
            int const width = gi->granule.width[sfb];
            assert(width >= 0);
            if (!gfc->sv_qnt.pseudohalf[sfb]) {
                j += width;
            }
            else {
                int     k;
                for (k = j, j += width; k < j; ++k) {
                    ix[k] = (xr[k] >= roundfac) ? ix[k] : 0;
                }
            }
        }
    }
    return noquant_count_bits(gfc, gi, prev_noise);
}

/***********************************************************************
  re-calculate the best scalefac_compress using scfsi
  the saved bits are kept in the bit reservoir.
 **********************************************************************/

/**
 * \internal
 * \brief Entries of the tables kept per region0_count + region1_count.
 */
#define REGION_COUNT_SUMS (REGION0_COUNT_LIMIT - 1 + REGION1_COUNT_LIMIT - 1 + 1)

inline static void
recalc_divide_init(const lame_internal_flags * const gfc,
                   gr_info const *cod_info,
                   int const *const ix, int r01_bits[], int r01_div[], int r0_tbl[], int r1_tbl[])
{
    int     r0, r1, bigv, r0t, r1t, bits;

    bigv = cod_info->huffman.big_values;

    for (r0 = 0; r0 < REGION_COUNT_SUMS; r0++) {
        r01_bits[r0] = LARGE_BITS;
    }

    for (r0 = 0; r0 < REGION0_COUNT_LIMIT; r0++) {
        int const a1 = gfc->scalefac_band.l[r0 + 1];
        int     r0bits;
        if (a1 >= bigv)
            break;
        r0bits = 0;
        r0t = gfc->choose_table(ix, ix + a1, &r0bits);

        for (r1 = 0; r1 < REGION1_COUNT_LIMIT; r1++) {
            int const a2 = gfc->scalefac_band.l[r0 + r1 + 2];
            if (a2 >= bigv)
                break;

            bits = r0bits;
            r1t = gfc->choose_table(ix + a1, ix + a2, &bits);
            if (r01_bits[r0 + r1] > bits) {
                r01_bits[r0 + r1] = bits;
                r01_div[r0 + r1] = r0;
                r0_tbl[r0 + r1] = r0t;
                r1_tbl[r0 + r1] = r1t;
            }
        }
    }
}

inline static void
recalc_divide_sub(const lame_internal_flags * const gfc,
                  const gr_huffman * huffman2,
                  gr_info * const gi,
                  const int *const ix,
                  const int r01_bits[], const int r01_div[], const int r0_tbl[], const int r1_tbl[])
{
    int     bits, r2, a2, bigv, r2t;

    bigv = huffman2->big_values;

    for (r2 = 2; r2 < SBMAX_l + 1; r2++) {
        a2 = gfc->scalefac_band.l[r2];
        if (a2 >= bigv)
            break;

        bits = r01_bits[r2 - 2] + huffman2->count1bits;
        if (gi->huffman.part2_3_length <= bits)
            break;

        r2t = gfc->choose_table(ix + a2, ix + bigv, &bits);
        if (gi->huffman.part2_3_length <= bits)
            continue;

        gi->huffman = *huffman2;
        gi->huffman.part2_3_length = bits;
        gi->huffman.region0_count = r01_div[r2 - 2];
        gi->huffman.region1_count = r2 - 2 - r01_div[r2 - 2];
        gi->huffman.table_select[0] = r0_tbl[r2 - 2];
        gi->huffman.table_select[1] = r1_tbl[r2 - 2];
        gi->huffman.table_select[2] = r2t;
    }
}




void
best_huffman_divide(const lame_internal_flags * const gfc, gr_info * const gi)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    int     i, a1, a2;
    gr_huffman huffman2;
    int const *const ix = gi->quant.l3_enc;

    int     r01_bits[REGION_COUNT_SUMS];
    int     r01_div[REGION_COUNT_SUMS];
    int     r0_tbl[REGION_COUNT_SUMS];
    int     r1_tbl[REGION_COUNT_SUMS];


    /* SHORT BLOCK stuff fails for MPEG2 */
    if (gi->granule.block_type == SHORT_TYPE && cfg->mode_gr == 1)
        return;


    huffman2 = gi->huffman;
    if (gi->granule.block_type == NORM_TYPE) {
        recalc_divide_init(gfc, gi, ix, r01_bits, r01_div, r0_tbl, r1_tbl);
        recalc_divide_sub(gfc, &huffman2, gi, ix, r01_bits, r01_div, r0_tbl, r1_tbl);
    }

    i = huffman2.big_values;
    if (i == 0 || (unsigned int) (ix[i - 2] | ix[i - 1]) > 1)
        return;

    i = gi->huffman.count1 + 2;
    if (i > SAMPLES_PER_GRANULE)
        return;

    /* Determines the number of bits to encode the quadruples. */
    huffman2 = gi->huffman;
    huffman2.count1 = i;
    a1 = a2 = 0;

    assert(i <= SAMPLES_PER_GRANULE);

    for (; i > huffman2.big_values; i -= 4) {
        int const p = ((ix[i - 4] * 2 + ix[i - 3]) * 2 + ix[i - 2]) * 2 + ix[i - 1];
        a1 += t32l[p];
        a2 += t33l[p];
    }
    huffman2.big_values = i;

    huffman2.count1table_select = 0;
    if (a1 > a2) {
        a1 = a2;
        huffman2.count1table_select = 1;
    }

    huffman2.count1bits = a1;

    if (gi->granule.block_type == NORM_TYPE)
        recalc_divide_sub(gfc, &huffman2, gi, ix, r01_bits, r01_div, r0_tbl, r1_tbl);
    else {
        /* Count the number of bits necessary to code the bigvalues region. */
        huffman2.part2_3_length = a1;
        a1 = gfc->scalefac_band.l[SWITCHED_REGION0_COUNT + 1];
        if (a1 > i) {
            a1 = i;
        }
        if (a1 > 0)
            huffman2.table_select[0] =
                gfc->choose_table(ix, ix + a1,
                                  (int *) &huffman2.part2_3_length);
        if (i > a1)
            huffman2.table_select[1] =
                gfc->choose_table(ix + a1, ix + i,
                                  (int *) &huffman2.part2_3_length);
        if (gi->huffman.part2_3_length > huffman2.part2_3_length)
            gi->huffman = huffman2;
    }
}

static const int slen1_n[16] = { 1, 1, 1, 1, 8, 2, 2, 2, 4, 4, 4, 8, 8, 8, 16, 16 };
static const int slen2_n[16] = { 1, 2, 4, 8, 1, 2, 4, 8, 2, 4, 8, 2, 4, 8, 4, 8 };
const int slen1_tab[16] = { 0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4 };
const int slen2_tab[16] = { 0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3 };

static void
scfsi_calc(int ch, III_side_info_t * l3_side)
{
    unsigned int i;
    int     s1, s2, c1, c2;
    int     sfb;
    gr_info *const gi = &l3_side->tt[1][ch];
    gr_info const *const g0 = &l3_side->tt[0][ch];

    for (i = 0; i < (sizeof(scfsi_band) / sizeof(int)) - 1; i++) {
        for (sfb = scfsi_band[i]; sfb < scfsi_band[i + 1]; sfb++) {
            if (g0->quant.scalefac[sfb] != gi->quant.scalefac[sfb]
                && gi->quant.scalefac[sfb] >= 0)
                break;
        }
        if (sfb == scfsi_band[i + 1]) {
            for (sfb = scfsi_band[i]; sfb < scfsi_band[i + 1]; sfb++) {
                gi->quant.scalefac[sfb] = SCALEFAC_SCFSI;
            }
            l3_side->scfsi[ch][i] = 1;
        }
    }

    s1 = c1 = 0;
    for (sfb = 0; sfb < 11; sfb++) {
        if (gi->quant.scalefac[sfb] == SCALEFAC_SCFSI)
            continue;
        c1++;
        if (s1 < gi->quant.scalefac[sfb])
            s1 = gi->quant.scalefac[sfb];
    }

    s2 = c2 = 0;
    for (; sfb < SBPSY_l; sfb++) {
        if (gi->quant.scalefac[sfb] == SCALEFAC_SCFSI)
            continue;
        c2++;
        if (s2 < gi->quant.scalefac[sfb])
            s2 = gi->quant.scalefac[sfb];
    }

    for (i = 0; i < 16; i++) {
        if (s1 < slen1_n[i] && s2 < slen2_n[i]) {
            int const c = slen1_tab[i] * c1 + slen2_tab[i] * c2;
            if (gi->quant.part2_length > c) {
                gi->quant.part2_length = c;
                gi->quant.scalefac_compress = (int)i;
            }
        }
    }
}

/*
Find the optimal way to store the scalefactors.
Only call this routine after final scalefactors have been
chosen and the channel/granule will not be re-encoded.
 */
void
best_scalefac_store(const lame_internal_flags * gfc,
                    const int gr, const int ch, III_side_info_t * const l3_side)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    /* use scalefac_scale if we can */
    gr_info *const gi = &l3_side->tt[gr][ch];
    int     sfb, i, j, l;
    int     recalc = 0;

    /* remove scalefacs from bands with ix=0.  This idea comes
     * from the AAC ISO docs.  added mt 3/00 */
    /* check if l3_enc=0 */
    j = 0;
    for (sfb = 0; sfb < gi->granule.sfbmax; sfb++) {
        int const width = gi->granule.width[sfb];
        assert(width >= 0);
        for (l = j, j += width; l < j; ++l) {
            if (gi->quant.l3_enc[l] != 0)
                break;
        }
        if (l == j) {
            gi->quant.scalefac[sfb] = SCALEFAC_ANY;
            recalc = 1;
        }
    }

    if (!gi->quant.scalefac_scale && !gi->quant.preflag) {
        int     s = 0;
        for (sfb = 0; sfb < gi->granule.sfbmax; sfb++)
            if (gi->quant.scalefac[sfb] > 0)
                s |= gi->quant.scalefac[sfb];

        if (!(s & 1) && s != 0) {
            for (sfb = 0; sfb < gi->granule.sfbmax; sfb++)
                if (gi->quant.scalefac[sfb] > 0)
                    gi->quant.scalefac[sfb] >>= 1;

            gi->quant.scalefac_scale = recalc = 1;
        }
    }

    if (!gi->quant.preflag && gi->granule.block_type != SHORT_TYPE && cfg->mode_gr == 2) {
        for (sfb = 11; sfb < SBPSY_l; sfb++)
            if (gi->quant.scalefac[sfb] < pretab[sfb] && gi->quant.scalefac[sfb] != SCALEFAC_ANY)
                break;
        if (sfb == SBPSY_l) {
            for (sfb = 11; sfb < SBPSY_l; sfb++)
                if (gi->quant.scalefac[sfb] > 0)
                    gi->quant.scalefac[sfb] -= pretab[sfb];

            gi->quant.preflag = recalc = 1;
        }
    }

    for (i = 0; i < 4; i++)
        l3_side->scfsi[ch][i] = 0;

    if (cfg->mode_gr == 2 && gr == 1
        && l3_side->tt[0][ch].granule.block_type != SHORT_TYPE
        && l3_side->tt[1][ch].granule.block_type != SHORT_TYPE) {
        scfsi_calc(ch, l3_side);
        recalc = 0;
    }
    for (sfb = 0; sfb < gi->granule.sfbmax; sfb++) {
        if (gi->quant.scalefac[sfb] == SCALEFAC_ANY) {
            gi->quant.scalefac[sfb] = 0;
        }
    }
    if (recalc) {
        (void) scale_bitcount(gfc, gi);
    }
}


#ifndef NDEBUG
static int
all_scalefactors_not_negative(int const *scalefac, int n)
{
    int     i;
    for (i = 0; i < n; ++i) {
        if (scalefac[i] < 0)
            return 0;
    }
    return 1;
}
#endif


/* number of bits used to encode scalefacs */

/* 18*slen1_tab[i] + 18*slen2_tab[i] */
static const int scale_short[16] = {
    0, 18, 36, 54, 54, 36, 54, 72, 54, 72, 90, 72, 90, 108, 108, 126
};

/* 17*slen1_tab[i] + 18*slen2_tab[i] */
static const int scale_mixed[16] = {
    0, 18, 36, 54, 51, 35, 53, 71, 52, 70, 88, 69, 87, 105, 104, 122
};

/* 11*slen1_tab[i] + 10*slen2_tab[i] */
static const int scale_long[16] = {
    0, 10, 20, 30, 33, 21, 31, 41, 32, 42, 52, 43, 53, 63, 64, 74
};


/*************************************************************************/
/*            scale_bitcount                                             */
/*************************************************************************/

/* Also calculates the number of bits necessary to code the scalefactors. */

static int
mpeg1_scale_bitcount(LAME_UNUSED const lame_internal_flags * gfc, gr_info * const cod_info)
{
    int     k, sfb, max_slen1 = 0, max_slen2 = 0;

    /* maximum values */
    const int *tab;
    int    *const scalefac = cod_info->quant.scalefac;

    assert(all_scalefactors_not_negative(scalefac, cod_info->granule.sfbmax));

    if (cod_info->granule.block_type == SHORT_TYPE) {
        tab = scale_short;
        if (cod_info->granule.mixed_block_flag)
            tab = scale_mixed;
    }
    else {              /* block_type == 1,2,or 3 */
        tab = scale_long;
        if (!cod_info->quant.preflag) {
            for (sfb = 11; sfb < SBPSY_l; sfb++)
                if (scalefac[sfb] < pretab[sfb])
                    break;

            if (sfb == SBPSY_l) {
                cod_info->quant.preflag = 1;
                for (sfb = 11; sfb < SBPSY_l; sfb++)
                    scalefac[sfb] -= pretab[sfb];
            }
        }
    }

    for (sfb = 0; sfb < cod_info->granule.sfbdivide; sfb++)
        if (max_slen1 < scalefac[sfb])
            max_slen1 = scalefac[sfb];

    for (; sfb < cod_info->granule.sfbmax; sfb++)
        if (max_slen2 < scalefac[sfb])
            max_slen2 = scalefac[sfb];

    /* from Takehiro TOMINAGA <tominaga@isoternet.org> 10/99
     * loop over *all* posible values of scalefac_compress to find the
     * one which uses the smallest number of bits.  ISO would stop
     * at first valid index */
    cod_info->quant.part2_length = LARGE_BITS;
    for (k = 0; k < 16; k++) {
        if (max_slen1 < slen1_n[k] && max_slen2 < slen2_n[k]
            && cod_info->quant.part2_length > tab[k]) {
            cod_info->quant.part2_length = tab[k];
            cod_info->quant.scalefac_compress = k;
        }
    }
    return cod_info->quant.part2_length == LARGE_BITS;
}



/*
  table of largest scalefactor values for MPEG2
*/
static const int max_range_sfac_tab[6][4] = {
    {15, 15, 7, 7},
    {15, 15, 7, 0},
    {7, 3, 0, 0},
    {15, 31, 31, 0},
    {7, 7, 7, 0},
    {3, 3, 0, 0}
};




/*************************************************************************/
/*            scale_bitcount_lsf                                         */
/*************************************************************************/

/* Also counts the number of bits to encode the scalefacs but for MPEG 2 */
/* Lower sampling frequencies  (24, 22.05 and 16 kHz.)                   */

/*  This is reverse-engineered from section 2.4.3.2 of the MPEG2 IS,     */
/* "Audio Decoding Layer III"                                            */

static int
mpeg2_scale_bitcount(const lame_internal_flags * gfc, gr_info * const cod_info)
{
    int     table_number, row_in_table, partition, nr_sfb, window, over;
    int     i, sfb, max_sfac[4];
    const int *partition_table;
    int const *const scalefac = cod_info->quant.scalefac;

    /*
       Set partition table. Note that should try to use table one,
       but do not yet...
     */
    if (cod_info->quant.preflag)
        table_number = 2;
    else
        table_number = 0;

    for (i = 0; i < 4; i++)
        max_sfac[i] = 0;

    if (cod_info->granule.block_type == SHORT_TYPE) {
        row_in_table = 1;
        partition_table = &nr_of_sfb_block[table_number][row_in_table][0];
        for (sfb = 0, partition = 0; partition < 4; partition++) {
            nr_sfb = partition_table[partition] / 3;
            for (i = 0; i < nr_sfb; i++, sfb++)
                for (window = 0; window < 3; window++)
                    if (scalefac[sfb * 3 + window] > max_sfac[partition])
                        max_sfac[partition] = scalefac[sfb * 3 + window];
        }
    }
    else {
        row_in_table = 0;
        partition_table = &nr_of_sfb_block[table_number][row_in_table][0];
        for (sfb = 0, partition = 0; partition < 4; partition++) {
            nr_sfb = partition_table[partition];
            for (i = 0; i < nr_sfb; i++, sfb++)
                if (scalefac[sfb] > max_sfac[partition])
                    max_sfac[partition] = scalefac[sfb];
        }
    }

    for (over = 0, partition = 0; partition < 4; partition++) {
        if (max_sfac[partition] > max_range_sfac_tab[table_number][partition])
            over++;
    }
    if (!over) {
        /*
           Since no bands have been over-amplified, we can set scalefac_compress
           and slen[] for the formatter
         */
        static const int log2tab[] = { 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4 };

        int     slen1, slen2, slen3, slen4;

        cod_info->quant.sfb_partition_table = nr_of_sfb_block[table_number][row_in_table];
        for (partition = 0; partition < 4; partition++)
            cod_info->quant.slen[partition] = log2tab[max_sfac[partition]];

        /* set scalefac_compress */
        slen1 = cod_info->quant.slen[0];
        slen2 = cod_info->quant.slen[1];
        slen3 = cod_info->quant.slen[2];
        slen4 = cod_info->quant.slen[3];

        switch (table_number) {
        case 0:
            cod_info->quant.scalefac_compress = (((slen1 * 5) + slen2) << 4)
                + (slen3 << 2)
                + slen4;
            break;

        case 1:
            cod_info->quant.scalefac_compress = 400 + (((slen1 * 5) + slen2) << 2)
                + slen3;
            break;

        case 2:
            cod_info->quant.scalefac_compress = 500 + (slen1 * 3) + slen2;
            break;

        default:
            ERRORF(gfc, "intensity stereo not implemented yet\n");
            break;
        }
    }
#ifdef DEBUG
    if (over)
        DEBUGF(gfc, "---WARNING !! Amplification of some bands over limits\n");
#endif
    if (!over) {
        assert(cod_info->quant.sfb_partition_table);
        cod_info->quant.part2_length = 0;
        for (partition = 0; partition < 4; partition++)
            cod_info->quant.part2_length +=
                cod_info->quant.slen[partition] * cod_info->quant.sfb_partition_table[partition];
    }
    return over;
}


int
scale_bitcount(const lame_internal_flags * gfc, gr_info * cod_info)
{
    if (gfc->cfg.mode_gr == 2) {
        return mpeg1_scale_bitcount(gfc, cod_info);
    }
    else {
        return mpeg2_scale_bitcount(gfc, cod_info);
    }
}


void
huffman_init(lame_internal_flags * const gfc)
{
    int     i;

    /* Ascending, so the widest implementation the machine offers is the one
       left standing, and a tier that has no routines of its own still gets
       everything the tiers below it provide. */
    gfc->choose_table = choose_table_c;
#if defined( HAVE_SSE2_INTRINSICS )
    if (vector_implementation(gfc) >= VECTOR_IMPL_SSE2)
        gfc->choose_table = choose_table_sse2;
#endif
#if defined( HAVE_AVX2_INTRINSICS )
    if (vector_implementation(gfc) >= VECTOR_IMPL_AVX2)
        gfc->choose_table = choose_table_avx2;
#endif
#if defined( HAVE_NEON_INTRINSICS )
    if (vector_implementation(gfc) >= VECTOR_IMPL_NEON)
        gfc->choose_table = choose_table_neon;
#endif

    for (i = 2; i <= SAMPLES_PER_GRANULE; i += 2) {
        int     scfb_anz = 0, bv_index;
        while (gfc->scalefac_band.l[++scfb_anz] < i);

        bv_index = subdv_table[scfb_anz].region0_count;
        while (gfc->scalefac_band.l[bv_index + 1] > i)
            bv_index--;

        if (bv_index < 0) {
            /* this is an indication that everything is going to
               be encoded as region0:  bigvalues < region0 < region1
               so lets set region0, region1 to some value larger
               than bigvalues */
            bv_index = subdv_table[scfb_anz].region0_count;
        }

        gfc->sv_qnt.bv_scf[i - 2] = bv_index;

        bv_index = subdv_table[scfb_anz].region1_count;
        while (gfc->scalefac_band.l[bv_index + gfc->sv_qnt.bv_scf[i - 2] + 2] > i)
            bv_index--;

        if (bv_index < 0) {
            bv_index = subdv_table[scfb_anz].region1_count;
        }

        gfc->sv_qnt.bv_scf[i - 1] = bv_index;
    }
}
