/*
 *      psymodel.c
 *
 *      Copyright (c) 1999-2000 Mark Taylor
 *      Copyright (c) 2001-2002 Naoki Shibata
 *      Copyright (c) 2000-2003 Takehiro Tominaga
 *      Copyright (c) 2000-2012 Robert Hegemann
 *      Copyright (c) 2000-2005 Gabriel Bouvigne
 *      Copyright (c) 2000-2005 Alexander Leidinger
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

/* $Id$ */


/**
 * \file
 * \internal
 * \brief Psychoacoustic model: per-band masking thresholds, block type and
 *        perceptual entropy.
 *
 * For each granule, the model computes how much quantization noise each
 * scalefactor band can contain before a listener hears it. The rest of the
 * encoder uses these thresholds. The quantization loops shape the noise to
 * them. The perceptual entropy controls the bit reservoir. The stereo mode
 * decision reads the thresholds of each channel.
 *
 * \par One entry point, every mode
 *
 * Despite its name, L3psycho_anal_vbr() is used for CBR, ABR and VBR.
 *
 * \par Results lag the input by one granule
 *
 * The block type of a granule is known only after the next granule is
 * analyzed, because an attack makes the granule before it a start block. So
 * the analysis runs on the current granule and returns the results of the
 * previous granule. Callers handle this delay.
 *
 * \par Two frequency partitionings
 *
 * The analysis uses a grid of at most CBANDS partition bands, each about
 * DELBARK bark wide on the Zwicker scale (freq2bark()). init_numline() decides
 * how many are used, from the sample rate. At the end,
 * convert_partition2scalefac() maps the results onto the scalefactor bands of
 * the bitstream, SBMAX_l long and SBMAX_s short. It splits a partition that
 * crosses a scalefactor band boundary between its two neighbors.
 *
 * \par The analysis filter bank is not the coding filter bank
 *
 * The thresholds come from a separate FFT, not from the MDCT that the encoder
 * quantizes: 1024 points with a Blackman window for long blocks, and 256
 * points with a Hann window for each of the three short sub-blocks
 * (init_fft()). This transform is not critically sampled. So it keeps the full
 * energy of the signal and gives a magnitude spectrum to measure tonality on.
 * The thresholds are scaled into the domain of the coder where they are used.
 *
 * \par What the model computes, in order
 *
 * -# Attack detection. vbrpsy_attack_detection() high-passes the input at
 *    fs/4, splits the granule into nine sub-blocks, and compares each with the
 *    one two positions earlier. A large enough ratio counts as an attack, and
 *    an attack selects short blocks.
 * -# Energy per partition band, from the FFT (calc_energy()).
 * -# Tonality, as the ratio of peak to average energy over three neighboring
 *    partitions (calc_mask_index()). Tonal content masks less than noise, so
 *    the index selects how far below the signal the threshold of the band is.
 * -# Spreading. Every band masks its neighbors through s3_func().
 *    vbrpsy_mask_add() combines the contributions, and this is not a plain
 *    power sum.
 * -# Pre-echo control. It keeps thresholds low where higher thresholds would
 *    let noise come before a transient. In the long path it compares with the
 *    two granules before (vbrpsy_compute_masking_l()). In the short path it
 *    compares with the sub-block before, depending on where the attack is in
 *    the granule. That code is in L3psycho_anal_vbr() itself, after the
 *    mapping to scalefactor bands, because only there is the attack position
 *    known.
 * -# Mid/side thresholds, in joint stereo only
 *    (vbrpsy_compute_MS_thresholds()).
 * -# Perceptual entropy, an estimate of how many bits the granule will need
 *    (pecalc_l(), pecalc_s()).
 *
 * \par The absolute threshold is separate
 *
 * Masking thresholds follow the signal. The absolute threshold of hearing is a
 * lower limit for them. Other code computes it: ATHformula(), with the
 * loudness adjustment in adjust_ATH().
 */




#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <float.h>

#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "psymodel.h"
#include "lame_global_flags.h"
#include "fft.h"
#include "lame-analysis.h"


#define NSFIRLEN 21

#ifdef M_LN10
#define  LN_TO_LOG10  (M_LN10/10)
#else
#define  LN_TO_LOG10  0.2302585093
#endif


/*
   L3psycho_anal.  Compute psycho acoustics.

   Data returned to the calling program must be delayed by one 
   granule. 

   This is done in two places.  
   If we do not need to know the blocktype, the copying
   can be done here at the top of the program: we copy the data for
   the last granule (computed during the last call) before it is
   overwritten with the new data.  It looks like this:
  
   0. static psymodel_data 
   1. calling_program_data = psymodel_data
   2. compute psymodel_data
    
   For data which needs to know the blocktype, the copying must be
   done at the end of this loop, and the old values must be saved:
   
   0. static psymodel_data_old 
   1. compute psymodel_data
   2. compute possible block type of this granule
   3. compute final block type of previous granule based on #2.
   4. calling_program_data = psymodel_data_old
   5. psymodel_data_old = psymodel_data
*/





/* psycho_loudness_approx
   jd - 2001 mar 12
in:  energy   - BLKSIZE/2 elements of frequency magnitudes ^ 2
     gfp      - uses out_samplerate, ATHtype (also needed for ATHformula)
returns: loudness^2 approximation, a positive value roughly tuned for a value
         of 1.0 for signals near clipping.
notes:   When calibrated, feeding this function binary white noise at sample
         values +32767 or -32768 should return values that approach 3.
         ATHformula is used to approximate an equal loudness curve.
future:  Data indicates that the shape of the equal loudness curve varies
         with intensity.  This function might be improved by using an equal
         loudness curve shaped for typical playback levels (instead of the
         ATH, that is shaped for the threshold).  A flexible realization might
         simply bend the existing ATH curve to achieve the desired shape.
         However, the potential gain may not be enough to justify an effort.
*/
/**
 * \brief Approximate the loudness of a granule as an equal-loudness-weighted
 *        power sum.
 *
 * The weights in \a eql_w come from the absolute threshold of hearing, which
 * approximates an equal-loudness contour. The scale is set so that a signal
 * near clipping returns about 1.0, and full-scale binary white noise (samples
 * at +32767 and -32768) returns almost 3.
 *
 * The result controls adjust_ATH(). This is the only place where the encoder
 * adapts to the level of the material.
 *
 * \todo The shape of an equal-loudness contour changes with the level, and the
 *       ATH has the shape at the threshold, not at a typical playback level. A
 *       curve with the shape at playback level would be more accurate. Nobody
 *       has measured whether this is worth the work.
 */
static  FLOAT
psycho_loudness_approx(FLOAT const *energy, FLOAT const *eql_w)
{
    int     i;
    FLOAT   loudness_power;

    loudness_power = 0.0;
    /* apply weights to power in freq. bands */
    for (i = 0; i < BLKSIZE / 2; ++i)
        loudness_power += energy[i] * eql_w[i];
    loudness_power *= VO_SCALE;

    return loudness_power;
}

/* mask_add optimization */
/* init the limit values used to avoid computing log in mask_add when it is not necessary */

/* For example, with i = 10*log10(m2/m1)/10*16         (= log10(m2/m1)*16)
 *
 * abs(i)>8 is equivalent (as i is an integer) to
 * abs(i)>=9
 * i>=9 || i<=-9
 * equivalent to (as i is the biggest integer smaller than log10(m2/m1)*16 
 * or the smallest integer bigger than log10(m2/m1)*16 depending on the sign of log10(m2/m1)*16)
 * log10(m2/m1)>=9/16 || log10(m2/m1)<=-9/16
 * exp10 is strictly increasing thus this is equivalent to
 * m2/m1 >= 10^(9/16) || m2/m1<=10^(-9/16) which are comparisons to constants
 */


#define I1LIMIT 8       /* as in if(i>8)  */
#define I2LIMIT 23      /* as in if(i>24) -> changed 23 */
#define MLIMIT  15      /* as in if(m<15) */

/* pow(10, (I1LIMIT + 1) / 16.0); */
static const FLOAT ma_max_i1 = 3.6517412725483771;
/* pow(10, (I2LIMIT + 1) / 16.0); */
static const FLOAT ma_max_i2 = 31.622776601683793;
/* pow(10, (MLIMIT) / 10.0); */
static const FLOAT ma_max_m  = 31.622776601683793;

    /*This is the masking table:
       According to tonality, values are going from 0dB (TMN)
       to 9.3dB (NMT).
       After additive masking computation, 8dB are added, so
       final values are going from 8dB to 17.3dB
     */
static const FLOAT tab[] = {
    1.0 /*pow(10, -0) */ ,
    0.79433 /*pow(10, -0.1) */ ,
    0.63096 /*pow(10, -0.2) */ ,
    0.63096 /*pow(10, -0.2) */ ,
    0.63096 /*pow(10, -0.2) */ ,
    0.63096 /*pow(10, -0.2) */ ,
    0.63096 /*pow(10, -0.2) */ ,
    0.25119 /*pow(10, -0.6) */ ,
    0.11749             /*pow(10, -0.93) */
};

static const int tab_mask_add_delta[] = { 2, 2, 2, 1, 1, 1, 0, 0, -1 };
#define STATIC_ASSERT_EQUAL_DIMENSION(A,B) enum{static_assert_##A=1/((dimension_of(A) == dimension_of(B))?1:0)}

/**
 * \brief Distance in partition bands within which vbrpsy_mask_add() combines
 *        two maskers non-linearly.
 *
 * The tonality index selects it: tonal partitions get a narrower window than
 * noise-like ones. About three partitions make one bark.
 */
inline static int
mask_add_delta(int i)
{
    STATIC_ASSERT_EQUAL_DIMENSION(tab_mask_add_delta,tab);
    assert(i < (int)dimension_of(tab));
    return tab_mask_add_delta[i];
}


/**
 * \brief Check that the precomputed limits of vbrpsy_mask_add() still equal
 *        the expressions they come from.
 *
 * Compiles to nothing under NDEBUG. The limits let the hot path compare
 * energies directly, without a logarithm.
 */
static void
init_mask_add_max_values(void)
{
#ifndef NDEBUG
    FLOAT const _ma_max_i1 = pow(10, (I1LIMIT + 1) / 16.0);
    FLOAT const _ma_max_i2 = pow(10, (I2LIMIT + 1) / 16.0);
    FLOAT const _ma_max_m = pow(10, (MLIMIT) / 10.0);
    assert(fabs(ma_max_i1 - _ma_max_i1) <= FLT_EPSILON);
    assert(fabs(ma_max_i2 - _ma_max_i2) <= FLT_EPSILON);
    assert(fabs(ma_max_m  - _ma_max_m ) <= FLT_EPSILON);
#endif
}




/* addition of simultaneous masking   Naoki Shibata 2000/7 */
/**
 * \brief Combine two masking contributions.
 *
 * Not a power sum. Within \a delta partitions and a moderate level ratio, the
 * two are added and the sum is increased. Further apart, or when one masker
 * is much stronger, the result is the plain sum or only the larger one.
 *
 * This matters because a plain sum over many neighboring bands overestimates
 * how much a spread-out signal masks.
 *
 * Naoki Shibata, 2000.
 */
inline static FLOAT
vbrpsy_mask_add(FLOAT m1, FLOAT m2, int b, int delta)
{
    static const FLOAT table2[] = {
        1.33352 * 1.33352, 1.35879 * 1.35879, 1.38454 * 1.38454, 1.39497 * 1.39497,
        1.40548 * 1.40548, 1.3537 * 1.3537, 1.30382 * 1.30382, 1.22321 * 1.22321,
        1.14758 * 1.14758,
        1
    };

    FLOAT   ratio;

    if (m1 < 0) {
        m1 = 0;
    }
    if (m2 < 0) {
        m2 = 0;
    }
    if (m1 <= 0) {
        return m2;
    }
    if (m2 <= 0) {
        return m1;
    }
    if (m2 > m1) {
        ratio = m2 / m1;
    }
    else {
        ratio = m1 / m2;
    }
    if (abs(b) <= delta) {       /* approximately, 1 bark = 3 partitions */
        /* originally 'if(i > 8)' */
        if (ratio >= ma_max_i1) {
            return m1 + m2;
        }
        else {
            int     i = (int) (FAST_LOG10_X(ratio, 16.0f));
            return (m1 + m2) * table2[i];
        }
    }
    if (ratio < ma_max_i2) {
        return m1 + m2;
    }
    if (m1 < m2) {
        m1 = m2;
    }
    return m1;
}


/* short block threshold calculation (part 2)

    partition band bo_s[sfb] is at the transition from scalefactor
    band sfb to the next one sfb+1; enn and thmm have to be split
    between them
*/
/**
 * \brief Map partition-band energies and thresholds onto scalefactor bands.
 *
 * The two grids do not match. The analysis uses partition bands of about
 * constant bark width. The bitstream quantizes in the scalefactor bands of
 * the standard. A partition that crosses a scalefactor band boundary is split
 * between the two, weighted by how much of it is on each side.
 */
static void
convert_partition2scalefac(PsyConst_CB2SB_t const *const gd, FLOAT const *eb, FLOAT const *thr,
                           FLOAT enn_out[], FLOAT thm_out[])
{
    FLOAT   enn, thmm;
    int     sb, b, n = gd->n_sb;
    enn = thmm = 0.0f;
    for (sb = b = 0; sb < n; ++b, ++sb) {
        int const bo_sb = gd->bo[sb];
        int const npart = gd->npart;
        int const b_lim = bo_sb < npart ? bo_sb : npart;
        while (b < b_lim) {
            assert(eb[b] >= 0); /* iff failed, it may indicate some index error elsewhere */
            assert(thr[b] >= 0);
            enn += eb[b];
            thmm += thr[b];
            b++;
        }
        if (b >= npart) {
            enn_out[sb] = enn;
            thm_out[sb] = thmm;
            ++sb;
            break;
        }
        assert(eb[b] >= 0); /* iff failed, it may indicate some index error elsewhere */
        assert(thr[b] >= 0);
        {
            /* at transition sfb -> sfb+1 */
            FLOAT const w_curr = gd->bo_weight[sb];
            FLOAT const w_next = 1.0f - w_curr;
            enn += w_curr * eb[b];
            thmm += w_curr * thr[b];
            enn_out[sb] = enn;
            thm_out[sb] = thmm;
            enn = w_next * eb[b];
            thmm = w_next * thr[b];
        }
    }
    /* zero initialize the rest */
    for (; sb < n; ++sb) {
        enn_out[sb] = 0;
        thm_out[sb] = 0;
    }
}

/**
 * \brief convert_partition2scalefac() for one short sub-block, storing into
 *        the per-channel short-block state.
 */
static void
convert_partition2scalefac_s(lame_internal_flags * gfc, FLOAT const *eb, FLOAT const *thr, int chn,
                             int sblock)
{
    PsyStateVar_t *const psv = &gfc->sv_psy;
    PsyConst_CB2SB_t const *const gds = &gfc->cd_psy->s;
    FLOAT   enn[SBMAX_s], thm[SBMAX_s];
    int     sb;
    convert_partition2scalefac(gds, eb, thr, enn, thm);
    for (sb = 0; sb < SBMAX_s; ++sb) {
        psv->en[chn].s[sb][sblock] = enn[sb];
        psv->thm[chn].s[sb][sblock] = thm[sb];
    }
}

/* longblock threshold calculation (part 2) */
/**
 * \brief convert_partition2scalefac() for a long block, storing into the
 *        per-channel long-block state.
 */
static void
convert_partition2scalefac_l(lame_internal_flags * gfc, FLOAT const *eb, FLOAT const *thr, int chn)
{
    PsyStateVar_t *const psv = &gfc->sv_psy;
    PsyConst_CB2SB_t const *const gdl = &gfc->cd_psy->l;
    FLOAT  *enn = &psv->en[chn].l[0];
    FLOAT  *thm = &psv->thm[chn].l[0];
    convert_partition2scalefac(gdl, eb, thr, enn, thm);
}

/**
 * \brief Derive short-block thresholds from a long-block analysis.
 *
 * Used where the short-block analysis did not run, but short-block thresholds
 * are still needed. The threshold is divided by a constant factor, and all
 * three sub-blocks get the same value. So it is a placeholder, not an
 * analysis: it cannot tell the sub-blocks apart.
 */
static void
convert_partition2scalefac_l_to_s(lame_internal_flags * gfc, FLOAT const *eb, FLOAT const *thr,
                                  int chn)
{
    PsyStateVar_t *const psv = &gfc->sv_psy;
    PsyConst_CB2SB_t const *const gds = &gfc->cd_psy->l_to_s;
    FLOAT   enn[SBMAX_s], thm[SBMAX_s];
    int     sb, sblock;
    convert_partition2scalefac(gds, eb, thr, enn, thm);
    for (sb = 0; sb < SBMAX_s; ++sb) {
        FLOAT const scale = 1. / 64.f;
        FLOAT const tmp_enn = enn[sb];
        FLOAT const tmp_thm = thm[sb] * scale;
        for (sblock = 0; sblock < 3; ++sblock) {
            psv->en[chn].s[sb][sblock] = tmp_enn;
            psv->thm[chn].s[sb][sblock] = tmp_thm;
        }
    }
}



/**
 * \brief Geometric interpolation, \a x raised to \a r times \a y raised to
 *        1 - \a r.
 *
 * An \a r of 1 or more returns \a x, and one of 0 or less returns \a y.
 */
static inline FLOAT
NS_INTERP(FLOAT x, FLOAT y, FLOAT r)
{
    /* was pow((x),(r))*pow((y),1-(r)) */
    if (r >= 1.0f)
        return x;
    if (r <= 0.0f)
        return y;
    if (y > 0.0f)
        return powf(x / y, r) * y;
    return 0.0f;        /* never happens */
}



/**
 * \brief Perceptual entropy of a short-block granule.
 *
 * Sums, over the bands where the energy is above the threshold, a weight per
 * band times the logarithm of the ratio of energy to threshold. The result
 * estimates how many bits the granule will need. It controls the bit
 * reservoir and the VBR bitrate.
 *
 * \note The band weights are tuned for 44.1 kHz only.
 */
static  FLOAT
pecalc_s(III_psy_ratio const *mr, FLOAT masking_lower)
{
    FLOAT   pe_s;
    static const FLOAT regcoef_s[] = {
        11.8,           /* these values are tuned only for 44.1kHz... */
        13.6,
        17.2,
        32,
        46.5,
        51.3,
        57.5,
        67.1,
        71.5,
        84.6,
        97.6,
        130,
/*      255.8 */
    };
    unsigned int sb, sblock;

    pe_s = 1236.28f / 4;
    for (sb = 0; sb < SBMAX_s - 1; sb++) {
        for (sblock = 0; sblock < 3; sblock++) {
            FLOAT const thm = mr->thm.s[sb][sblock];
            assert(sb < dimension_of(regcoef_s));
            if (thm > 0.0f) {
                FLOAT const x = thm * masking_lower;
                FLOAT const en = mr->en.s[sb][sblock];
                if (en > x) {
                    if (en > x * 1e10f) {
                        pe_s += regcoef_s[sb] * (10.0f * LOG10);
                    }
                    else {
                        assert(x > 0);
                        pe_s += regcoef_s[sb] * FAST_LOG10(en / x);
                    }
                }
            }
        }
    }

    return pe_s;
}

/**
 * \brief Perceptual entropy of a long-block granule.
 *
 * \see pecalc_s(), also for the sample rate that the band weights are tuned
 *      for.
 */
static  FLOAT
pecalc_l(III_psy_ratio const *mr, FLOAT masking_lower)
{
    FLOAT   pe_l;
    static const FLOAT regcoef_l[] = {
        6.8,            /* these values are tuned only for 44.1kHz... */
        5.8,
        5.8,
        6.4,
        6.5,
        9.9,
        12.1,
        14.4,
        15,
        18.9,
        21.6,
        26.9,
        34.2,
        40.2,
        46.8,
        56.5,
        60.7,
        73.9,
        85.7,
        93.4,
        126.1,
/*      241.3 */
    };
    unsigned int sb;

    pe_l = 1124.23f / 4;
    for (sb = 0; sb < SBMAX_l - 1; sb++) {
        FLOAT const thm = mr->thm.l[sb];
        assert(sb < dimension_of(regcoef_l));
        if (thm > 0.0f) {
            FLOAT const x = thm * masking_lower;
            FLOAT const en = mr->en.l[sb];
            if (en > x) {
                if (en > x * 1e10f) {
                    pe_l += regcoef_l[sb] * (10.0f * LOG10);
                }
                else {
                    assert(x > 0);
                    pe_l += regcoef_l[sb] * FAST_LOG10(en / x);
                }
            }
        }
    }

    return pe_l;
}


/**
 * \brief Total, peak and mean FFT line energy for each partition band.
 *
 * calc_mask_index() compares the peak and the mean to decide how tonal the
 * band is. The total is the strength of the masker.
 */
static void
calc_energy(PsyConst_CB2SB_t const *l, FLOAT const *fftenergy, FLOAT * eb, FLOAT * max, FLOAT * avg)
{
    int     b, j;

    for (b = j = 0; b < l->npart; ++b) {
        FLOAT   ebb = 0, m = 0;
        int     i;
        for (i = 0; i < l->numlines[b]; ++i, ++j) {
            FLOAT const el = fftenergy[j];
            assert(el >= 0);
            ebb += el;
            if (m < el)
                m = el;
        }
        eb[b] = ebb;
        max[b] = m;
        avg[b] = ebb * l->rnumlines[b];
        assert(l->rnumlines[b] >= 0);
        assert(ebb >= 0);
        assert(eb[b] >= 0);
        assert(max[b] >= 0);
        assert(avg[b] >= 0);
    }
}


/**
 * \internal
 * \brief Tonality index per partition band.
 *
 * Measured as the ratio of the peak line to the mean, over the band and its
 * two neighbors. A spectrum in few lines is tonal, an even spectrum is
 * noise-like. The index selects an entry in the table that decides how far
 * below the signal the threshold of the band is, because a tone masks noise
 * much less than noise masks a tone.
 *
 * \param gd        the partition layout of the block type, long or short.
 * \param max       the peak line energy of each partition.
 * \param avg       the mean line energy of each partition.
 * \param mask_idx  receives the index of each partition.
 */
static void
calc_mask_index(PsyConst_CB2SB_t const *gd, FLOAT const *max,
                FLOAT const *avg, unsigned char *mask_idx)
{
    FLOAT   m, a;
    int     b, k;
    int const last_tab_entry = dimension_of(tab) - 1;
    b = 0;
    a = avg[b] + avg[b + 1];
    assert(a >= 0);
    if (a > 0.0f) {
        m = max[b];
        if (m < max[b + 1])
            m = max[b + 1];
        assert((gd->numlines[b] + gd->numlines[b + 1] - 1) > 0);
        a = 20.0f * (m * 2.0f - a)
            / (a * (gd->numlines[b] + gd->numlines[b + 1] - 1));
        k = (int) a;
        if (k > last_tab_entry)
            k = last_tab_entry;
        mask_idx[b] = k;
    }
    else {
        mask_idx[b] = 0;
    }

    for (b = 1; b < gd->npart - 1; b++) {
        a = avg[b - 1] + avg[b] + avg[b + 1];
        assert(a >= 0);
        if (a > 0.0f) {
            m = max[b - 1];
            if (m < max[b])
                m = max[b];
            if (m < max[b + 1])
                m = max[b + 1];
            assert((gd->numlines[b - 1] + gd->numlines[b] + gd->numlines[b + 1] - 1) > 0);
            a = 20.0f * (m * 3.0f - a)
                / (a * (gd->numlines[b - 1] + gd->numlines[b] + gd->numlines[b + 1] - 1));
            k = (int) a;
            if (k > last_tab_entry)
                k = last_tab_entry;
            mask_idx[b] = k;
        }
        else {
            mask_idx[b] = 0;
        }
    }
    assert(b > 0);
    assert(b == gd->npart - 1);

    a = avg[b - 1] + avg[b];
    assert(a >= 0);
    if (a > 0.0f) {
        m = max[b - 1];
        if (m < max[b])
            m = max[b];
        assert((gd->numlines[b - 1] + gd->numlines[b] - 1) > 0);
        a = 20.0f * (m * 2.0f - a)
            / (a * (gd->numlines[b - 1] + gd->numlines[b] - 1));
        k = (int) a;
        if (k > last_tab_entry)
            k = last_tab_entry;
        mask_idx[b] = k;
    }
    else {
        mask_idx[b] = 0;
    }
    assert(b == (gd->npart - 1));
}


/**
 * \internal
 * \brief Turns the FFT of the left and the right channel into the FFT of the
 *        mid and the side channel, in place.
 * \param l  the left channel; receives the mid channel.
 * \param r  the right channel; receives the side channel.
 * \param n  the number of FFT values.
 */
static inline void
ms_from_lr(FLOAT * l, FLOAT * r, int n)
{
    FLOAT const sqrt2_half = SQRT2 * 0.5f;
    int     j;

    for (j = n - 1; j >= 0; --j) {
        FLOAT const lj = l[j];
        FLOAT const rj = r[j];
        l[j] = (lj + rj) * sqrt2_half;
        r[j] = (lj - rj) * sqrt2_half;
    }
}


/**
 * \internal
 * \brief The energy of each line of a real FFT.
 * \param w  the FFT in the layout of fht(): the real parts up to n / 2, the
 *           imaginary parts above.
 * \param e  receives n / 2 + 1 energies.
 * \param n  the FFT size.
 */
static inline void
power_spectrum(FLOAT const *w, FLOAT * e, int n)
{
    int     j;

    e[0] = w[0];
    e[0] *= e[0];
    for (j = n / 2 - 1; j >= 0; --j) {
        FLOAT const re = w[n / 2 - j];
        FLOAT const im = w[n / 2 + j];
        e[n / 2 - j] = (re * re + im * im) * 0.5f;
    }
}


/**
 * \brief Windowed long-block FFT and its per-line energy.
 */
static void
vbrpsy_compute_fft_l(lame_internal_flags * gfc, const sample_t * const buffer[2], int chn,
                     int gr_out, FLOAT fftenergy[HBLKSIZE], FLOAT(*wsamp_l)[BLKSIZE])
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    PsyStateVar_t *psv = &gfc->sv_psy;
    plotting_data *plt = cfg->analysis ? gfc->pinfo : 0;
    int     j;

    if (chn < 2) {
        fft_long(gfc, *wsamp_l, chn, buffer);
    }
    else if (chn == 2) {
        /* FFT data for mid and side channel is derived from L & R */
        ms_from_lr(wsamp_l[0], wsamp_l[1], BLKSIZE);
    }
    power_spectrum(*wsamp_l, fftenergy, BLKSIZE);
    /* total energy */
    {
        FLOAT   totalenergy = 0.0f;
        for (j = 11; j < HBLKSIZE; j++)
            totalenergy += fftenergy[j];

        psv->tot_ener[chn] = totalenergy;
    }

    if (plt) {
        for (j = 0; j < HBLKSIZE; j++) {
            plt->energy[gr_out][chn][j] = plt->energy_save[chn][j];
            plt->energy_save[chn][j] = fftenergy[j];
        }
    }
}


/**
 * \brief Windowed FFT and per-line energy for one short sub-block.
 */
static void
vbrpsy_compute_fft_s(lame_internal_flags const *gfc, const sample_t * const buffer[2], int chn,
                     int sblock, FLOAT(*fftenergy_s)[HBLKSIZE_s], FLOAT(*wsamp_s)[3][BLKSIZE_s])
{
    if (sblock == 0 && chn < 2) {
        fft_short(gfc, *wsamp_s, chn, buffer);
    }
    if (chn == 2) {
        /* FFT data for mid and side channel is derived from L & R */
        ms_from_lr(wsamp_s[0][sblock], wsamp_s[1][sblock], BLKSIZE_s);
    }
    power_spectrum((*wsamp_s)[sblock], fftenergy_s[sblock], BLKSIZE_s);
}


    /*********************************************************************
    * compute loudness approximation (used for ATH auto-level adjustment) 
    *********************************************************************/
/**
 * \brief Store the loudness of the granule for adjust_ATH(), when the
 *        absolute threshold adapts.
 */
static void
vbrpsy_compute_loudness_approximation_l(lame_internal_flags * gfc, int gr_out, int chn,
                                        const FLOAT fftenergy[HBLKSIZE])
{
    PsyStateVar_t *psv = &gfc->sv_psy;
    if (chn < 2) {      /*no loudness for mid/side ch */
        gfc->ov_psy.loudness_sq[gr_out][chn] = psv->loudness_sq_save[chn];
        psv->loudness_sq_save[chn] = psycho_loudness_approx(fftenergy, gfc->ATH->eql_w);
    }
}


    /**********************************************************************
    *  Apply HPF of fs/4 to the input signal.
    *  This is used for attack detection / handling.
    **********************************************************************/
/**
 * \internal
 * \brief Returns the maskings and energies of the previous granule to the
 *        caller.
 *
 * There is a one granule delay: the caller gets what the last call computed.
 *
 * \param psv               the model's state.
 * \param gr_out            the granule the caller encodes.
 * \param n_chn_out         the channels of the output.
 * \param n_chn_psy         the channels the model analyses, 4 in joint stereo.
 * \param masking_ratio     receives the left and right maskings.
 * \param masking_MS_ratio  receives the mid and side maskings, in joint stereo.
 * \param energy            receives the energy of each analysed channel.
 */
static void
vbrpsy_return_previous(PsyStateVar_t const *psv, int gr_out, int n_chn_out, int n_chn_psy,
                       III_psy_ratio masking_ratio[2][2], III_psy_ratio masking_MS_ratio[2][2],
                       FLOAT energy[4])
{
    int     chn;

    for (chn = 0; chn < n_chn_out; chn++) {
        masking_ratio[gr_out][chn].en = psv->en[chn];
        masking_ratio[gr_out][chn].thm = psv->thm[chn];
        if (n_chn_psy > 2) {
            /* MS maskings  */
            masking_MS_ratio[gr_out][chn].en = psv->en[chn + 2];
            masking_MS_ratio[gr_out][chn].thm = psv->thm[chn + 2];
        }
    }
    for (chn = 0; chn < n_chn_psy; chn++) {
        energy[chn] = psv->tot_ener[chn];
    }
}


/**
 * \brief Detect transients and choose long or short blocks.
 *
 * Quantization noise spreads over the whole synthesis window. So in a long
 * block, noise can come before a transient that would mask it. This is a
 * pre-echo. A short block has a shorter window, which is the only remedy the
 * bitstream format offers.
 *
 * The granule is high-pass filtered at a quarter of the sample rate, because a
 * transient shows most clearly where the signal is least tonal. Then it is
 * split into nine sub-blocks, and the peak amplitude of each is compared with
 * the sub-block two positions earlier. A ratio above the configured threshold
 * is an attack.
 *
 * Two refinements follow. A periodic signal would trigger the detector in
 * every period, so an attack is ignored where neighboring sub-blocks have
 * similar energy at a low level. And a sub-block whose energy comes in a
 * single spike gets an extra attenuation factor, which is applied later to
 * its threshold.
 *
 * This function also returns the maskings of the previous granule to the
 * caller. This is where the one-granule delay of the model happens.
 */
static void
vbrpsy_attack_detection(lame_internal_flags * gfc, const sample_t * const buffer[2], int gr_out,
                        III_psy_ratio masking_ratio[2][2], III_psy_ratio masking_MS_ratio[2][2],
                        FLOAT energy[4], FLOAT sub_short_factor[4][3], int ns_attacks[4][4],
                        int uselongblock[2])
{
    FLOAT   ns_hpfsmpl[2][SAMPLES_PER_GRANULE];
    SessionConfig_t const *const cfg = &gfc->cfg;
    PsyStateVar_t *const psv = &gfc->sv_psy;
    plotting_data *plt = cfg->analysis ? gfc->pinfo : 0;
    int const n_chn_out = cfg->channels_out;
    /* chn=2 and 3 = Mid and Side channels */
    int const n_chn_psy = (cfg->mode == JOINT_STEREO) ? 4 : n_chn_out;
    int     chn, i, j;

    vbrpsy_return_previous(psv, gr_out, n_chn_out, n_chn_psy, masking_ratio, masking_MS_ratio,
                           energy);
    memset(&ns_hpfsmpl[0][0], 0, sizeof(ns_hpfsmpl));
    /* Don't copy the input buffer into a temporary buffer */
    /* unroll the loop 2 times */
    for (chn = 0; chn < n_chn_out; chn++) {
        static const FLOAT fircoef[] = {
            -8.65163e-18 * 2, -0.00851586 * 2, -6.74764e-18 * 2, 0.0209036 * 2,
            -3.36639e-17 * 2, -0.0438162 * 2, -1.54175e-17 * 2, 0.0931738 * 2,
            -5.52212e-17 * 2, -0.313819 * 2
        };
        /* apply high pass filter of fs/4 */
        const sample_t *const firbuf = &buffer[chn][SAMPLES_PER_GRANULE - 350 - NSFIRLEN + 192];
        assert(dimension_of(fircoef) == ((NSFIRLEN - 1) / 2));
        for (i = 0; i < SAMPLES_PER_GRANULE; i++) {
            FLOAT   sum1, sum2;
            sum1 = firbuf[i + 10];
            sum2 = 0.0;
            for (j = 0; j < ((NSFIRLEN - 1) / 2) - 1; j += 2) {
                sum1 += fircoef[j] * (firbuf[i + j] + firbuf[i + NSFIRLEN - j]);
                sum2 += fircoef[j + 1] * (firbuf[i + j + 1] + firbuf[i + NSFIRLEN - j - 1]);
            }
            ns_hpfsmpl[chn][i] = sum1 + sum2;
        }
    }
    for (chn = 0; chn < n_chn_psy; chn++) {
        FLOAT   attack_intensity[12];
        FLOAT   en_subshort[12];
        FLOAT   en_short[4] = { 0, 0, 0, 0 };
        FLOAT const *pf = ns_hpfsmpl[chn & 1];
        int     ns_uselongblock = 1;

        if (chn == 2) {
            for (i = 0, j = SAMPLES_PER_GRANULE; j > 0; ++i, --j) {
                FLOAT const l = ns_hpfsmpl[0][i];
                FLOAT const r = ns_hpfsmpl[1][i];
                ns_hpfsmpl[0][i] = l + r;
                ns_hpfsmpl[1][i] = l - r;
            }
        }
        /*************************************************************** 
        * determine the block type (window type)
        ***************************************************************/
        /* calculate energies of each sub-shortblocks */
        for (i = 0; i < 3; i++) {
            en_subshort[i] = psv->last_en_subshort[chn][i + 6];
            assert(psv->last_en_subshort[chn][i + 4] > 0);
            attack_intensity[i] = en_subshort[i] / psv->last_en_subshort[chn][i + 4];
            en_short[0] += en_subshort[i];
        }

        for (i = 0; i < 9; i++) {
            FLOAT const *const pfe = pf + SAMPLES_PER_GRANULE / 9;
            FLOAT   p = 1.;
            for (; pf < pfe; pf++)
                if (p < fabs(*pf))
                    p = fabs(*pf);
            psv->last_en_subshort[chn][i] = en_subshort[i + 3] = p;
            en_short[1 + i / 3] += p;
            if (p > en_subshort[i + 3 - 2]) {
                assert(en_subshort[i + 3 - 2] > 0);
                p = p / en_subshort[i + 3 - 2];
            }
            else if (en_subshort[i + 3 - 2] > p * 10.0f) {
                assert(p > 0);
                p = en_subshort[i + 3 - 2] / (p * 10.0f);
            }
            else {
                p = 0.0;
            }
            attack_intensity[i + 3] = p;
        }

        /* pulse like signal detection for fatboy.wav and so on */
        for (i = 0; i < 3; ++i) {
            FLOAT const enn =
                en_subshort[i * 3 + 3] + en_subshort[i * 3 + 4] + en_subshort[i * 3 + 5];
            FLOAT   factor = 1.f;
            if (en_subshort[i * 3 + 5] * 6 < enn) {
                factor *= 0.5f;
                if (en_subshort[i * 3 + 4] * 6 < enn) {
                    factor *= 0.5f;
                }
            }
            sub_short_factor[chn][i] = factor;
        }

        if (plt) {
            FLOAT   x = attack_intensity[0];
            for (i = 1; i < 12; i++) {
                if (x < attack_intensity[i]) {
                    x = attack_intensity[i];
                }
            }
            plt->ers[gr_out][chn] = plt->ers_save[chn];
            plt->ers_save[chn] = x;
        }

        /* compare energies between sub-shortblocks */
        {
            FLOAT   x = gfc->cd_psy->attack_threshold[chn];
            for (i = 0; i < 12; i++) {
                if (ns_attacks[chn][i / 3] == 0) {
                    if (attack_intensity[i] > x) {
                        ns_attacks[chn][i / 3] = (i % 3) + 1;
                    }
                }
            }
        }
        /* should have energy change between short blocks, in order to avoid periodic signals */
        /* Good samples to show the effect are Trumpet test songs */
        /* GB: tuned (1) to avoid too many short blocks for test sample TRUMPET */
        /* RH: tuned (2) to let enough short blocks through for test sample FSOL and SNAPS */
        for (i = 1; i < 4; i++) {
            FLOAT const u = en_short[i - 1];
            FLOAT const v = en_short[i];
            FLOAT const m = Max(u, v);
            if (m < 40000) { /* (2) */
                if (u < 1.7f * v && v < 1.7f * u) { /* (1) */
                    if (i == 1 && ns_attacks[chn][0] <= ns_attacks[chn][i]) {
                        ns_attacks[chn][0] = 0;
                    }
                    ns_attacks[chn][i] = 0;
                }
            }
        }

        if (ns_attacks[chn][0] <= psv->last_attacks[chn]) {
            ns_attacks[chn][0] = 0;
        }

        if (psv->last_attacks[chn] == 3 ||
            ns_attacks[chn][0] + ns_attacks[chn][1] + ns_attacks[chn][2] + ns_attacks[chn][3]) {
            ns_uselongblock = 0;

            if (ns_attacks[chn][1] && ns_attacks[chn][0]) {
                ns_attacks[chn][1] = 0;
            }
            if (ns_attacks[chn][2] && ns_attacks[chn][1]) {
                ns_attacks[chn][2] = 0;
            }
            if (ns_attacks[chn][3] && ns_attacks[chn][2]) {
                ns_attacks[chn][3] = 0;
            }
        }

        if (chn < 2) {
            uselongblock[chn] = ns_uselongblock;
        }
        else {
            if (ns_uselongblock == 0) {
                uselongblock[0] = uselongblock[1] = 0;
            }
        }
    }
}


/**
 * \brief Move the short-block spreading state on by one granule, when the
 *        short-block masking of the granule was not computed.
 */
static void
vbrpsy_skip_masking_s(lame_internal_flags * gfc, int chn, int sblock)
{
    if (sblock == 0) {
        FLOAT  *nbs2 = &gfc->sv_psy.nb_s2[chn][0];
        FLOAT  *nbs1 = &gfc->sv_psy.nb_s1[chn][0];
        int const n = gfc->cd_psy->s.npart;
        int     b;
        for (b = 0; b < n; b++) {
            nbs2[b] = nbs1[b];
        }
    }
}


/**
 * \internal
 * \brief Applies the two limits to the masking threshold of a partition.
 *
 * The threshold may not be above a limit from the peak line of the band. If
 * it were, the quantizer would take the difference from other bands, and a
 * strongly tonal band would be heavily distorted. And it may not be above the
 * energy of the band itself. The masking_lower factor applies around these
 * limits: a factor above 1 before the energy limit, a factor below 1 after it.
 *
 * \param thr            the threshold.
 * \param max            the peak line energy of the partition.
 * \param minval         the minimum masking value of the partition.
 * \param avg_mask       the tonality factor of the spread energy.
 * \param masking_lower  the masking_lower factor of the partition.
 * \param eb             the energy of the partition.
 * \return the bounded threshold.
 */
static inline FLOAT
bound_threshold(FLOAT thr, FLOAT max, FLOAT minval, FLOAT avg_mask, FLOAT masking_lower,
                FLOAT eb)
{
    FLOAT   x = max;

    x *= minval;
    x *= avg_mask;
    if (thr > x) {
        thr = x;
    }
    if (masking_lower > 1) {
        thr *= masking_lower;
    }
    if (thr > eb) {
        thr = eb;
    }
    if (masking_lower < 1) {
        thr *= masking_lower;
    }
    return thr;
}


/**
 * \brief Masking thresholds for one short sub-block.
 *
 * Spreads the energy of each partition over its neighbors through the
 * precomputed spreading matrix, combines the contributions with
 * vbrpsy_mask_add(), and scales the result by the tonality of the bands that
 * contributed.
 *
 * Then bound_threshold() applies the two limits to the threshold.
 *
 * Pre-echo control does not happen here. For short blocks it happens after
 * the mapping to scalefactor bands, in L3psycho_anal_vbr(), where the
 * position of the attack in the granule is known.
 */
static void
vbrpsy_compute_masking_s(lame_internal_flags * gfc, const FLOAT(*fftenergy_s)[HBLKSIZE_s],
                         FLOAT * eb, FLOAT * thr, int chn, int sblock)
{
    PsyStateVar_t *const psv = &gfc->sv_psy;
    PsyConst_CB2SB_t const *const gds = &gfc->cd_psy->s;
    FLOAT   max[CBANDS], avg[CBANDS];
    int     j, b;
    unsigned char mask_idx_s[CBANDS];

    memset(max, 0, sizeof(max));
    memset(avg, 0, sizeof(avg));

    calc_energy(gds, fftenergy_s[sblock], eb, max, avg);
    calc_mask_index(gds, max, avg, mask_idx_s);
    for (j = b = 0; b < gds->npart; b++) {
        int     kk = gds->s3ind[b][0];
        int const last = gds->s3ind[b][1];
        int const delta = mask_add_delta(mask_idx_s[b]);
        int     dd, dd_n;
        FLOAT   x, ecb, avg_mask;
        FLOAT const masking_lower = gds->masking_lower[b] * gfc->sv_qnt.masking_lower;

        dd = mask_idx_s[kk];
        dd_n = 1;
        ecb = gds->s3[j] * eb[kk] * tab[mask_idx_s[kk]];
        ++j, ++kk;
        while (kk <= last) {
            dd += mask_idx_s[kk];
            dd_n += 1;
            x = gds->s3[j] * eb[kk] * tab[mask_idx_s[kk]];
            ecb = vbrpsy_mask_add(ecb, x, kk - b, delta);
            ++j, ++kk;
        }
        dd = (1 + 2 * dd) / (2 * dd_n);
        avg_mask = tab[dd] * 0.5f;
        ecb *= avg_mask;
        thr[b] = ecb;
        psv->nb_s2[chn][b] = psv->nb_s1[chn][b];
        psv->nb_s1[chn][b] = ecb;
        thr[b] = bound_threshold(thr[b], max[b], gds->minval[b], avg_mask, masking_lower, eb[b]);

        assert(thr[b] >= 0);
    }
    for (; b < CBANDS; ++b) {
        eb[b] = 0;
        thr[b] = 0;
    }
}


/**
 * \brief Masking thresholds for a long block, including pre-echo control.
 *
 * Spreading and non-linear addition are as in vbrpsy_compute_masking_s(), and
 * bound_threshold() applies the two limits. This path adds pre-echo control
 * over time. The threshold may not rise much above the thresholds of the two
 * granules before. So in a quiet passage before a sudden loud passage, the
 * noise floor cannot rise before the loud passage arrives to mask it.
 *
 * If the previous granule used short blocks, only the limit from that granule
 * is used. If no long-block analysis exists for that granule, the limit is
 * estimated from the energy of the current band. This estimate is a guess
 * that saves computing time.
 */
static void
vbrpsy_compute_masking_l(lame_internal_flags * gfc, const FLOAT fftenergy[HBLKSIZE],
                         FLOAT eb_l[CBANDS], FLOAT thr[CBANDS], int chn)
{
    PsyStateVar_t *const psv = &gfc->sv_psy;
    PsyConst_CB2SB_t const *const gdl = &gfc->cd_psy->l;
    FLOAT   max[CBANDS], avg[CBANDS];
    unsigned char mask_idx_l[CBANDS + 2];
    int     k, b;

 /*********************************************************************
    *    Calculate the energy and the tonality of each partition.
 *********************************************************************/
    calc_energy(gdl, fftenergy, eb_l, max, avg);
    calc_mask_index(gdl, max, avg, mask_idx_l);

 /*********************************************************************
    *      convolve the partitioned energy and unpredictability
    *      with the spreading function, s3_l[b][k]
 ********************************************************************/
    k = 0;
    for (b = 0; b < gdl->npart; b++) {
        FLOAT   x, ecb, avg_mask, t;
        FLOAT const masking_lower = gdl->masking_lower[b] * gfc->sv_qnt.masking_lower;
        /* convolve the partitioned energy with the spreading function */
        int     kk = gdl->s3ind[b][0];
        int const last = gdl->s3ind[b][1];
        int const delta = mask_add_delta(mask_idx_l[b]);
        int     dd = 0, dd_n = 0;

        dd = mask_idx_l[kk];
        dd_n += 1;
        ecb = gdl->s3[k] * eb_l[kk] * tab[mask_idx_l[kk]];
        ++k, ++kk;
        while (kk <= last) {
            dd += mask_idx_l[kk];
            dd_n += 1;
            x = gdl->s3[k] * eb_l[kk] * tab[mask_idx_l[kk]];
            t = vbrpsy_mask_add(ecb, x, kk - b, delta);
            ecb = t;
            ++k, ++kk;
        }
        dd = (1 + 2 * dd) / (2 * dd_n);
        avg_mask = tab[dd] * 0.5f;
        ecb *= avg_mask;

        /****   long block pre-echo control   ****/
        /* dont use long block pre-echo control if previous granule was 
         * a short block.  This is to avoid the situation:   
         * frame0:  quiet (very low masking)  
         * frame1:  surge  (triggers short blocks)
         * frame2:  regular frame.  looks like pre-echo when compared to 
         *          frame0, but all pre-echo was in frame1.
         */
        /* chn=0,1   L and R channels
           chn=2,3   S and M channels.
         */
        if (psv->blocktype_old[chn & 0x01] == SHORT_TYPE) {
            FLOAT const ecb_limit = rpelev * psv->nb_l1[chn][b];
            if (ecb_limit > 0) {
                thr[b] = Min(ecb, ecb_limit);
            }
            else {
                /* Robert 071209:
                   Because we don't calculate long block psy when we know a granule
                   should be of short blocks, we don't have any clue how the granule
                   before would have looked like as a long block. So we have to guess
                   a little bit for this END_TYPE block.
                   Most of the time we get away with this sloppyness. (fingers crossed :)
                   The speed increase is worth it.
                 */
                thr[b] = Min(ecb, eb_l[b] * NS_PREECHO_ATT2);
            }
        }
        else {
            FLOAT   ecb_limit_2 = rpelev2 * psv->nb_l2[chn][b];
            FLOAT   ecb_limit_1 = rpelev * psv->nb_l1[chn][b];
            FLOAT   ecb_limit;
            if (ecb_limit_2 <= 0) {
                ecb_limit_2 = ecb;
            }
            if (ecb_limit_1 <= 0) {
                ecb_limit_1 = ecb;
            }
            if (psv->blocktype_old[chn & 0x01] == NORM_TYPE) {
                ecb_limit = Min(ecb_limit_1, ecb_limit_2);
            }
            else {
                ecb_limit = ecb_limit_1;
            }
            thr[b] = Min(ecb, ecb_limit);
        }
        psv->nb_l2[chn][b] = psv->nb_l1[chn][b];
        psv->nb_l1[chn][b] = ecb;
        thr[b] = bound_threshold(thr[b], max[b], gdl->minval[b], avg_mask, masking_lower, eb_l[b]);
        assert(thr[b] >= 0);
    }
    for (; b < CBANDS; ++b) {
        eb_l[b] = 0;
        thr[b] = 0;
    }
}


/**
 * \brief Apply the configured short-block setting to the decision of each
 *        channel.
 *
 * Short blocks can be forced on, turned off, or coupled, so that both
 * channels always use the same block type.
 */
static void
vbrpsy_compute_block_type(SessionConfig_t const *cfg, int *uselongblock)
{
    int     chn;

    if (cfg->short_blocks == short_block_coupled
        /* force both channels to use the same block type */
        /* this is necessary if the frame is to be encoded in ms_stereo.  */
        /* But even without ms_stereo, FhG  does this */
        && !(uselongblock[0] && uselongblock[1]))
        uselongblock[0] = uselongblock[1] = 0;

    for (chn = 0; chn < cfg->channels_out; chn++) {
        /* disable short blocks */
        if (cfg->short_blocks == short_block_dispensed) {
            uselongblock[chn] = 1;
        }
        if (cfg->short_blocks == short_block_forced) {
            uselongblock[chn] = 0;
        }
    }
}


/**
 * \brief Decide the block type of the previous granule and return it to the
 *        caller.
 *
 * A block type cannot be chosen from its own granule alone. The window shape
 * needs a transition, so the granule before a short one becomes a start
 * block, and the granule after it a stop block. This is known only after the
 * next granule is analyzed. That is why the results of the model lag its
 * input by one granule.
 */
static void
vbrpsy_apply_block_type(PsyStateVar_t * psv, int nch, int const *uselongblock, int *blocktype_d)
{
    int     chn;

    /* update the blocktype of the previous granule, since it depends on what
     * happend in this granule */
    for (chn = 0; chn < nch; chn++) {
        int     blocktype = NORM_TYPE;
        /* disable short blocks */

        if (uselongblock[chn]) {
            /* no attack : use long blocks */
            assert(psv->blocktype_old[chn] != START_TYPE);
            if (psv->blocktype_old[chn] == SHORT_TYPE)
                blocktype = STOP_TYPE;
        }
        else {
            /* attack : use short blocks */
            blocktype = SHORT_TYPE;
            if (psv->blocktype_old[chn] == NORM_TYPE) {
                psv->blocktype_old[chn] = START_TYPE;
            }
            if (psv->blocktype_old[chn] == STOP_TYPE)
                psv->blocktype_old[chn] = SHORT_TYPE;
        }

        blocktype_d[chn] = psv->blocktype_old[chn]; /* value returned to calling program */
        psv->blocktype_old[chn] = blocktype; /* save for next call to l3psy_anal */
    }
}


/*************************************************************** 
 * compute M/S thresholds from Johnston & Ferreira 1992 ICASSP paper
 ***************************************************************/

/**
 * \brief Adjust mid and side thresholds for joint-stereo coding.
 *
 * Coding mid and side instead of left and right changes where the
 * quantization noise is in the stereo image. Noise at another place than the
 * signal that should mask it can become audible. This effect is the binaural
 * masking level difference. The correction applies only where the left and
 * right thresholds are within about 2 dB of each other. In that case the two
 * channels are similar enough for the effect to matter.
 *
 * If the caller set an msfix value, the mid and side thresholds are also kept
 * low relative to the smaller of the two mono thresholds. Both are first
 * raised to at least the absolute threshold of hearing.
 *
 * After Johnston and Ferreira, ICASSP 1992; the msfix refinement is Naoki
 * Shibata, 2000.
 */
static void
vbrpsy_compute_MS_thresholds(const FLOAT eb[4][CBANDS], FLOAT thr[4][CBANDS],
                             const FLOAT cb_mld[CBANDS], const FLOAT ath_cb[CBANDS], FLOAT athlower,
                             FLOAT msfix, int n)
{
    FLOAT const msfix2 = msfix * 2.f;
    FLOAT   rside, rmid;
    int     b;
    for (b = 0; b < n; ++b) {
        FLOAT const ebM = eb[2][b];
        FLOAT const ebS = eb[3][b];
        FLOAT const thmL = thr[0][b];
        FLOAT const thmR = thr[1][b];
        FLOAT   thmM = thr[2][b];
        FLOAT   thmS = thr[3][b];

        /* use this fix if L & R masking differs by 2db or less */
        /* if db = 10*log10(x2/x1) < 2 */
        /* if (x2 < 1.58*x1) { */
        if (thmL <= 1.58f * thmR && thmR <= 1.58f * thmL) {
            FLOAT const mld_m = cb_mld[b] * ebS;
            FLOAT const mld_s = cb_mld[b] * ebM;
            FLOAT const tmp_m = Min(thmS, mld_m);
            FLOAT const tmp_s = Min(thmM, mld_s);
            rmid = Max(thmM, tmp_m);
            rside = Max(thmS, tmp_s);
        }
        else {
            rmid = thmM;
            rside = thmS;
        }
        if (msfix > 0.f) {
            /***************************************************************/
            /* Adjust M/S maskings if user set "msfix"                     */
            /***************************************************************/
            /* Naoki Shibata 2000 */
            FLOAT   thmLR, thmMS;
            FLOAT const ath = ath_cb[b] * athlower;
            FLOAT const tmp_l = Max(thmL, ath);
            FLOAT const tmp_r = Max(thmR, ath);
            thmLR = Min(tmp_l, tmp_r);
            thmM = Max(rmid, ath);
            thmS = Max(rside, ath);
            thmMS = thmM + thmS;
            if (thmMS > 0.f && (thmLR * msfix2) < thmMS) {
                FLOAT const f = thmLR * msfix2 / thmMS;
                thmM *= f;
                thmS *= f;
                assert(thmMS > 0.f);
            }
            rmid = Min(thmM, rmid);
            rside = Min(thmS, rside);
        }
        if (rmid > ebM) {
            rmid = ebM;
        }
        if (rside > ebS) {
            rside = ebS;
        }
        thr[2][b] = rmid;
        thr[3][b] = rside;
    }
}


/*
 * NOTE: the bitrate reduction from the inter-channel masking effect is low
 * compared to the chance of getting annyoing artefacts. L3psycho_anal_vbr does
 * not use this feature. (Robert 071216)
*/

/**
 * \internal
 * \brief Short block pre-echo control.
 *
 * Where an attack is near, it lowers each new short block threshold towards
 * the one before it, and it applies the pulse factor of the attack detection.
 *
 * \param psv               the model's state; the short block thresholds change.
 * \param last_thm          the thresholds of the previous granule.
 * \param ns_attacks        the attacks per channel and short block.
 * \param sub_short_factor  the pulse factor per channel and short block.
 * \param n_chn_psy         the channels the model analyses.
 */
static void
vbrpsy_short_preecho(PsyStateVar_t * psv, III_psy_xmin const last_thm[4],
                     int ns_attacks[4][4], FLOAT sub_short_factor[4][3], int n_chn_psy)
{
    FLOAT const pcfact = 0.6f;
    FLOAT   thmm;
    int     chn, sb, sblock;

    for (chn = 0; chn < n_chn_psy; chn++) {
        for (sb = 0; sb < SBMAX_s; sb++) {
            FLOAT   new_thmm[3], prev_thm, t1, t2;
            for (sblock = 0; sblock < 3; sblock++) {
                thmm = psv->thm[chn].s[sb][sblock];
                thmm *= NS_PREECHO_ATT0;

                t1 = t2 = thmm;

                if (sblock > 0) {
                    prev_thm = new_thmm[sblock - 1];
                }
                else {
                    prev_thm = last_thm[chn].s[sb][2];
                }
                if (ns_attacks[chn][sblock] >= 2 || ns_attacks[chn][sblock + 1] == 1) {
                    t1 = NS_INTERP(prev_thm, thmm, NS_PREECHO_ATT1 * pcfact);
                }
                thmm = Min(t1, thmm);
                if (ns_attacks[chn][sblock] == 1) {
                    t2 = NS_INTERP(prev_thm, thmm, NS_PREECHO_ATT2 * pcfact);
                }
                else if ((sblock == 0 && psv->last_attacks[chn] == 3)
                         || (sblock > 0 && ns_attacks[chn][sblock - 1] == 3)) { /* 2nd preceeding block */
                    switch (sblock) {
                    case 0:
                        prev_thm = last_thm[chn].s[sb][1];
                        break;
                    case 1:
                        prev_thm = last_thm[chn].s[sb][2];
                        break;
                    case 2:
                        prev_thm = new_thmm[0];
                        break;
                    }
                    t2 = NS_INTERP(prev_thm, thmm, NS_PREECHO_ATT2 * pcfact);
                }

                thmm = Min(t2, thmm);

                /* pulse like signal detection for fatboy.wav and so on */
                thmm *= sub_short_factor[chn][sblock];

                new_thmm[sblock] = thmm;
            }
            for (sblock = 0; sblock < 3; sblock++) {
                psv->thm[chn].s[sb][sblock] = new_thmm[sblock];
            }
        }
    }
}


/**
 * \internal
 * \brief Run the psychoacoustic model over one granule.
 *
 * The single entry point, used by every encoding mode. The file description
 * says what the model computes and in which order.
 *
 * The results describe the **previous** granule, not the one in \a buffer,
 * because a block type is known only after the next granule.
 *
 * \param gfc               encoder state. The model reads its configuration
 *                          and the history of each channel, and updates the
 *                          history.
 * \param buffer            input samples per channel, centered on the granule.
 * \param gr_out            index of the granule the results are returned for.
 * \param[out] masking_ratio      per-band energies and thresholds, L/R.
 * \param[out] masking_MS_ratio   the same for mid/side, in joint stereo.
 * \param[out] percep_entropy     perceptual entropy, L/R.
 * \param[out] percep_MS_entropy  perceptual entropy, mid/side.
 * \param[out] energy             per-channel granule energy.
 * \param[out] blocktype_d        the settled block type per channel.
 */
void
L3psycho_anal_vbr(lame_internal_flags * gfc,
                  const sample_t * const buffer[2], int gr_out,
                  III_psy_ratio masking_ratio[2][2],
                  III_psy_ratio masking_MS_ratio[2][2],
                  FLOAT percep_entropy[2], FLOAT percep_MS_entropy[2],
                  FLOAT energy[4], int blocktype_d[2])
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    PsyStateVar_t *const psv = &gfc->sv_psy;
    PsyConst_CB2SB_t const *const gdl = &gfc->cd_psy->l;
    PsyConst_CB2SB_t const *const gds = &gfc->cd_psy->s;
    plotting_data *plt = cfg->analysis ? gfc->pinfo : 0;

    III_psy_xmin last_thm[4];

    /* fft and energy calculation   */
    FLOAT(*wsamp_l)[BLKSIZE];
    FLOAT(*wsamp_s)[3][BLKSIZE_s];
    FLOAT   fftenergy[HBLKSIZE];
    FLOAT   fftenergy_s[3][HBLKSIZE_s];
    FLOAT   wsamp_L[2][BLKSIZE];
    FLOAT   wsamp_S[2][3][BLKSIZE_s];
    FLOAT   eb[4][CBANDS], thr[4][CBANDS];

    FLOAT   sub_short_factor[4][3];
    FLOAT const ath_factor =
        (cfg->msfix > 0.f) ? (cfg->ATH_offset_factor * gfc->ATH->adjust_factor) : 1.f;

    const   FLOAT(*const_eb)[CBANDS] = (const FLOAT(*)[CBANDS]) eb;
    const   FLOAT(*const_fftenergy_s)[HBLKSIZE_s] = (const FLOAT(*)[HBLKSIZE_s]) fftenergy_s;

    /* block type  */
    int     ns_attacks[4][4] = { {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} };
    int     uselongblock[2] = { 1, 1 };

    /* usual variables like loop indices, etc..    */
    int     chn, sblock;

    /* chn=2 and 3 = Mid and Side channels */
    int const n_chn_psy = (cfg->mode == JOINT_STEREO) ? 4 : cfg->channels_out;

    memcpy(&last_thm[0], &psv->thm[0], sizeof(last_thm));

    vbrpsy_attack_detection(gfc, buffer, gr_out, masking_ratio, masking_MS_ratio, energy,
                            sub_short_factor, ns_attacks, uselongblock);

    vbrpsy_compute_block_type(cfg, uselongblock);

    /* LONG BLOCK CASE */
    {
        for (chn = 0; chn < n_chn_psy; chn++) {
            int const ch01 = chn & 0x01;

            wsamp_l = wsamp_L + ch01;
            vbrpsy_compute_fft_l(gfc, buffer, chn, gr_out, fftenergy, wsamp_l);
            vbrpsy_compute_loudness_approximation_l(gfc, gr_out, chn, fftenergy);
            vbrpsy_compute_masking_l(gfc, fftenergy, eb[chn], thr[chn], chn);
        }
        if (cfg->mode == JOINT_STEREO) {
            if ((uselongblock[0] + uselongblock[1]) == 2) {
                vbrpsy_compute_MS_thresholds(const_eb, thr, gdl->mld_cb, gfc->ATH->cb_l,
                                             ath_factor, cfg->msfix, gdl->npart);
            }
        }
        /* TODO: apply adaptive ATH masking here ?? */
        for (chn = 0; chn < n_chn_psy; chn++) {
            convert_partition2scalefac_l(gfc, eb[chn], thr[chn], chn);
            convert_partition2scalefac_l_to_s(gfc, eb[chn], thr[chn], chn);
        }
    }
    /* SHORT BLOCKS CASE */
    {
        int const force_short_block_calc = gfc->cd_psy->force_short_block_calc;
        for (sblock = 0; sblock < 3; sblock++) {
            for (chn = 0; chn < n_chn_psy; ++chn) {
                int const ch01 = chn & 0x01;
                if (uselongblock[ch01] && !force_short_block_calc) {
                    vbrpsy_skip_masking_s(gfc, chn, sblock);
                }
                else {
                    /* compute masking thresholds for short blocks */
                    wsamp_s = wsamp_S + ch01;
                    vbrpsy_compute_fft_s(gfc, buffer, chn, sblock, fftenergy_s, wsamp_s);
                    vbrpsy_compute_masking_s(gfc, const_fftenergy_s, eb[chn], thr[chn], chn,
                                             sblock);
                }
            }
            if (cfg->mode == JOINT_STEREO) {
                if ((uselongblock[0] + uselongblock[1]) == 0) {
                    vbrpsy_compute_MS_thresholds(const_eb, thr, gds->mld_cb, gfc->ATH->cb_s,
                                                 ath_factor, cfg->msfix, gds->npart);
                }
            }
            /* TODO: apply adaptive ATH masking here ?? */
            for (chn = 0; chn < n_chn_psy; ++chn) {
                int const ch01 = chn & 0x01;
                if (!uselongblock[ch01] || force_short_block_calc) {
                    convert_partition2scalefac_s(gfc, eb[chn], thr[chn], chn, sblock);
                }
            }
        }

        vbrpsy_short_preecho(psv, last_thm, ns_attacks, sub_short_factor, n_chn_psy);
    }
    for (chn = 0; chn < n_chn_psy; chn++) {
        psv->last_attacks[chn] = ns_attacks[chn][2];
    }


    /*************************************************************** 
    * determine final block type
    ***************************************************************/
    vbrpsy_apply_block_type(psv, cfg->channels_out, uselongblock, blocktype_d);

    /*********************************************************************
    * compute the value of PE to return ... no delay and advance
    *********************************************************************/
    for (chn = 0; chn < n_chn_psy; chn++) {
        FLOAT  *ppe;
        int     type;
        III_psy_ratio const *mr;

        if (chn > 1) {
            ppe = percep_MS_entropy - 2;
            type = NORM_TYPE;
            if (blocktype_d[0] == SHORT_TYPE || blocktype_d[1] == SHORT_TYPE)
                type = SHORT_TYPE;
            mr = &masking_MS_ratio[gr_out][chn - 2];
        }
        else {
            ppe = percep_entropy;
            type = blocktype_d[chn];
            mr = &masking_ratio[gr_out][chn];
        }
        if (type == SHORT_TYPE) {
            ppe[chn] = pecalc_s(mr, gfc->sv_qnt.masking_lower);
        }
        else {
            ppe[chn] = pecalc_l(mr, gfc->sv_qnt.masking_lower);
        }

        if (plt) {
            plt->pe[gr_out][chn] = ppe[chn];
        }
    }
}




/* 
 *   The spreading function.  Values returned in units of energy
 */
/**
 * \brief The spreading function: how much a masker raises the threshold at a
 *        given bark distance.
 *
 * It is asymmetric, like hearing: masking reaches further up in frequency
 * than down. The distance is scaled differently on the two sides before one
 * common curve is computed, and a shallow notch is subtracted just above the
 * masker.
 *
 * The curve depends only on the distance in frequency. It has no term for the
 * level of the masker, so the same spreading is used at every signal level.
 * The result is normalized, so that spreading a flat spectrum does not change
 * it.
 */
static  FLOAT
s3_func(FLOAT bark)
{
    FLOAT   tempx, x, tempy, temp;
    tempx = bark;
    if (tempx >= 0)
        tempx *= 3;
    else
        tempx *= 1.5;

    if (tempx >= 0.5 && tempx <= 2.5) {
        temp = tempx - 0.5;
        x = 8.0 * (temp * temp - 2.0 * temp);
    }
    else
        x = 0.0;
    tempx += 0.474;
    tempy = 15.811389 + 7.5 * tempx - 17.5 * sqrt(1.0 + tempx * tempx);

    if (tempy <= -60.0)
        return 0.0;

    tempx = exp((x + tempy) * LN_TO_LOG10);

    /* Normalization.  The spreading function should be normalized so that:
       +inf
       /
       |  s3 [ bark ]  d(bark)   =  1
       /
       -inf
     */
    tempx /= .6609193;
    return tempx;
}

/**
 * \brief Weight that vbrpsy_compute_MS_thresholds() uses to move masking
 *        between the mid and side channels.
 *
 * It rises with frequency to a constant level. The source code says only that
 * it is a curve fitted to a published plot, without naming the source.
 */
static  FLOAT
stereo_demask(double f)
{
    /* setup stereo demasking thresholds */
    /* formula reverse enginerred from plot in paper */
    double  arg = freq2bark(f);
    arg = (Min(arg, 15.5) / 15.5);

    return pow(10.0, 1.25 * (1 - cos(PI * arg)) - 2.5);
}

/**
 * \brief Lay out the partition bands for one sample rate and transform size.
 *
 * Assigns each FFT line to a partition, so that a partition has about a fixed
 * bark width. For each scalefactor band it records in which partition the band
 * ends, and how to split a partition that crosses the band boundary.
 */
static void
init_numline(PsyConst_CB2SB_t * gd, FLOAT sfreq, int fft_size,
             int mdct_size, int sbmax, int const *scalepos)
{
    FLOAT   b_frq[CBANDS + 1];
    FLOAT const mdct_freq_frac = sfreq / (2.0f * mdct_size);
    FLOAT const deltafreq = fft_size / (2.0f * mdct_size);
    int     partition[HBLKSIZE] = { 0 };
    int     i, j, ni;
    int     sfb;
    sfreq /= fft_size;
    j = 0;
    ni = 0;
    /* compute numlines, the number of spectral lines in each partition band */
    /* each partition band should be about DELBARK wide. */
    for (i = 0; i < CBANDS; i++) {
        FLOAT   bark1;
        int     j2, nl;
        bark1 = freq2bark(sfreq * j);

        b_frq[i] = sfreq * j;

        for (j2 = j; freq2bark(sfreq * j2) - bark1 < DELBARK && j2 <= fft_size / 2; j2++);

        nl = j2 - j;
        gd->numlines[i] = nl;
        gd->rnumlines[i] = (nl > 0) ? (1.0f / nl) : 0;

        ni = i + 1;

        while (j < j2) {
            assert(j < HBLKSIZE);
            partition[j++] = i;
        }
        if (j > fft_size / 2) {
            j = fft_size / 2;
            ++i;
            break;
        }
    }
    assert(i < CBANDS);
    b_frq[i] = sfreq * j;

    gd->n_sb = sbmax;
    gd->npart = ni;

    {
        j = 0;
        for (i = 0; i < gd->npart; i++) {
            int const nl = gd->numlines[i];
            FLOAT const freq = sfreq * (j + nl / 2);
            gd->mld_cb[i] = stereo_demask(freq);
            j += nl;
        }
        for (; i < CBANDS; ++i) {
            gd->mld_cb[i] = 1;
        }
    }
    for (sfb = 0; sfb < sbmax; sfb++) {
        int     i1, i2, bo;
        int     start = scalepos[sfb];
        int     end = scalepos[sfb + 1];

        i1 = floor(.5 + deltafreq * (start - .5));
        if (i1 < 0)
            i1 = 0;
        i2 = floor(.5 + deltafreq * (end - .5));

        if (i2 > fft_size / 2)
            i2 = fft_size / 2;

        bo = partition[i2];
        gd->bm[sfb] = (partition[i1] + partition[i2]) / 2;
        gd->bo[sfb] = bo;

        /* calculate how much of this band belongs to current scalefactor band */
        {
            FLOAT const f_tmp = mdct_freq_frac * end;
            FLOAT   bo_w = (f_tmp - b_frq[bo]) / (b_frq[bo + 1] - b_frq[bo]);
            if (bo_w < 0) {
                bo_w = 0;
            }
            else {
                if (bo_w > 1) {
                    bo_w = 1;
                }
            }
            gd->bo_weight[sfb] = bo_w;
        }
        gd->mld[sfb] = stereo_demask(mdct_freq_frac * start);
    }
}

/**
 * \brief Center bark value and bark width of each partition band.
 */
static void
compute_bark_values(PsyConst_CB2SB_t const *gd, FLOAT sfreq, int fft_size,
                    FLOAT * bval, FLOAT * bval_width)
{
    /* compute bark values of each critical band */
    int     k, j = 0, ni = gd->npart;
    sfreq /= fft_size;
    for (k = 0; k < ni; k++) {
        int const w = gd->numlines[k];
        FLOAT   bark1, bark2;

        bark1 = freq2bark(sfreq * (j));
        bark2 = freq2bark(sfreq * (j + w - 1));
        bval[k] = .5 * (bark1 + bark2);

        bark1 = freq2bark(sfreq * (j - .5));
        bark2 = freq2bark(sfreq * (j + w - .5));
        bval_width[k] = bark2 - bark1;
        j += w;
    }
}

/**
 * \brief Precompute the spreading matrix from s3_func().
 *
 * The band layout is fixed for the whole encode. So the values that matter
 * are computed once, not for each pair of bands in every granule, and stored
 * with the index range they cover. Each row is normalized, so that spreading
 * does not change the total energy.
 */
static int
init_s3_values(FLOAT ** p, int (*s3ind)[2], int npart,
               FLOAT const *bval, FLOAT const *bval_width, FLOAT const *norm)
{
    FLOAT   s3[CBANDS][CBANDS];
    /* The s3 array is not linear in the bark scale.
     * bval[x] should be used to get the bark value.
     */
    int     i, j, k;
    int     numberOfNoneZero = 0;

    memset(&s3[0][0], 0, sizeof(s3));

    /* s[i][j], the value of the spreading function,
     * centered at band j (masker), for band i (maskee)
     *
     * i.e.: sum over j to spread into signal barkval=i
     * NOTE: i and j are used opposite as in the ISO docs
     */
    for (i = 0; i < npart; i++) {
        for (j = 0; j < npart; j++) {
            FLOAT   v = s3_func(bval[i] - bval[j]) * bval_width[j];
            s3[i][j] = v * norm[i];
        }
    }
    for (i = 0; i < npart; i++) {
        for (j = 0; j < npart; j++) {
            if (s3[i][j] > 0.0f)
                break;
        }
        s3ind[i][0] = j;

        for (j = npart - 1; j > 0; j--) {
            if (s3[i][j] > 0.0f)
                break;
        }
        s3ind[i][1] = j;
        numberOfNoneZero += (s3ind[i][1] - s3ind[i][0] + 1);
    }
    *p = lame_calloc(FLOAT, numberOfNoneZero);
    if (!*p)
        return -1;

    k = 0;
    for (i = 0; i < npart; i++)
        for (j = s3ind[i][0]; j <= s3ind[i][1]; j++)
            (*p)[k++] = s3[i][j];

    return 0;
}

/**
 * \internal
 * \brief The lowest absolute threshold over the FFT lines of one partition,
 *        as an energy over the whole partition.
 * \param cfg       the session; selects the threshold formula.
 * \param sfreq     the output sample rate, in Hz.
 * \param blksize   the FFT size of the block type.
 * \param first     the first line of the partition.
 * \param numlines  the number of lines of the partition.
 * \return the threshold, in FFT energy units.
 */
static double
partition_ath(SessionConfig_t const *cfg, FLOAT sfreq, int blksize, int first, int numlines)
{
    double  x = FLOAT_MAX;
    int     k;

    for (k = 0; k < numlines; k++) {
        FLOAT const freq = sfreq * (first + k) / (1000.0 * blksize);
        FLOAT   level;
        level = ATHformula(cfg, freq * 1000) - 20; /* scale to FFT units; returned value is in dB */
        level = pow(10., 0.1 * level); /* convert from dB -> energy */
        level *= numlines;
        if (x > level)
            x = level;
    }
    return x;
}


/**
 * \internal
 * \brief The signal-to-noise ratio that the spreading function is normalized
 *        to at one Bark value: snr_a up to bvl_a, then a straight line to
 *        snr_b at bvl_b.
 * \param bval   the Bark value of the partition.
 * \param snr_a  the ratio up to bvl_a, in dB.
 * \param snr_b  the ratio at bvl_b, in dB.
 * \param bvl_a  the Bark value where the line starts.
 * \param bvl_b  the Bark value where the line ends.
 * \return the ratio, in dB.
 */
static double
snr_between(FLOAT bval, FLOAT snr_a, FLOAT snr_b, FLOAT bvl_a, FLOAT bvl_b)
{
    double  snr = snr_a;

    if (bval >= bvl_a) {
        snr = snr_b * (bval - bvl_a) / (bvl_b - bvl_a)
            + snr_a * (bvl_b - bval) / (bvl_b - bvl_a);
    }
    return snr;
}


/**
 * \internal
 * \brief Sets the history the model keeps from granule to granule to its start.
 *
 * \param psv  the model's state.
 */
static void
init_psy_history(PsyStateVar_t * psv)
{
    int     i, j, sb;

    psv->blocktype_old[0] = psv->blocktype_old[1] = NORM_TYPE; /* the vbr header is long blocks */

    for (i = 0; i < 4; ++i) {
        for (j = 0; j < CBANDS; ++j) {
            psv->nb_l1[i][j] = 1e20;
            psv->nb_l2[i][j] = 1e20;
            psv->nb_s1[i][j] = psv->nb_s2[i][j] = 1.0;
        }
        for (sb = 0; sb < SBMAX_l; sb++) {
            psv->en[i].l[sb] = 1e20;
            psv->thm[i].l[sb] = 1e20;
        }
        for (j = 0; j < 3; ++j) {
            for (sb = 0; sb < SBMAX_s; sb++) {
                psv->en[i].s[sb][j] = 1e20;
                psv->thm[i].s[sb][j] = 1e20;
            }
        }
        psv->last_attacks[i] = 0;
        for (j = 0; j < 9; j++)
            psv->last_en_subshort[i][j] = 10.;
    }


    /* init. for loudness approx. -jd 2001 mar 27 */
    psv->loudness_sq_save[0] = psv->loudness_sq_save[1] = 0.0;
}


/**
 * \internal
 * \brief Sets the attack thresholds: the caller's, or the defaults.
 *
 * \param gd   the model's constants.
 * \param gfp  the settings; a negative threshold there means the default.
 */
static void
init_attack_thresholds(PsyConst_t * gd, lame_global_flags const *gfp)
{
    float   x = gfp->attackthre;
    float   y = gfp->attackthre_s;
    if (x < 0) {
        x = NSATTACKTHRE;
    }
    if (y < 0) {
        y = NSATTACKTHRE_S;
    }
    gd->attack_threshold[0] = gd->attack_threshold[1] = gd->attack_threshold[2] = x;
    gd->attack_threshold[3] = y;
}


#ifndef NDEBUG
/**
 * \internal
 * \brief Returns how many spectral lines the partitions cover.
 *
 * \param part  the partition layout of one block size.
 * \return the sum of the lines of every partition.
 */
static int
partition_line_count(PsyConst_CB2SB_t const *part)
{
    int     b, n = 0;
    for (b = 0; b < part->npart; ++b) {
        n += part->numlines[b];
    }
    return n;
}
#endif


/**
 * \internal
 * \brief Build the per-session constant tables the model needs.
 *
 * Called once per encoder instance. It lays out the partition bands for both
 * transform sizes. It precomputes the spreading matrices, the minimum masking
 * value of each band, the stereo demasking weights and the attack thresholds.
 * It initializes the history of each channel, which the model keeps from
 * granule to granule.
 *
 * \return 0 on success, non-zero if a table could not be allocated.
 */
int
psymodel_init(lame_global_flags const *gfp)
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    SessionConfig_t *const cfg = &gfc->cfg;
    PsyStateVar_t *const psv = &gfc->sv_psy;
    PsyConst_t *gd;
    int     i, j, b;
    FLOAT   bvl_a = 13, bvl_b = 24;
    FLOAT   snr_l_a = 0, snr_l_b = 0;
    FLOAT   snr_s_a = -8.25, snr_s_b = -4.5;

    FLOAT   bval[CBANDS];
    FLOAT   bval_width[CBANDS];
    FLOAT   norm[CBANDS];
    FLOAT const sfreq = cfg->samplerate_out;

    FLOAT   xav = 10, xbv = 12;
    FLOAT const minval_low = (0.f - cfg->minval);

    if (gfc->cd_psy != 0) {
        return 0;
    }
    memset(norm, 0, sizeof(norm));

    gd = lame_calloc(PsyConst_t, 1);
    if (gd == 0)
        return -1;
    gfc->cd_psy = gd;

    gd->force_short_block_calc = gfp->experimentalZ;

    init_psy_history(psv);



    /*************************************************************************
     * now compute the psychoacoustic model specific constants
     ************************************************************************/
    /* compute numlines, bo, bm, bval, bval_width, mld */
    init_numline(&gd->l, sfreq, BLKSIZE, SAMPLES_PER_GRANULE, SBMAX_l, gfc->scalefac_band.l);
    assert(gd->l.npart < CBANDS);
    compute_bark_values(&gd->l, sfreq, BLKSIZE, bval, bval_width);

    /* compute the spreading function */
    for (i = 0; i < gd->l.npart; i++) {
        double const snr = snr_between(bval[i], snr_l_a, snr_l_b, bvl_a, bvl_b);
        norm[i] = pow(10.0, snr / 10.0);
    }
    i = init_s3_values(&gd->l.s3, gd->l.s3ind, gd->l.npart, bval, bval_width, norm);
    if (i)
        return i;

    /* compute long block specific values, ATH and MINVAL */
    j = 0;
    for (i = 0; i < gd->l.npart; i++) {
        double  x;

        /* ATH */
        gfc->ATH->cb_l[i] = partition_ath(cfg, sfreq, BLKSIZE, j, gd->l.numlines[i]);
        j += gd->l.numlines[i];

        /* MINVAL.
           For low freq, the strength of the masking is limited by minval
           this is an ISO MPEG1 thing, dont know if it is really needed */
        /* FIXME: it does work to reduce low-freq problems in S53-Wind-Sax
           and lead-voice samples, but introduces some 3 kbps bit bloat too.
           TODO: Further refinement of the shape of this hack.
         */
        x = 20.0 * (bval[i] / xav - 1.0);
        if (x > 6) {
            x = 30;
        }
        if (x < minval_low) {
            x = minval_low;
        }
        if (cfg->samplerate_out < 44000) {
            x = 30;
        }
        x -= 8.;
        gd->l.minval[i] = pow(10.0, x / 10.) * gd->l.numlines[i];
    }

    /************************************************************************
     * do the same things for short blocks
     ************************************************************************/
    init_numline(&gd->s, sfreq, BLKSIZE_s, 192, SBMAX_s, gfc->scalefac_band.s);
    assert(gd->s.npart < CBANDS);
    compute_bark_values(&gd->s, sfreq, BLKSIZE_s, bval, bval_width);

    /* SNR formula. short block is normalized by SNR. is it still right ? */
    j = 0;
    for (i = 0; i < gd->s.npart; i++) {
        double  x;
        double const snr = snr_between(bval[i], snr_s_a, snr_s_b, bvl_a, bvl_b);
        norm[i] = pow(10.0, snr / 10.0);

        /* ATH */
        gfc->ATH->cb_s[i] = partition_ath(cfg, sfreq, BLKSIZE_s, j, gd->s.numlines[i]);
        j += gd->s.numlines[i];

        /* MINVAL.
           For low freq, the strength of the masking is limited by minval
           this is an ISO MPEG1 thing, dont know if it is really needed */
        x = 7.0 * (bval[i] / xbv - 1.0);
        if (bval[i] > xbv) {
            x *= 1 + log(1 + x) * 3.1;
        }
        if (bval[i] < xbv) {
            x *= 1 + log(1 - x) * 2.3;
        }
        if (x > 6) {
            x = 30;
        }
        if (x < minval_low) {
            x = minval_low;
        }
        if (cfg->samplerate_out < 44000) {
            x = 30;
        }
        x -= 8;
        gd->s.minval[i] = pow(10.0, x / 10) * gd->s.numlines[i];
    }

    i = init_s3_values(&gd->s.s3, gd->s.s3ind, gd->s.npart, bval, bval_width, norm);
    if (i)
        return i;


    init_mask_add_max_values();
    init_fft(gfc);

    /* setup temporal masking */
    gd->decay = exp(-1.0 * LOG10 / (temporalmask_sustain_sec * sfreq / 192.0));

    {
        FLOAT   msfix;
        msfix = NS_MSFIX;
        if (cfg->use_safe_joint_stereo)
            msfix = 1.0;
        if (fabs(cfg->msfix) > 0.0)
            msfix = cfg->msfix;
        cfg->msfix = msfix;

        /* spread only from npart_l bands.  Normally, we use the spreading
         * function to convolve from npart_l down to npart_l bands 
         */
        for (b = 0; b < gd->l.npart; b++)
            if (gd->l.s3ind[b][1] > gd->l.npart - 1)
                gd->l.s3ind[b][1] = gd->l.npart - 1;
    }

    /*  prepare for ATH auto adjustment:
     *  we want to decrease the ATH by 12 dB per second
     */
#define  frame_duration ((double) SAMPLES_PER_GRANULE * cfg->mode_gr / sfreq)
    gfc->ATH->decay = pow(10., -12. / 10. * frame_duration);
    gfc->ATH->adjust_factor = 0.01; /* minimum, for leading low loudness */
    gfc->ATH->adjust_limit = 1.0; /* on lead, allow adjust up to maximum */
#undef  frame_duration

    assert(gd->l.bo[SBMAX_l - 1] <= gd->l.npart);
    assert(gd->s.bo[SBMAX_s - 1] <= gd->s.npart);

    if (cfg->ATHtype != -1) {
        /* compute equal loudness weights (eql_w) */
        FLOAT   freq;
        FLOAT const freq_inc = (FLOAT) cfg->samplerate_out / (FLOAT) (BLKSIZE);
        FLOAT   eql_balance = 0.0;
        freq = 0.0;
        for (i = 0; i < BLKSIZE / 2; ++i) {
            /* convert ATH dB to relative power (not dB) */
            /*  to determine eql_w */
            freq += freq_inc;
            gfc->ATH->eql_w[i] = 1. / pow(10, ATHformula(cfg, freq) / 10);
            eql_balance += gfc->ATH->eql_w[i];
        }
        eql_balance = 1.0 / eql_balance;
        for (i = BLKSIZE / 2; --i >= 0;) { /* scale weights */
            gfc->ATH->eql_w[i] *= eql_balance;
        }
    }
    assert(partition_line_count(&gd->s) == HBLKSIZE_s);
    assert(partition_line_count(&gd->l) == HBLKSIZE);
    init_attack_thresholds(gd, gfp);
    {
        float   sk_s, sk_l;
        static float const sk[] =
            { -7.4, -7.4, -7.4, -9.5, -7.4, -6.1, -5.5, -4.7, -4.7, -4.7, -4.7 };
        if (gfp->VBR_q < 4) {
            sk_l = sk_s = sk[0];
        }
        else {
            sk_l = sk_s = sk[gfp->VBR_q] + gfp->VBR_q_frac * (sk[gfp->VBR_q] - sk[gfp->VBR_q + 1]);
        }
        b = 0;
        for (; b < gd->s.npart; b++) {
            float   m = (float) (gd->s.npart - b) / gd->s.npart;
            gd->s.masking_lower[b] = powf(10.f, sk_s * m * 0.1f);
        }
        for (; b < CBANDS; ++b) {
            gd->s.masking_lower[b] = 1.f;
        }
        b = 0;
        for (; b < gd->l.npart; b++) {
            float   m = (float) (gd->l.npart - b) / gd->l.npart;
            gd->l.masking_lower[b] = powf(10.f, sk_l * m * 0.1f);
        }
        for (; b < CBANDS; ++b) {
            gd->l.masking_lower[b] = 1.f;
        }
    }
    memcpy(&gd->l_to_s, &gd->l, sizeof(gd->l_to_s));
    init_numline(&gd->l_to_s, sfreq, BLKSIZE, 192, SBMAX_s, gfc->scalefac_band.s);
    return 0;
}
