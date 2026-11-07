/*
 *	psymodel.h
 *
 *	Copyright (c) 1999 Mark Taylor
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

/**
 * \file
 * \internal
 * \brief Entry points and tuning constants of the psychoacoustic model.
 *
 * \see psymodel.c for what the model computes and in which order.
 */

#ifndef LAME_PSYMODEL_H
#define LAME_PSYMODEL_H


void    L3psycho_anal_vbr(lame_internal_flags * gfc,
                          const sample_t *const buffer[2], int gr,
                          III_psy_ratio ratio[2][2],
                          III_psy_ratio MS_ratio[2][2],
                          FLOAT pe[2], FLOAT pe_MS[2], FLOAT ener[4], int blocktype_d[2]);


int     psymodel_init(lame_global_flags const* gfp);


/**
 * \brief How far a long-block threshold may rise above the threshold of the
 *        previous granule, and of the granule before that.
 *
 * Pre-echo control: in a quiet granule before a sudden loud passage, the noise
 * floor must not rise before the loud passage arrives to mask it. The nearer
 * granule sets the tighter limit.
 *
 * \see vbrpsy_compute_masking_l()
 */
#define rpelev 2
#define rpelev2 16      /**< \brief \copybrief rpelev */

/** \brief Width of a partition band, in barks. \see init_numline() */
#define DELBARK .34


/**
 * \brief Factor that scales the result of psycho_loudness_approx(), so that a
 *        signal near clipping gives about 1.0.
 *
 * It depends on the internal energy scale of the encoder, so it is not a free
 * parameter.
 */
#define VO_SCALE (1./( 14752*14752 )/(BLKSIZE/2))

/**
 * \brief Time constant of the post-masking sustain, in seconds.
 *
 * After a masker stops, a sound stays partly masked for a short time
 * (temporal masking). The encoder uses this effect only here.
 * psymodel_init() converts the value into a decay factor for each sub-block.
 * calc_xmin() then raises the allowed noise of a short sub-block towards the
 * value of the previous sub-block, if that value is higher.
 *
 * This happens only when temporal masking is on (lame_set_useTemporal()). It
 * is on by default, except with \c vbr_mtrh, where it is off by default.
 */
#define temporalmask_sustain_sec 0.01

/**
 * \brief Short-block pre-echo attenuations.
 *
 * Applied to the thresholds of the scalefactor bands in L3psycho_anal_vbr(),
 * when the position of an attack in the granule is known. #NS_PREECHO_ATT0
 * attenuates every sub-block. The other two weight an interpolation towards
 * the threshold of the previous sub-block. The closer the attack, the
 * stronger the weight.
 */
#define NS_PREECHO_ATT0 0.8
#define NS_PREECHO_ATT1 0.6     /**< \brief \copybrief NS_PREECHO_ATT0 */
#define NS_PREECHO_ATT2 0.3     /**< \brief \copybrief NS_PREECHO_ATT0 */

/** \brief Default bound on mid/side thresholds relative to left/right.
 *  \see vbrpsy_compute_MS_thresholds(), lame_set_msfix() */
#define NS_MSFIX 3.5

/**
 * \brief Default energy ratio between sub-blocks that counts as an attack.
 *        #NSATTACKTHRE is for the left, right and mid channels,
 *        #NSATTACKTHRE_S for the side channel.
 *
 * Used only if the attack threshold is still negative. lame_init_params()
 * applies a preset in every mode, including CBR and ABR. A preset always sets
 * both thresholds. So an encode through the frontend or the documented API
 * never uses these values.
 *
 * \see vbrpsy_attack_detection(), lame_set_short_threshold_lrm()
 */
#define NSATTACKTHRE 4.4
#define NSATTACKTHRE_S 25       /**< \brief \copybrief NSATTACKTHRE */

#endif /* LAME_PSYMODEL_H */
