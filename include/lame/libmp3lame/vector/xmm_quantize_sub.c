/*
 * MP3 quantization, intrinsics functions
 *
 *      Copyright (c) 2005-2006 Gabriel Bouvigne
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
 * \brief The SSE2 forms of init_xrpow_core() and of the FFT (fht_SSE2()).
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "lame_intrin.h"
#include "fft_pvt.h"



#ifdef HAVE_SSE2_INTRINSICS

#include <xmmintrin.h>

typedef union {
    int32_t _i_32[4]; /* unions are initialized by its first member */
    float   _float[4];
    __m128  _m128;
} vecfloat_union;



/* Coefficients below 2^-80 are set to zero before the roots: from 2^-84 down,
   x * sqrt(x) is a denormal, and a square root evaluated from a reciprocal
   estimate, as a compiler may do under -ffast-math, makes a NaN of a denormal;
   the margin keeps the estimate's error away from that edge. They quantize to
   zero at every step size. */
#define XRPOW_SMALLEST_ROOTED 8.271806125530277e-25f

/**
 * \internal
 * \brief The xrpow values of four coefficients: |xr|^(3/4), and 0 for a
 *        coefficient below #XRPOW_SMALLEST_ROOTED.
 * \param x          the four coefficients.
 * \param fabs_mask  the mask that clears the sign bit.
 * \param smallest   #XRPOW_SMALLEST_ROOTED in every lane.
 * \param sum        adds |xr| of each lane.
 * \param max        keeps the largest xrpow value of each lane.
 * \return the four xrpow values.
 */
static inline SSE_FUNCTION __m128
xrpow_block(__m128 x, __m128 fabs_mask, __m128 smallest, __m128 * sum, __m128 * max)
{
    x = _mm_and_ps(x, fabs_mask);
    *sum = _mm_add_ps(*sum, x);
    x = _mm_and_ps(x, _mm_cmpge_ps(x, smallest));
    x = _mm_sqrt_ps(_mm_mul_ps(x, _mm_sqrt_ps(x)));
    *max = _mm_max_ps(*max, x);
    return x;
}

SSE_FUNCTION void
init_xrpow_core_sse(gr_info * const cod_info, FLOAT xrpow[SAMPLES_PER_GRANULE], int max_nz, FLOAT * sum)
{
    int     i;
    float   tmp_max = 0;
    float   tmp_sum = 0;
    int     upper = max_nz + 1;
    int     upper4 = (upper / 4) * 4;
    int     rest = upper-upper4;

    const vecfloat_union fabs_mask = {{ 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF }};
    const __m128 vec_fabs_mask = _mm_loadu_ps(&fabs_mask._float[0]);
    const __m128 vec_smallest = _mm_set_ps1(XRPOW_SMALLEST_ROOTED);
    vecfloat_union vec_xrpow_max;
    vecfloat_union vec_sum;
    vecfloat_union vec_tmp;

    _mm_prefetch((char const *) cod_info->granule.xr, _MM_HINT_T0);
    _mm_prefetch((char const *) xrpow, _MM_HINT_T0);

    vec_xrpow_max._m128 = _mm_set_ps1(0);
    vec_sum._m128 = _mm_set_ps1(0);

    for (i = 0; i < upper4; i += 4) {
        vec_tmp._m128 = _mm_loadu_ps(&(cod_info->granule.xr[i])); /* load */
        vec_tmp._m128 = xrpow_block(vec_tmp._m128, vec_fabs_mask, vec_smallest,
                                    &vec_sum._m128, &vec_xrpow_max._m128);
        _mm_storeu_ps(&(xrpow[i]), vec_tmp._m128); /* store into xrpow[] */
    }
    vec_tmp._m128 = _mm_set_ps1(0);
    switch (rest) {
        case 3: vec_tmp._float[2] = cod_info->granule.xr[upper4+2]; /* fall through */
        case 2: vec_tmp._float[1] = cod_info->granule.xr[upper4+1]; /* fall through */
        case 1: vec_tmp._float[0] = cod_info->granule.xr[upper4+0];
            vec_tmp._m128 = xrpow_block(vec_tmp._m128, vec_fabs_mask, vec_smallest,
                                        &vec_sum._m128, &vec_xrpow_max._m128);
            switch (rest) {
                case 3: xrpow[upper4+2] = vec_tmp._float[2]; /* fall through */
                case 2: xrpow[upper4+1] = vec_tmp._float[1]; /* fall through */
                case 1: xrpow[upper4+0] = vec_tmp._float[0];
                default:
                    break;
            }
        default:
            break;
    }
    tmp_sum = vec_sum._float[0] + vec_sum._float[1] + vec_sum._float[2] + vec_sum._float[3];
    {
        float ma = vec_xrpow_max._float[0] > vec_xrpow_max._float[1]
                ? vec_xrpow_max._float[0] : vec_xrpow_max._float[1];
        float mb = vec_xrpow_max._float[2] > vec_xrpow_max._float[3]
                ? vec_xrpow_max._float[2] : vec_xrpow_max._float[3];
        tmp_max = ma > mb ? ma : mb;
    }
    cod_info->quant.xrpow_max = tmp_max;
    *sum = tmp_sum;
}


SSE_FUNCTION static void
store4(__m128 v, float* f0, float* f1, float* f2, float* f3)
{
    vecfloat_union r;
    r._m128 = v;
    *f0 = r._float[0];
    *f1 = r._float[1];
    *f2 = r._float[2];
    *f3 = r._float[3];
}


SSE_FUNCTION void
fht_SSE2(FLOAT * fz, int n)
{
    const FLOAT *tri = costab;
    int     k4;
    FLOAT  *fi, *gi;
    FLOAT const *fn;

    n <<= 1;            /* callers pass half the transform length */
    fn = fz + n;
    k4 = 4;
    do {
        FLOAT   s1, c1;
        int     i, k1, k2, k3, kx;
        kx = k4 >> 1;
        k1 = k4;
        k2 = k4 << 1;
        k3 = k2 + k1;
        k4 = k2 << 1;
        fht_pass_head(fz, fn, k1, k2, k3, k4, kx);
        c1 = tri[0];
        s1 = tri[1];
        for (i = 1; i < kx; i++) {
            __m128 v_s2;
            __m128 v_c2;
            __m128 v_c1;
            __m128 v_s1;
            FLOAT   c2, s2, s1_2 = s1+s1;
            c2 = 1 - s1_2 * s1;
            s2 = s1_2 * c1;
            fi = fz + i;
            gi = fz + k1 - i;
            v_c1 = _mm_set_ps1(c1);
            v_s1 = _mm_set_ps1(s1);
            v_c2 = _mm_set_ps1(c2);
            v_s2 = _mm_set_ps1(s2);
            {
                static const vecfloat_union sign_mask = {{0x80000000,0,0,0}};
                v_c1 = _mm_xor_ps(sign_mask._m128, v_c1); /* v_c1 := {-c1, +c1, +c1, +c1} */
            }
            {
                static const vecfloat_union sign_mask = {{0,0x80000000,0,0}};
                v_s1 = _mm_xor_ps(sign_mask._m128, v_s1); /* v_s1 := {+s1, -s1, +s1, +s1} */
            }
            {
                static const vecfloat_union sign_mask = {{0,0,0x80000000,0x80000000}};
                v_c2 = _mm_xor_ps(sign_mask._m128, v_c2); /* v_c2 := {+c2, +c2, -c2, -c2} */
            }
            do {
                __m128 p, q, r;

                q = _mm_setr_ps(fi[k1], fi[k3], gi[k1], gi[k3]); /* Q := {fi_k1,fi_k3,gi_k1,gi_k3}*/
                p = _mm_mul_ps(v_s2, q);                         /* P := s2 * Q */
                q = _mm_mul_ps(v_c2, q);                         /* Q := c2 * Q */
                q = _mm_shuffle_ps(q, q, _MM_SHUFFLE(1,0,3,2));  /* Q := {-c2*gi_k1,-c2*gi_k3,c2*fi_k1,c2*fi_k3} */
                p = _mm_add_ps(p, q);
                
                r = _mm_setr_ps(gi[0], gi[k2], fi[0], fi[k2]);   /* R := {gi_0,gi_k2,fi_0,fi_k2} */
                q = _mm_sub_ps(r, p);                            /* Q := {gi_0-p0,gi_k2-p1,fi_0-p2,fi_k2-p3} */
                r = _mm_add_ps(r, p);                            /* R := {gi_0+p0,gi_k2+p1,fi_0+p2,fi_k2+p3} */
                p = _mm_shuffle_ps(q, r, _MM_SHUFFLE(2,0,2,0));  /* P := {q0,q2,r0,r2} */
                p = _mm_shuffle_ps(p, p, _MM_SHUFFLE(3,1,2,0));  /* P := {q0,r0,q2,r2} */
                q = _mm_shuffle_ps(q, r, _MM_SHUFFLE(3,1,3,1));  /* Q := {q1,q3,r1,r3} */
                r = _mm_mul_ps(v_c1, q);
                q = _mm_mul_ps(v_s1, q);
                q = _mm_shuffle_ps(q, q, _MM_SHUFFLE(0,1,2,3));  /* Q := {q3,q2,q1,q0} */
                q = _mm_add_ps(q, r);

                store4(_mm_sub_ps(p, q), &gi[k3], &gi[k2], &fi[k3], &fi[k2]);
                store4(_mm_add_ps(p, q), &gi[k1], &gi[ 0], &fi[k1], &fi[ 0]);

                gi += k4;
                fi += k4;
            } while (fi < fn);
            c2 = c1;
            c1 = c2 * tri[0] - s1 * tri[1];
            s1 = c2 * tri[1] + s1 * tri[0];
        }
        tri += 2;
    } while (k4 < n);
}

#endif	/* HAVE_SSE2_INTRINSICS */

