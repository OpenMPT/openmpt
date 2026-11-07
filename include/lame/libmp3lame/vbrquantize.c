/*
 *	MP3 quantization
 *
 *	Copyright (c) 1999-2000 Mark Taylor
 *	Copyright (c) 2000-2012 Robert Hegemann
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
#  include <config.h>
#endif


#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "vbrquantize.h"
#include "quantize_pvt.h"
#include "vector/lame_intrin.h"




struct algo_s;
typedef struct algo_s algo_t;

typedef void (*alloc_sf_f) (const algo_t *, const int *, const int *, int);
typedef uint8_t (*find_sf_f) (const FLOAT *, const FLOAT *, FLOAT, unsigned int, uint8_t,
                              vector_impl_t);

struct algo_s {
    alloc_sf_f alloc;
    find_sf_f  find;
    const FLOAT *xr34orig;
    lame_internal_flags *gfc;
    gr_info *cod_info;
    int     mingain_l;
    int     mingain_s[3];
};



/*  Remarks on optimizing compilers:
 *
 *  the MSVC compiler may get into aliasing problems when accessing
 *  memory through the fi_union. declaring it volatile does the trick here
 *
 *  the calc_sfb_noise_* functions are not inlined because the intel compiler
 *  optimized executeables won't work as expected anymore
 */

#ifdef _MSC_VER
#  if _MSC_VER < 1400
#  define VOLATILE volatile
#  else
#  define VOLATILE
#  endif
#else
#  define VOLATILE
#endif

typedef VOLATILE union {
    float   f;
    int     i;
} fi_union;



#if TAKEHIRO_IEEE754_HACK
#define DOUBLEX double
#else
#define DOUBLEX FLOAT
#endif
 
#define MAGIC_FLOAT_def (65536*(128))
#define MAGIC_INT_def    0x4b000000

#if TAKEHIRO_IEEE754_HACK
#else
/*********************************************************************
 * XRPOW_FTOI is a macro to convert floats to ints.
 * if XRPOW_FTOI(x) = nearest_int(x), then QUANTFAC(x)=adj43asm[x]
 *                                         rounding offset -0.0946
 *
 * if XRPOW_FTOI(x) = floor(x), then QUANTFAC(x)=asj43[x]
 *                                   rounding offset 0.4054
 *********************************************************************/
#  define QUANTFAC(rx)  adj43[rx]
#  define XRPOW_FTOI(src,dest) ((dest) = (int)(src))
#endif

#if TAKEHIRO_IEEE754_HACK
static int const MAGIC_INT = MAGIC_INT_def;
static DOUBLEX const MAGIC_FLOAT = MAGIC_FLOAT_def;
#endif


inline static  float
vec_max_c(const float * xr34, unsigned int bw)
{
    float   xfsf = 0;
    unsigned int i = bw >> 2u;
    unsigned int const remaining = (bw & 0x03u);

    while (i-- > 0) {
        if (xfsf < xr34[0]) {
            xfsf = xr34[0];
        }
        if (xfsf < xr34[1]) {
            xfsf = xr34[1];
        }
        if (xfsf < xr34[2]) {
            xfsf = xr34[2];
        }
        if (xfsf < xr34[3]) {
            xfsf = xr34[3];
        }
        xr34 += 4;
    }
    switch( remaining ) {
    case 3: if (xfsf < xr34[2]) xfsf = xr34[2]; /* fall through */
    case 2: if (xfsf < xr34[1]) xfsf = xr34[1]; /* fall through */
    case 1: if (xfsf < xr34[0]) xfsf = xr34[0];
    default: break;
    }
    return xfsf;
}


inline static  uint8_t
find_lowest_scalefac(const FLOAT xr34)
{
    uint8_t sf_ok = 255;
    uint8_t sf = 128, delsf = 64;
    uint8_t i;
    FLOAT const ixmax_val = IXMAX_VAL;
    for (i = 0; i < 8; ++i) {
        FLOAT const xfsf = ipow20[sf] * xr34;
        if (xfsf <= ixmax_val) {
            sf_ok = sf;
            sf -= delsf;
        }
        else {
            sf += delsf;
        }
        delsf >>= 1;
    }
    return sf_ok;
}


inline static void
k_34_4(DOUBLEX x[4], int l3[4])
{
#if TAKEHIRO_IEEE754_HACK
    fi_union fi[4];

    assert(x[0] <= IXMAX_VAL && x[1] <= IXMAX_VAL && x[2] <= IXMAX_VAL && x[3] <= IXMAX_VAL);
    x[0] += MAGIC_FLOAT;
    fi[0].f = x[0];
    x[1] += MAGIC_FLOAT;
    fi[1].f = x[1];
    x[2] += MAGIC_FLOAT;
    fi[2].f = x[2];
    x[3] += MAGIC_FLOAT;
    fi[3].f = x[3];
    fi[0].f = x[0] + adj43asm[fi[0].i - MAGIC_INT];
    fi[1].f = x[1] + adj43asm[fi[1].i - MAGIC_INT];
    fi[2].f = x[2] + adj43asm[fi[2].i - MAGIC_INT];
    fi[3].f = x[3] + adj43asm[fi[3].i - MAGIC_INT];
    l3[0] = fi[0].i - MAGIC_INT;
    l3[1] = fi[1].i - MAGIC_INT;
    l3[2] = fi[2].i - MAGIC_INT;
    l3[3] = fi[3].i - MAGIC_INT;
#else
    assert(x[0] <= IXMAX_VAL && x[1] <= IXMAX_VAL && x[2] <= IXMAX_VAL && x[3] <= IXMAX_VAL);
    XRPOW_FTOI(x[0], l3[0]);
    XRPOW_FTOI(x[1], l3[1]);
    XRPOW_FTOI(x[2], l3[2]);
    XRPOW_FTOI(x[3], l3[3]);
    x[0] += QUANTFAC(l3[0]);
    x[1] += QUANTFAC(l3[1]);
    x[2] += QUANTFAC(l3[2]);
    x[3] += QUANTFAC(l3[3]);
    XRPOW_FTOI(x[0], l3[0]);
    XRPOW_FTOI(x[1], l3[1]);
    XRPOW_FTOI(x[2], l3[2]);
    XRPOW_FTOI(x[3], l3[3]);
#endif
}





/*  do call the calc_sfb_noise_* functions only with sf values
 *  for which holds: sfpow34*xr34 <= IXMAX_VAL
 */

/* Bands narrower than this run the scalar path: a fifth to a third of the calls
 * sit below it (the widths are scalefactor bands, so many are tiny - a bw
 * histogram over a -V2 encode has 38 % of calls below 8 and the most common
 * width is 4), and there the vector setup and the horizontal reduce do not earn
 * back their cost.  Taken from that histogram, not tuned.
 */
#define CALC_SFB_NOISE_VECTOR_MIN      8

/** @internal
 * This routine has an SSE2 version only.  On the tested CPUs, AVX2 and
 * AVX-512 versions are not faster.  The AVX-512 version costs about 5 % of a
 * `-V 2` encode on Ice Lake, and is at the noise floor on Zen 4.
 *
 * The reason is the reduction.  It is done four blocks at a time, so that the
 * output is the same on all CPUs.  A wider vector does not change this, so the
 * extra lanes only add cost.  The hardware gather of AVX2 is also not faster
 * than the manual gather of SSE2 on the tested cores.  The measurements are in
 * @ref vector_dispatch.
 *
 * A routine may exist at one tier only, and this is such a routine.
 */
static  FLOAT
calc_sfb_noise_x34(const FLOAT * xr, const FLOAT * xr34, unsigned int bw, uint8_t sf,
                   LAME_UNUSED vector_impl_t impl)
{
    DOUBLEX x[4];
    int     l3[4];
    const FLOAT sfpow = pow20[sf + Q_MAX2]; /*pow(2.0,sf/4.0); */
    const FLOAT sfpow34 = ipow20[sf]; /*pow(sfpow,-3.0/4.0); */

    FLOAT   xfsf = 0;

    /* The SSE2 tier pins gcc/clang's (x0^2+x2^2)+(x1^2+x3^2) reduction so all
       CPUs of an architecture agree; the hack formulation is a different
       computation and keeps its own scalar path (maintainer directive). */
#if !TAKEHIRO_IEEE754_HACK
#if defined( HAVE_SSE2_INTRINSICS )
    if (impl >= VECTOR_IMPL_SSE2 && bw >= CALC_SFB_NOISE_VECTOR_MIN) {
        return calc_sfb_noise_x34_sse2(xr, xr34, bw, sfpow, sfpow34, adj43, pow43);
    }
#endif
#endif
    unsigned int i = bw >> 2u;
    unsigned int const remaining = (bw & 0x03u);

    while (i-- > 0) {
        x[0] = sfpow34 * xr34[0];
        x[1] = sfpow34 * xr34[1];
        x[2] = sfpow34 * xr34[2];
        x[3] = sfpow34 * xr34[3];

        k_34_4(x, l3);

        x[0] = fabsf(xr[0]) - sfpow * pow43[l3[0]];
        x[1] = fabsf(xr[1]) - sfpow * pow43[l3[1]];
        x[2] = fabsf(xr[2]) - sfpow * pow43[l3[2]];
        x[3] = fabsf(xr[3]) - sfpow * pow43[l3[3]];
        xfsf += (x[0] * x[0] + x[1] * x[1]) + (x[2] * x[2] + x[3] * x[3]);

        xr += 4;
        xr34 += 4;
    }
    if (remaining) {
        x[0] = x[1] = x[2] = x[3] = 0;
        switch( remaining ) {
        case 3: x[2] = sfpow34 * xr34[2]; /* fall through */
        case 2: x[1] = sfpow34 * xr34[1]; /* fall through */
        case 1: x[0] = sfpow34 * xr34[0];
        }

        k_34_4(x, l3);
        x[0] = x[1] = x[2] = x[3] = 0;

        switch( remaining ) {
        case 3: x[2] = fabsf(xr[2]) - sfpow * pow43[l3[2]]; /* fall through */
        case 2: x[1] = fabsf(xr[1]) - sfpow * pow43[l3[1]]; /* fall through */
        case 1: x[0] = fabsf(xr[0]) - sfpow * pow43[l3[0]];
        }
        xfsf += (x[0] * x[0] + x[1] * x[1]) + (x[2] * x[2] + x[3] * x[3]);
    }
    return xfsf;
}



struct calc_noise_cache {
    int     valid;
    FLOAT   value;
};

typedef struct calc_noise_cache calc_noise_cache_t;


static  uint8_t
tri_calc_sfb_noise_x34(const FLOAT * xr, const FLOAT * xr34, FLOAT l3_xmin, unsigned int bw,
                       uint8_t sf, calc_noise_cache_t * did_it, vector_impl_t impl)
{
    if (did_it[sf].valid == 0) {
        did_it[sf].valid = 1;
        did_it[sf].value = calc_sfb_noise_x34(xr, xr34, bw, sf, impl);
    }
    if (l3_xmin < did_it[sf].value) {
        return 1;
    }
    if (sf < 255) {
        uint8_t const sf_x = sf + 1;
        if (did_it[sf_x].valid == 0) {
            did_it[sf_x].valid = 1;
            did_it[sf_x].value = calc_sfb_noise_x34(xr, xr34, bw, sf_x, impl);
        }
        if (l3_xmin < did_it[sf_x].value) {
            return 1;
        }
    }
    if (sf > 0) {
        uint8_t const sf_x = sf - 1;
        if (did_it[sf_x].valid == 0) {
            did_it[sf_x].valid = 1;
            did_it[sf_x].value = calc_sfb_noise_x34(xr, xr34, bw, sf_x, impl);
        }
        if (l3_xmin < did_it[sf_x].value) {
            return 1;
        }
    }
    return 0;
}


/**
 *  \brief Computes the quantization step size from the allowed masking.
 *
 *  Robert Hegemann 2001-05-01
 */
static int
calc_scalefac(FLOAT l3_xmin, int bw)
{
    FLOAT const c = 5.799142446; /* 10 * 10^(2/3) * log10(4/3) */
    return GLOBAL_GAIN_UNITY + (int) (c * log10f(l3_xmin / bw) - .5f);
}

static uint8_t
guess_scalefac_x34(LAME_UNUSED const FLOAT * xr, LAME_UNUSED const FLOAT * xr34, FLOAT l3_xmin,
                   unsigned int bw, uint8_t sf_min, LAME_UNUSED vector_impl_t impl)
{
    int const guess = calc_scalefac(l3_xmin, bw);
    if (guess < sf_min) return sf_min;
    if (guess >= GLOBAL_GAIN_MAX) return GLOBAL_GAIN_MAX;
    return guess;
}


/* the find_scalefac* routines calculate
 * a quantization step size which would
 * introduce as much noise as is allowed.
 * The larger the step size the more
 * quantization noise we'll get. The
 * scalefactors are there to lower the
 * global step size, allowing limited
 * differences in quantization step sizes
 * per band (shaping the noise).
 */

static  uint8_t
find_scalefac_x34(const FLOAT * xr, const FLOAT * xr34, FLOAT l3_xmin, unsigned int bw,
                  uint8_t sf_min, vector_impl_t impl)
{
    calc_noise_cache_t did_it[256];
    uint8_t sf = 128, sf_ok = 255, delsf = 128, seen_good_one = 0, i;
    memset(did_it, 0, sizeof(did_it));
    for (i = 0; i < 8; ++i) {
        delsf >>= 1;
        if (sf <= sf_min) {
            sf += delsf;
        }
        else {
            uint8_t const bad = tri_calc_sfb_noise_x34(xr, xr34, l3_xmin, bw, sf, did_it, impl);
            if (bad) {  /* distortion.  try a smaller scalefactor */
                sf -= delsf;
            }
            else {
                sf_ok = sf;
                sf += delsf;
                seen_good_one = 1;
            }
        }
    }
    /*  returning a scalefac without distortion, if possible
     */
    if (seen_good_one > 0) {
        sf = sf_ok;
    }
    if (sf <= sf_min) {
        sf = sf_min;
    }
    return sf;
}



/***********************************************************************
 *
 *      calc_short_block_vbr_sf()
 *      calc_long_block_vbr_sf()
 *
 *  Mark Taylor 2000-??-??
 *  Robert Hegemann 2000-10-25 made functions of it
 *
 ***********************************************************************/

/* a variation for vbr-mtrh */
static int
block_sf(algo_t * that, const FLOAT l3_xmin[SFBMAX], int vbrsf[SFBMAX], int vbrsfmin[SFBMAX])
{
    FLOAT   max_xr34;
    const FLOAT *const xr = &that->cod_info->granule.xr[0];
    const FLOAT *const xr34_orig = &that->xr34orig[0];
    const int *const width = &that->cod_info->granule.width[0];
    vector_impl_t const impl = vector_implementation(that->gfc);
    const char *const energy_above_cutoff = &that->cod_info->granule.energy_above_cutoff[0];
    unsigned int const max_nonzero_coeff = (unsigned int) that->cod_info->granule.max_nonzero_coeff;
    uint8_t maxsf = 0;
    int     sfb = 0, m_o = -1;
    unsigned int j = 0, i = 0;
    int const psymax = that->cod_info->granule.psymax;

    assert(that->cod_info->granule.max_nonzero_coeff >= 0);

    that->mingain_l = 0;
    that->mingain_s[0] = 0;
    that->mingain_s[1] = 0;
    that->mingain_s[2] = 0;
    while (j <= max_nonzero_coeff) {
        unsigned int const w = (unsigned int) width[sfb];
        unsigned int const m = (unsigned int) (max_nonzero_coeff - j + 1);
        unsigned int l = w;
        uint8_t m1, m2;
        if (l > m) {
            l = m;
        }
        max_xr34 = vec_max_c(&xr34_orig[j], l);

        m1 = find_lowest_scalefac(max_xr34);
        vbrsfmin[sfb] = m1;
        if (that->mingain_l < m1) {
            that->mingain_l = m1;
        }
        if (that->mingain_s[i] < m1) {
            that->mingain_s[i] = m1;
        }
        if (++i > 2) {
            i = 0;
        }
        if (sfb < psymax && w > 2) { /* mpeg2.5 at 8 kHz doesn't use all scalefactors, unused have width 2 */
            if (energy_above_cutoff[sfb]) {
                m2 = that->find(&xr[j], &xr34_orig[j], l3_xmin[sfb], l, m1, impl);
#if 0
                if (0) {
                    /*  Robert Hegemann 2007-09-29:
                     *  It seems here is some more potential for speed improvements.
                     *  Current find method does 11-18 quantization calculations.
                     *  Using a "good guess" may help to reduce this amount.
                     */
                    uint8_t guess = calc_scalefac(l3_xmin[sfb], l);
                    DEBUGF(that->gfc, "sfb=%3d guess=%3d found=%3d diff=%3d\n", sfb, guess, m2,
                           m2 - guess);
                }
#endif
                if (maxsf < m2) {
                    maxsf = m2;
                }
                if (m_o < m2 && m2 < 255) {
                    m_o = m2;
                }
            }
            else {
                m2 = 255;
                maxsf = 255;
            }
        }
        else {
            if (maxsf < m1) {
                maxsf = m1;
            }
            m2 = maxsf;
        }
        vbrsf[sfb] = m2;
        ++sfb;
        j += w;        
    }
    for (; sfb < SFBMAX; ++sfb) {
        vbrsf[sfb] = maxsf;
        vbrsfmin[sfb] = 0;
    }
    if (m_o > -1) {
        maxsf = m_o;
        for (sfb = 0; sfb < SFBMAX; ++sfb) {
            if (vbrsf[sfb] == 255) {
                vbrsf[sfb] = m_o;
            }
        }
    }
    return maxsf;
}



/***********************************************************************
 *
 *  quantize xr34 based on scalefactors
 *
 *  block_xr34
 *
 *  Mark Taylor 2000-??-??
 *  Robert Hegemann 2000-10-20 made functions of them
 *
 ***********************************************************************/

static void
quantize_x34(const algo_t * that)
{
    DOUBLEX x[4];
    const FLOAT *xr34_orig = that->xr34orig;
    gr_info *const cod_info = that->cod_info;
    int    *l3 = cod_info->quant.l3_enc;
    unsigned int j = 0, sfb = 0;
    unsigned int const max_nonzero_coeff = (unsigned int) cod_info->granule.max_nonzero_coeff;

    assert(cod_info->granule.max_nonzero_coeff >= 0);
    assert(cod_info->granule.max_nonzero_coeff < SAMPLES_PER_GRANULE);

    while (j <= max_nonzero_coeff) {
        int const step = sfb_quant_step(cod_info, sfb);
        uint8_t const sfac = (uint8_t) step;
        FLOAT const sfpow34 = ipow20[sfac];
        unsigned int const w = (unsigned int) cod_info->granule.width[sfb];
        unsigned int const m = (unsigned int) (max_nonzero_coeff - j + 1);
        unsigned int i, remaining;

        assert(step >= 0);
        assert(cod_info->granule.width[sfb] >= 0);
        j += w;
        ++sfb;
        
        i = (w <= m) ? w : m;
        remaining = (i & 0x03u);
        i >>= 2u;

        while (i-- > 0) {
            x[0] = sfpow34 * xr34_orig[0];
            x[1] = sfpow34 * xr34_orig[1];
            x[2] = sfpow34 * xr34_orig[2];
            x[3] = sfpow34 * xr34_orig[3];

            k_34_4(x, l3);

            l3 += 4;
            xr34_orig += 4;
        }
        if (remaining) {
            int tmp_l3[4];
            x[0] = x[1] = x[2] = x[3] = 0;
            switch( remaining ) {
            case 3: x[2] = sfpow34 * xr34_orig[2]; /* fall through */
            case 2: x[1] = sfpow34 * xr34_orig[1]; /* fall through */
            case 1: x[0] = sfpow34 * xr34_orig[0];
            }

            k_34_4(x, tmp_l3);

            switch( remaining ) {
            case 3: l3[2] = tmp_l3[2]; /* fall through */
            case 2: l3[1] = tmp_l3[1]; /* fall through */
            case 1: l3[0] = tmp_l3[0];
            }

            l3 += remaining;
            xr34_orig += remaining;
        }
    }
}



static const uint8_t max_range_short[SBMAX_s * 3] = {
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    0, 0, 0
};

static const uint8_t max_range_long[SBMAX_l] = {
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 0
};

static const uint8_t max_range_long_lsf_pretab[SBMAX_l] = {
    7, 7, 7, 7, 7, 7, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};



/*
    sfb=0..5  scalefac < 16
    sfb>5     scalefac < 8

    ifqstep = ( cod_info->scalefac_scale == 0 ) ? 2 : 4;
    ol_sf =  (cod_info->global_gain-210.0);
    ol_sf -= 8*cod_info->subblock_gain[i];
    ol_sf -= ifqstep*scalefac[gr][ch].s[sfb][i];
*/

static void
set_subblock_gain(gr_info * cod_info, const int mingain_s[3], int sf[])
{
    const int maxrange1 = 15, maxrange2 = 7;
    const int ifqstepShift = (cod_info->quant.scalefac_scale == 0) ? 1 : 2;
    int    *const sbg = cod_info->quant.subblock_gain;
    unsigned int const psymax = (unsigned int) cod_info->granule.psymax;
    unsigned int psydiv = 18;
    int     sbg0, sbg1, sbg2;
    unsigned int sfb, i;
    int     min_sbg = 7;

    if (psydiv > psymax) {
        psydiv = psymax;
    }
    for (i = 0; i < 3; ++i) {
        int     maxsf1 = 0, maxsf2 = 0, minsf = 1000;
        /* see if we should use subblock gain */
        for (sfb = i; sfb < psydiv; sfb += 3) { /* part 1 */
            int const v = -sf[sfb];
            if (maxsf1 < v) {
                maxsf1 = v;
            }
            if (minsf > v) {
                minsf = v;
            }
        }
        for (; sfb < SFBMAX; sfb += 3) { /* part 2 */
            int const v = -sf[sfb];
            if (maxsf2 < v) {
                maxsf2 = v;
            }
            if (minsf > v) {
                minsf = v;
            }
        }

        /* boost subblock gain as little as possible so we can
         * reach maxsf1 with scalefactors
         * 8*sbg >= maxsf1
         */
        {
            int const m1 = maxsf1 - (maxrange1 << ifqstepShift);
            int const m2 = maxsf2 - (maxrange2 << ifqstepShift);

            maxsf1 = Max(m1, m2);
        }
        if (minsf > 0) {
            sbg[i] = minsf >> 3;
        }
        else {
            sbg[i] = 0;
        }
        if (maxsf1 > 0) {
            int const m1 = sbg[i];
            int const m2 = (maxsf1 + 7) >> 3;
            sbg[i] = Max(m1, m2);
        }
        if (sbg[i] > 0 && mingain_s[i] > (cod_info->quant.global_gain - sbg[i] * 8)) {
            sbg[i] = (cod_info->quant.global_gain - mingain_s[i]) >> 3;
        }
        if (sbg[i] > 7) {
            sbg[i] = 7;
        }
        if (min_sbg > sbg[i]) {
            min_sbg = sbg[i];
        }
    }
    sbg0 = sbg[0] * 8;
    sbg1 = sbg[1] * 8;
    sbg2 = sbg[2] * 8;
    for (sfb = 0; sfb < SFBMAX; sfb += 3) {
        sf[sfb + 0] += sbg0;
        sf[sfb + 1] += sbg1;
        sf[sfb + 2] += sbg2;
    }
    if (min_sbg > 0) {
        for (i = 0; i < 3; ++i) {
            sbg[i] -= min_sbg;
        }
        cod_info->quant.global_gain -= min_sbg * 8;
    }
}



/*
	  ifqstep = ( cod_info->scalefac_scale == 0 ) ? 2 : 4;
	  ol_sf =  (cod_info->global_gain-210.0);
	  ol_sf -= ifqstep*scalefac[gr][ch].l[sfb];
	  if (cod_info->preflag && sfb>=11)
	  ol_sf -= ifqstep*pretab[sfb];
*/
static void
set_scalefacs(gr_info * cod_info, const int *vbrsfmin, int sf[], const uint8_t * max_range)
{
    const int ifqstep = (cod_info->quant.scalefac_scale == 0) ? 2 : 4;
    const int ifqstepShift = (cod_info->quant.scalefac_scale == 0) ? 1 : 2;
    int    *const scalefac = cod_info->quant.scalefac;
    int const sfbmax = cod_info->granule.sfbmax;
    int     sfb;
    int const *const sbg = cod_info->quant.subblock_gain;
    int const *const window = cod_info->granule.window;
    int const preflag = cod_info->quant.preflag;

    if (preflag) {
        for (sfb = 11; sfb < sfbmax; ++sfb) {
            sf[sfb] += pretab[sfb] * ifqstep;
        }
    }
    for (sfb = 0; sfb < sfbmax; ++sfb) {
        int const gain = cod_info->quant.global_gain - (sbg[window[sfb]] * 8)
            - ((preflag ? pretab[sfb] : 0) * ifqstep);

        if (sf[sfb] < 0) {
            int const m = gain - vbrsfmin[sfb];
            /* ifqstep*scalefac >= -sf[sfb], so round UP */
            scalefac[sfb] = (ifqstep - 1 - sf[sfb]) >> ifqstepShift;

            if (scalefac[sfb] > max_range[sfb]) {
                scalefac[sfb] = max_range[sfb];
            }
            if (scalefac[sfb] > 0 && (scalefac[sfb] << ifqstepShift) > m) {
                scalefac[sfb] = m >> ifqstepShift;
            }
        }
        else {
            scalefac[sfb] = 0;
        }
    }
    for (; sfb < SFBMAX; ++sfb) {
        scalefac[sfb] = 0; /* sfb21 */
    }
}


#ifndef NDEBUG
static int
checkScalefactor(const gr_info * cod_info, const int vbrsfmin[SFBMAX])
{
    int     sfb;
    for (sfb = 0; sfb < cod_info->granule.psymax; ++sfb) {
        if (sfb_quant_step(cod_info, sfb) < vbrsfmin[sfb]) {
            return 0;
        }
    }
    return 1;
}
#endif


/******************************************************************
 *
 *  short block scalefacs
 *
 ******************************************************************/

static void
short_block_constrain(const algo_t * that, const int vbrsf[SFBMAX],
                      const int vbrsfmin[SFBMAX], int vbrmax)
{
    gr_info *const cod_info = that->cod_info;
    lame_internal_flags const *const gfc = that->gfc;
    SessionConfig_t const *const cfg = &gfc->cfg;
    int const maxminsfb = that->mingain_l;
    int     mover, maxover0 = 0, maxover1 = 0, delta = 0;
    int     v, v0, v1;
    int     sfb;
    int const psymax = cod_info->granule.psymax;

    for (sfb = 0; sfb < psymax; ++sfb) {
        assert(vbrsf[sfb] >= vbrsfmin[sfb]);
        v = vbrmax - vbrsf[sfb];
        if (delta < v) {
            delta = v;
        }
        v0 = v - (4 * 14 + 2 * max_range_short[sfb]);
        v1 = v - (4 * 14 + 4 * max_range_short[sfb]);
        if (maxover0 < v0) {
            maxover0 = v0;
        }
        if (maxover1 < v1) {
            maxover1 = v1;
        }
    }
    if (cfg->noise_shaping == 2) {
        /* allow scalefac_scale=1 */
        mover = Min(maxover0, maxover1);
    }
    else {
        mover = maxover0;
    }
    if (delta > mover) {
        delta = mover;
    }
    vbrmax -= delta;
    maxover0 -= mover;
    maxover1 -= mover;

    if (maxover0 == 0) {
        cod_info->quant.scalefac_scale = 0;
    }
    else if (maxover1 == 0) {
        cod_info->quant.scalefac_scale = 1;
    }
    if (vbrmax < maxminsfb) {
        vbrmax = maxminsfb;
    }
    cod_info->quant.global_gain = vbrmax;

    if (cod_info->quant.global_gain < 0) {
        cod_info->quant.global_gain = 0;
    }
    else if (cod_info->quant.global_gain > GLOBAL_GAIN_MAX) {
        cod_info->quant.global_gain = GLOBAL_GAIN_MAX;
    }
    {
        int     sf_temp[SFBMAX];
        for (sfb = 0; sfb < SFBMAX; ++sfb) {
            sf_temp[sfb] = vbrsf[sfb] - vbrmax;
        }
        set_subblock_gain(cod_info, &that->mingain_s[0], sf_temp);
        set_scalefacs(cod_info, vbrsfmin, sf_temp, max_range_short);
    }
    assert(checkScalefactor(cod_info, vbrsfmin));
}



/******************************************************************
 *
 *  long block scalefacs
 *
 ******************************************************************/

static void
long_block_constrain(const algo_t * that, const int vbrsf[SFBMAX], const int vbrsfmin[SFBMAX],
                     int vbrmax)
{
    gr_info *const cod_info = that->cod_info;
    lame_internal_flags const *const gfc = that->gfc;
    SessionConfig_t const *const cfg = &gfc->cfg;
    uint8_t const *max_rangep;
    int const maxminsfb = that->mingain_l;
    int     sfb;
    int     maxover0, maxover1, maxover0p, maxover1p, mover, delta = 0;
    int     v, v0, v1, v0p, v1p, vm0p = 1, vm1p = 1;
    int const psymax = cod_info->granule.psymax;

    max_rangep = cfg->mode_gr == 2 ? max_range_long : max_range_long_lsf_pretab;

    maxover0 = 0;
    maxover1 = 0;
    maxover0p = 0;      /* pretab */
    maxover1p = 0;      /* pretab */

    for (sfb = 0; sfb < psymax; ++sfb) {
        assert(vbrsf[sfb] >= vbrsfmin[sfb]);
        v = vbrmax - vbrsf[sfb];
        if (delta < v) {
            delta = v;
        }
        v0 = v - 2 * max_range_long[sfb];
        v1 = v - 4 * max_range_long[sfb];
        v0p = v - 2 * (max_rangep[sfb] + pretab[sfb]);
        v1p = v - 4 * (max_rangep[sfb] + pretab[sfb]);
        if (maxover0 < v0) {
            maxover0 = v0;
        }
        if (maxover1 < v1) {
            maxover1 = v1;
        }
        if (maxover0p < v0p) {
            maxover0p = v0p;
        }
        if (maxover1p < v1p) {
            maxover1p = v1p;
        }
    }
    if (vm0p == 1) {
        int     gain = vbrmax - maxover0p;
        if (gain < maxminsfb) {
            gain = maxminsfb;
        }
        for (sfb = 0; sfb < psymax; ++sfb) {
            int const a = (gain - vbrsfmin[sfb]) - 2 * pretab[sfb];
            if (a <= 0) {
                vm0p = 0;
                vm1p = 0;
                break;
            }
        }
    }
    if (vm1p == 1) {
        int     gain = vbrmax - maxover1p;
        if (gain < maxminsfb) {
            gain = maxminsfb;
        }
        for (sfb = 0; sfb < psymax; ++sfb) {
            int const b = (gain - vbrsfmin[sfb]) - 4 * pretab[sfb];
            if (b <= 0) {
                vm1p = 0;
                break;
            }
        }
    }
    if (vm0p == 0) {
        maxover0p = maxover0;
    }
    if (vm1p == 0) {
        maxover1p = maxover1;
    }
    if (cfg->noise_shaping != 2) {
        maxover1 = maxover0;
        maxover1p = maxover0p;
    }
    mover = Min(maxover0, maxover0p);
    mover = Min(mover, maxover1);
    mover = Min(mover, maxover1p);

    if (delta > mover) {
        delta = mover;
    }
    vbrmax -= delta;
    if (vbrmax < maxminsfb) {
        vbrmax = maxminsfb;
    }
    maxover0 -= mover;
    maxover0p -= mover;
    maxover1 -= mover;
    maxover1p -= mover;

    if (maxover0 == 0) {
        cod_info->quant.scalefac_scale = 0;
        cod_info->quant.preflag = 0;
        max_rangep = max_range_long;
    }
    else if (maxover0p == 0) {
        cod_info->quant.scalefac_scale = 0;
        cod_info->quant.preflag = 1;
    }
    else if (maxover1 == 0) {
        cod_info->quant.scalefac_scale = 1;
        cod_info->quant.preflag = 0;
        max_rangep = max_range_long;
    }
    else if (maxover1p == 0) {
        cod_info->quant.scalefac_scale = 1;
        cod_info->quant.preflag = 1;
    }
    cod_info->quant.global_gain = vbrmax;
    if (cod_info->quant.global_gain < 0) {
        cod_info->quant.global_gain = 0;
    }
    else if (cod_info->quant.global_gain > GLOBAL_GAIN_MAX) {
        cod_info->quant.global_gain = GLOBAL_GAIN_MAX;
    }
    {
        int     sf_temp[SFBMAX];
        for (sfb = 0; sfb < SFBMAX; ++sfb) {
            sf_temp[sfb] = vbrsf[sfb] - vbrmax;
        }
        set_scalefacs(cod_info, vbrsfmin, sf_temp, max_rangep);
    }
    assert(checkScalefactor(cod_info, vbrsfmin));
}



static void
bitcount(const algo_t * that)
{
    int     rc = scale_bitcount(that->gfc, that->cod_info);

    if (rc == 0) {
        return;
    }
    /*  this should not happen due to the way the scalefactors are selected  */
    ERRORF(that->gfc, "INTERNAL ERROR IN VBR NEW CODE (986), please send bug report\n");
    that->gfc->sv_enc.internal_error = 1;
}



static int
quantizeAndCountBits(const algo_t * that)
{
    quantize_x34(that);
    that->cod_info->huffman.part2_3_length = noquant_count_bits(that->gfc, that->cod_info, 0);
    return that->cod_info->huffman.part2_3_length;
}





static int
tryThatOne(algo_t const* that, const int sftemp[SFBMAX], const int vbrsfmin[SFBMAX], int vbrmax)
{
    FLOAT const xrpow_max = that->cod_info->quant.xrpow_max;
    int     nbits = LARGE_BITS;
    that->alloc(that, sftemp, vbrsfmin, vbrmax);
    bitcount(that);
    nbits = quantizeAndCountBits(that);
    nbits += that->cod_info->quant.part2_length;
    that->cod_info->quant.xrpow_max = xrpow_max;
    return nbits;
}


static int
tryGlobalStepsize(const algo_t * that, const int sfwork[SFBMAX],
                  const int vbrsfmin[SFBMAX], int delta)
{
    int     sftemp[SFBMAX], i;
    int     gain, vbrmax = 0;
    for (i = 0; i < SFBMAX; ++i) {
        gain = sfwork[i] + delta;
        if (gain < vbrsfmin[i]) {
            gain = vbrsfmin[i];
        }
        if (gain > GLOBAL_GAIN_MAX) {
            gain = GLOBAL_GAIN_MAX;
        }
        if (vbrmax < gain) {
            vbrmax = gain;
        }
        sftemp[i] = gain;
    }
    return tryThatOne(that, sftemp, vbrsfmin, vbrmax) - that->cod_info->quant.part2_length;
}



static void
searchGlobalStepsizeMax(const algo_t * that, const int sfwork[SFBMAX],
                        const int vbrsfmin[SFBMAX], int target)
{
    gr_info const *const cod_info = that->cod_info;
    const int gain = cod_info->quant.global_gain;
    int     curr = gain;
    int     gain_ok = 1024;
    int     nbits = LARGE_BITS;
    int     l = gain, r = 512;

    assert(gain >= 0);
    while (l <= r) {
        curr = (l + r) >> 1;
        nbits = tryGlobalStepsize(that, sfwork, vbrsfmin, curr - gain);
        if (nbits == 0 || (nbits + cod_info->quant.part2_length) < target) {
            r = curr - 1;
            gain_ok = curr;
        }
        else {
            l = curr + 1;
            if (gain_ok == 1024) {
                gain_ok = curr;
            }
        }
    }
    if (gain_ok != curr) {
        curr = gain_ok;
        nbits = tryGlobalStepsize(that, sfwork, vbrsfmin, curr - gain);
    }
}



static int
sfDepth(const int sfwork[SFBMAX])
{
    int     m = 0;
    unsigned int i, j;
    for (j = SFBMAX, i = 0; j > 0; --j, ++i) {
        int const di = GLOBAL_GAIN_MAX - sfwork[i];
        if (m < di) {
            m = di;
        }
        assert(sfwork[i] >= 0);
        assert(sfwork[i] <= GLOBAL_GAIN_MAX);
    }
    assert(m >= 0);
    assert(m <= GLOBAL_GAIN_MAX);
    return m;
}


static void
cutDistribution(const int sfwork[SFBMAX], int sf_out[SFBMAX], int cut)
{
    unsigned int i, j;
    for (j = SFBMAX, i = 0; j > 0; --j, ++i) {
        int const x = sfwork[i];
        sf_out[i] = x < cut ? x : cut;
    }
}


static int
flattenDistribution(const int sfwork[SFBMAX], const int vbrsfmin[SFBMAX],
                    int sf_out[SFBMAX], int dm, int k, int p)
{
    unsigned int i, j;
    int     x, sfmax = 0;
    if (dm > 0) {
        for (j = SFBMAX, i = 0; j > 0; --j, ++i) {
            int const di = p - sfwork[i];
            x = sfwork[i] + (k * di) / dm;
            /* the scalefactor may not drop below the band's minimum, or the
               allocator cannot represent it; same floor as tryGlobalStepsize */
            if (x < vbrsfmin[i]) {
                x = vbrsfmin[i];
            }
            else {
                if (x > GLOBAL_GAIN_MAX) {
                    x = GLOBAL_GAIN_MAX;
                }
            }
            sf_out[i] = x;
            if (sfmax < x) {
                sfmax = x;
            }
        }
    }
    else {
        for (j = SFBMAX, i = 0; j > 0u; --j, ++i) {
            x = sfwork[i];
            sf_out[i] = x;
            if (sfmax < x) {
                sfmax = x;
            }
        }
    }
    return sfmax;
}


/** \internal \brief The argument of flattenDistribution() that bisect_flatten() varies. */
typedef enum {
    FLATTEN_DEPTH,              /**< the cut depth k, at the global gain of the granule */
    FLATTEN_GAIN                /**< the global gain p, at the full depth */
} flatten_arg_t;

/**
 * \internal
 * \brief Finds by bisection the smallest value of one argument of
 *        flattenDistribution() whose result fits the target, and leaves the
 *        granule quantized with it.
 * \param that      the quantizer state.
 * \param sfwork    the scalefactors.
 * \param vbrsfmin  the smallest scalefactor of each band.
 * \param wrk       work space for the flattened scalefactors.
 * \param dm        the depth of the distribution.
 * \param p         the global gain of the granule.
 * \param arg       the argument that the search varies.
 * \param lo        the smallest value to try.
 * \param hi        the largest value to try.
 * \param target    the number of bits that the granule may use.
 * \return 1 if a value fits, 0 if none does.
 */
static int
bisect_flatten(algo_t const *that, const int sfwork[SFBMAX], const int vbrsfmin[SFBMAX],
               int wrk[SFBMAX], int dm, int p, flatten_arg_t arg, int lo, int hi, int target)
{
    int     bi = (lo + hi) / 2;
    int     bi_ok = -1;
    int     bu = lo;
    int     bo = hi;

    for (;;) {
        int const sfmax = arg == FLATTEN_DEPTH
            ? flattenDistribution(sfwork, vbrsfmin, wrk, dm, bi, p)
            : flattenDistribution(sfwork, vbrsfmin, wrk, dm, dm, bi);
        int const nbits = tryThatOne(that, wrk, vbrsfmin, sfmax);

        if (nbits <= target) {
            bi_ok = bi;
            bo = bi - 1;
        }
        else {
            bu = bi + 1;
        }
        if (bu <= bo) {
            bi = (bu + bo) / 2;
        }
        else {
            break;
        }
    }
    if (bi_ok < 0) {
        return 0;
    }
    if (bi != bi_ok) {
        int const sfmax = arg == FLATTEN_DEPTH
            ? flattenDistribution(sfwork, vbrsfmin, wrk, dm, bi_ok, p)
            : flattenDistribution(sfwork, vbrsfmin, wrk, dm, dm, bi_ok);
        (void) tryThatOne(that, wrk, vbrsfmin, sfmax);
    }
    return 1;
}


static void
outOfBitsStrategy(algo_t const* that, const int sfwork[SFBMAX], const int vbrsfmin[SFBMAX], int target)
{
    int     wrk[SFBMAX];
    int const dm = sfDepth(sfwork);
    int const p = that->cod_info->quant.global_gain;

    /* First flatten the distribution, then raise the global gain. */
    if (bisect_flatten(that, sfwork, vbrsfmin, wrk, dm, p, FLATTEN_DEPTH, 0, dm, target)) {
        return;
    }
    if (bisect_flatten(that, sfwork, vbrsfmin, wrk, dm, p, FLATTEN_GAIN, p, GLOBAL_GAIN_MAX, target)) {
        return;
    }

    /* fall back to old code, likely to be never called */
    searchGlobalStepsizeMax(that, wrk, vbrsfmin, target);
}


/**
 * \internal
 * \brief Splits a bit budget in proportion to weights.
 * \param out    receives the share of each part.
 * \param f      the weight of each part.
 * \param s      the sum of the weights. All shares are 0 when it is not above 0.
 * \param n      the number of parts, 1 or 2.
 * \param total  the budget.
 */
static void
split_in_proportion(int out[2], float const f[2], float s, int n, int total)
{
    int     i;

    for (i = 0; i < n; ++i) {
        if (s > 0) {
            out[i] = total * f[i] / s;
        }
        else {
            out[i] = 0;
        }
    }
}

/**
 * \internal
 * \brief Gives a part's budget above its use plus a slack to the other part.
 *
 * The first part gives first; then the second gives to the first.
 *
 * \param m      the budgets of the two parts.
 * \param use    the bits that each part uses.
 * \param slack  the bits that a part keeps above its use.
 */
static void
rebalance_pair(int m[2], int const use[2], int slack)
{
    if (m[0] > use[0] + slack) {
        m[1] += m[0];
        m[1] -= use[0] + slack;
        m[0] = use[0] + slack;
    }
    if (m[1] > use[1] + slack) {
        m[0] += m[1];
        m[0] -= use[1] + slack;
        m[1] = use[1] + slack;
    }
}


static int
reduce_bit_usage(lame_internal_flags * gfc, int gr, int ch)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    gr_info *const cod_info = &gfc->l3_side.tt[gr][ch];
    /*  try some better scalefac storage
     */
    best_scalefac_store(gfc, gr, ch, &gfc->l3_side);

    /*  best huffman_divide may save some bits too
     */
    if (cfg->use_best_huffman == 1)
        best_huffman_divide(gfc, cod_info);
    return cod_info->huffman.part2_3_length + cod_info->quant.part2_length;
}




/**
 * \internal
 * \brief The bit budgets of one frame of the VBR new encoder, and the bits used.
 */
typedef struct {
    int     max_ch[2][2]; /**< the most bits per granule and channel */
    int     max_gr[2];    /**< the most bits per granule */
    int     max_fr;       /**< the most bits of the frame */
    int     use_ch[2][2]; /**< the bits used per granule and channel */
    int     use_gr[2];    /**< the bits used per granule */
    int     use_fr;       /**< the bits the frame uses */
} vbr_budget_t;


/**
 * \internal
 * \brief Sets up the search of each granule and channel, and the budgets.
 *
 * \param gfc       the encoder.
 * \param that_     receives the search set-up per granule and channel.
 * \param xr34orig  the spectrum, raised to the power 3/4.
 * \param max_bits  the most bits per granule and channel.
 * \param bud       receives the budgets.
 */
static void
setup_granules(lame_internal_flags * gfc, algo_t that_[2][2], const FLOAT xr34orig[2][2][SAMPLES_PER_GRANULE],
               const int max_bits[2][2], vbr_budget_t * bud)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    int const ngr = cfg->mode_gr;
    int const nch = cfg->channels_out;
    int     gr, ch;

    /* set up some encoding parameters
     */
    for (gr = 0; gr < ngr; ++gr) {
        bud->max_gr[gr] = 0;
        for (ch = 0; ch < nch; ++ch) {
            bud->max_ch[gr][ch] = max_bits[gr][ch];
            bud->use_ch[gr][ch] = 0;
            bud->max_gr[gr] += max_bits[gr][ch];
            bud->max_fr += max_bits[gr][ch];
            that_[gr][ch].find = (cfg->full_outer_loop < 0) ? guess_scalefac_x34 : find_scalefac_x34;
            that_[gr][ch].gfc = gfc;
            that_[gr][ch].cod_info = &gfc->l3_side.tt[gr][ch];
            that_[gr][ch].xr34orig = xr34orig[gr][ch];
            if (that_[gr][ch].cod_info->granule.block_type == SHORT_TYPE) {
                that_[gr][ch].alloc = short_block_constrain;
            }
            else {
                that_[gr][ch].alloc = long_block_constrain;
            }
        }               /* for ch */
    }
}


/**
 * \internal
 * \brief Finds the scalefactors of each granule and channel that has bits.
 *
 * \param that_      the search set-up per granule and channel.
 * \param l3_xmin    the allowed distortion per band.
 * \param max_bits   the most bits per granule and channel.
 * \param sfwork_    receives the scalefactors.
 * \param vbrsfmin_  receives the smallest scalefactors.
 * \param ngr        the granules of the frame.
 * \param nch        the channels.
 */
static void
search_scalefactors(algo_t that_[2][2], const FLOAT l3_xmin[2][2][SFBMAX], const int max_bits[2][2],
                    int sfwork_[2][2][SFBMAX], int vbrsfmin_[2][2][SFBMAX], int ngr, int nch)
{
    int     gr, ch;

    /* searches scalefactors
     */
    for (gr = 0; gr < ngr; ++gr) {
        for (ch = 0; ch < nch; ++ch) {
            if (max_bits[gr][ch] > 0) {
                algo_t *that = &that_[gr][ch];
                int    *sfwork = sfwork_[gr][ch];
                int    *vbrsfmin = vbrsfmin_[gr][ch];
                int     vbrmax;

                vbrmax = block_sf(that, l3_xmin[gr][ch], sfwork, vbrsfmin);
                that->alloc(that, sfwork, vbrsfmin, vbrmax);
                bitcount(that);
            }
            else {
                /*  xr contains no energy 
                 *  l3_enc, our encoding data, will be quantized to zero
                 *  continue with next channel
                 */
            }
        }               /* for ch */
    }
}


/**
 * \internal
 * \brief Quantizes the frame with the scalefactors found, and counts the bits.
 *
 * \param gfc       the encoder.
 * \param that_     the search set-up per granule and channel.
 * \param max_bits  the most bits per granule and channel.
 * \param bud       receives the bits used.
 */
static void
quantize_as_is(lame_internal_flags * gfc, algo_t that_[2][2], const int max_bits[2][2],
               vbr_budget_t * bud)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    int const ngr = cfg->mode_gr;
    int const nch = cfg->channels_out;
    int     gr, ch;

    /* encode 'as is'
     */
    bud->use_fr = 0;
    for (gr = 0; gr < ngr; ++gr) {
        bud->use_gr[gr] = 0;
        for (ch = 0; ch < nch; ++ch) {
            algo_t const *that = &that_[gr][ch];
            if (max_bits[gr][ch] > 0) {
                memset(&that->cod_info->quant.l3_enc[0], 0,
                       sizeof(that->cod_info->quant.l3_enc));
                (void) quantizeAndCountBits(that);
            }
            else {
                /*  xr contains no energy 
                 *  l3_enc, our encoding data, will be quantized to zero
                 *  continue with next channel
                 */
            }
            bud->use_ch[gr][ch] = reduce_bit_usage(gfc, gr, ch);
            bud->use_gr[gr] += bud->use_ch[gr][ch];
        }               /* for ch */
        bud->use_fr += bud->use_gr[gr];
    }
}


/**
 * \internal
 * \brief Tells whether every granule and channel keeps to its limit.
 *
 * No granule may use more than MAX_BITS_PER_GRANULE bits and no channel more
 * than MAX_BITS_PER_CHANNEL.
 *
 * \param bud  the bits used.
 * \param ngr  the granules of the frame.
 * \param nch  the channels.
 * \return 1 if they do, else 0.
 */
static int
fits_limits(vbr_budget_t const *bud, int ngr, int nch)
{
    int     gr, ch;
    int     ok = 1;

    for (gr = 0; gr < ngr; ++gr) {
        if (bud->use_gr[gr] > MAX_BITS_PER_GRANULE) {
            ok = 0;
        }
        for (ch = 0; ch < nch; ++ch) {
            if (bud->use_ch[gr][ch] > MAX_BITS_PER_CHANNEL) {
                /* part2_3_length has 12 bits, so at most 4095 */
                ok = 0;
            }
        }
    }
    return ok;
}


/**
 * \internal
 * \brief Shares the bits out again when the frame broke a limit.
 *
 * Each granule is capped at MAX_BITS_PER_GRANULE, and the frame at its
 * budget, in proportion to the bits each part used.
 *
 * \param bud  the budgets and the bits used; the budgets change.
 * \param ngr  the granules of the frame.
 * \param nch  the channels.
 */
static void
plan_budgets(vbr_budget_t * bud, int ngr, int nch)
{
    int     gr, ch;
    int     sum_fr;

    sum_fr = 0;

    for (gr = 0; gr < ngr; ++gr) {
        bud->max_gr[gr] = 0;
        for (ch = 0; ch < nch; ++ch) {
            if (bud->use_ch[gr][ch] > MAX_BITS_PER_CHANNEL) {
                bud->max_ch[gr][ch] = MAX_BITS_PER_CHANNEL;
            }
            else {
                bud->max_ch[gr][ch] = bud->use_ch[gr][ch];
            }
            bud->max_gr[gr] += bud->max_ch[gr][ch];
        }
        if (bud->max_gr[gr] > MAX_BITS_PER_GRANULE) {
            float   f[2] = {0.0f, 0.0f}, s = 0.0f;
            for (ch = 0; ch < nch; ++ch) {
                if (bud->max_ch[gr][ch] > 0) {
                    f[ch] = sqrt(sqrt(bud->max_ch[gr][ch]));
                    s += f[ch];
                }
                else {
                    f[ch] = 0;
                }
            }
            split_in_proportion(bud->max_ch[gr], f, s, nch, MAX_BITS_PER_GRANULE);
            if (nch > 1) {
                rebalance_pair(bud->max_ch[gr], bud->use_ch[gr], 32);
                if (bud->max_ch[gr][0] > MAX_BITS_PER_CHANNEL) {
                    bud->max_ch[gr][0] = MAX_BITS_PER_CHANNEL;
                }
                if (bud->max_ch[gr][1] > MAX_BITS_PER_CHANNEL) {
                    bud->max_ch[gr][1] = MAX_BITS_PER_CHANNEL;
                }
            }
            bud->max_gr[gr] = 0;
            for (ch = 0; ch < nch; ++ch) {
                bud->max_gr[gr] += bud->max_ch[gr][ch];
            }
        }
        sum_fr += bud->max_gr[gr];
    }
    if (sum_fr > bud->max_fr) {
        {
            float   f[2] = {0.0f, 0.0f}, s = 0.0f;
            for (gr = 0; gr < ngr; ++gr) {
                if (bud->max_gr[gr] > 0) {
                    f[gr] = sqrt(bud->max_gr[gr]);
                    s += f[gr];
                }
                else {
                    f[gr] = 0;
                }
            }
            split_in_proportion(bud->max_gr, f, s, ngr, bud->max_fr);
        }
        if (ngr > 1) {
            rebalance_pair(bud->max_gr, bud->use_gr, 125);
            for (gr = 0; gr < ngr; ++gr) {
                if (bud->max_gr[gr] > MAX_BITS_PER_GRANULE) {
                    bud->max_gr[gr] = MAX_BITS_PER_GRANULE;
                }
            }
        }
        for (gr = 0; gr < ngr; ++gr) {
            float   f[2] = {0.0f, 0.0f}, s = 0.0f;
            for (ch = 0; ch < nch; ++ch) {
                if (bud->max_ch[gr][ch] > 0) {
                    f[ch] = sqrt(bud->max_ch[gr][ch]);
                    s += f[ch];
                }
                else {
                    f[ch] = 0;
                }
            }
            split_in_proportion(bud->max_ch[gr], f, s, nch, bud->max_gr[gr]);
            if (nch > 1) {
                rebalance_pair(bud->max_ch[gr], bud->use_ch[gr], 32);
                for (ch = 0; ch < nch; ++ch) {
                    if (bud->max_ch[gr][ch] > MAX_BITS_PER_CHANNEL) {
                        bud->max_ch[gr][ch] = MAX_BITS_PER_CHANNEL;
                    }
                }
            }
        }
    }
}


/**
 * \internal
 * \brief Tells whether the budgets keep to every limit.
 *
 * \param bud  the budgets.
 * \param ngr  the granules of the frame.
 * \param nch  the channels.
 * \return 1 if they do, else 0.
 */
static int
budgets_are_valid(vbr_budget_t const *bud, int ngr, int nch)
{
    int     gr, ch;
    int     ok = 1;
    int     sum_fr;

    /* sanity check */
    sum_fr = 0;
    for (gr = 0; gr < ngr; ++gr) {
        int     sum_gr = 0;
        for (ch = 0; ch < nch; ++ch) {
            sum_gr += bud->max_ch[gr][ch];
            if (bud->max_ch[gr][ch] > MAX_BITS_PER_CHANNEL) {
                ok = 0;
            }
        }
        sum_fr += sum_gr;
        if (sum_gr > MAX_BITS_PER_GRANULE) {
            ok = 0;
        }
    }
    if (sum_fr > bud->max_fr) {
        ok = 0;
    }
    return ok;
}


/**
 * \internal
 * \brief Clears what the first scalefactor storage set, before it runs again.
 *
 * \param l3_side  the side information of the frame.
 * \param ngr      the granules of the frame.
 * \param nch      the channels.
 */
static void
reset_scalefac_storage(III_side_info_t * l3_side, int ngr, int nch)
{
    int     gr, ch;

    /* we already called the 'best_scalefac_store' function, so we need to reset some
     * variables before we can do it again.
     */
    for (ch = 0; ch < nch; ++ch) {
        l3_side->scfsi[ch][0] = 0;
        l3_side->scfsi[ch][1] = 0;
        l3_side->scfsi[ch][2] = 0;
        l3_side->scfsi[ch][3] = 0;
    }
    for (gr = 0; gr < ngr; ++gr) {
        for (ch = 0; ch < nch; ++ch) {
            l3_side->tt[gr][ch].quant.scalefac_compress = 0;
        }
    }
}


/**
 * \internal
 * \brief Quantizes the frame again to fit the budgets, and counts the bits.
 *
 * \param gfc        the encoder.
 * \param that_      the search set-up per granule and channel.
 * \param sfwork_    the scalefactors; they are cut to fit.
 * \param vbrsfmin_  the smallest scalefactors.
 * \param max_bits   the most bits per granule and channel.
 * \param bud        the budgets; receives the bits used.
 */
static void
requantize_to_budget(lame_internal_flags * gfc, algo_t that_[2][2], int sfwork_[2][2][SFBMAX],
                     int vbrsfmin_[2][2][SFBMAX], const int max_bits[2][2], vbr_budget_t * bud)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    int const ngr = cfg->mode_gr;
    int const nch = cfg->channels_out;
    int     gr, ch;

    /* alter our encoded data, until it fits into the target bitrate
     */
    bud->use_fr = 0;
    for (gr = 0; gr < ngr; ++gr) {
        bud->use_gr[gr] = 0;
        for (ch = 0; ch < nch; ++ch) {
            algo_t const *that = &that_[gr][ch];
            bud->use_ch[gr][ch] = 0;
            if (max_bits[gr][ch] > 0) {
                int    *sfwork = sfwork_[gr][ch];
                int const *vbrsfmin = vbrsfmin_[gr][ch];
                cutDistribution(sfwork, sfwork,
                                that->cod_info->quant.global_gain);
                outOfBitsStrategy(that, sfwork, vbrsfmin, bud->max_ch[gr][ch]);
            }
            bud->use_ch[gr][ch] = reduce_bit_usage(gfc, gr, ch);
            assert(bud->use_ch[gr][ch] <= bud->max_ch[gr][ch]);
            bud->use_gr[gr] += bud->use_ch[gr][ch];
        }               /* for ch */
        bud->use_fr += bud->use_gr[gr];
    }
}


int
VBR_encode_frame(lame_internal_flags * gfc, const FLOAT xr34orig[2][2][SAMPLES_PER_GRANULE],
                 const FLOAT l3_xmin[2][2][SFBMAX], const int max_bits[2][2])
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    int     sfwork_[2][2][SFBMAX];
    int     vbrsfmin_[2][2][SFBMAX];
    algo_t  that_[2][2];
    int const ngr = cfg->mode_gr;
    int const nch = cfg->channels_out;
    vbr_budget_t bud = {
        {{0, 0}, {0, 0}}, {0, 0}, 0,
        {{MAX_BITS_PER_CHANNEL + 1, MAX_BITS_PER_CHANNEL + 1},
         {MAX_BITS_PER_CHANNEL + 1, MAX_BITS_PER_CHANNEL + 1}},
        {MAX_BITS_PER_GRANULE + 1, MAX_BITS_PER_GRANULE + 1},
        MAX_BITS_PER_GRANULE + MAX_BITS_PER_GRANULE
    };
    int     gr, ch;

    setup_granules(gfc, that_, xr34orig, max_bits, &bud);
    search_scalefactors(that_, l3_xmin, max_bits, sfwork_, vbrsfmin_, ngr, nch);
    quantize_as_is(gfc, that_, max_bits, &bud);

    if (bud.use_fr <= bud.max_fr && fits_limits(&bud, ngr, nch)) {
        return bud.use_fr;
    }

    /* OK, we are in trouble and have to define how many bits are
     * to be used for each granule
     */
    plan_budgets(&bud, ngr, nch);
    if (!budgets_are_valid(&bud, ngr, nch)) {
        /* we must have done something wrong, fallback to 'on_pe' based constrain */
        for (gr = 0; gr < ngr; ++gr) {
            for (ch = 0; ch < nch; ++ch) {
                bud.max_ch[gr][ch] = max_bits[gr][ch];
            }
        }
    }

    reset_scalefac_storage(&gfc->l3_side, ngr, nch);
    requantize_to_budget(gfc, that_, sfwork_, vbrsfmin_, max_bits, &bud);

    /* check bit constrains, but it should always be ok, iff there are no bugs ;-)
     */
    if (bud.use_fr <= bud.max_fr) {
        return bud.use_fr;
    }

    ERRORF(gfc, "INTERNAL ERROR IN VBR NEW CODE (1313), please send bug report\n"
           "maxbits=%d usedbits=%d\n", bud.max_fr, bud.use_fr);
    gfc->sv_enc.internal_error = 1;
    return bud.use_fr;
}
