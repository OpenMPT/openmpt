/*
 *      LAME MP3 encoding engine
 *
 *      Copyright (c) 1999 Mark Taylor
 *      Copyright (c) 2000-2002 Takehiro Tominaga
 *      Copyright (c) 2000-2011 Robert Hegemann
 *      Copyright (c) 2001 Gabriel Bouvigne
 *      Copyright (c) 2001 John Dahlstrom
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "lame_global_flags.h"
#include "newmdct.h"
#include "psymodel.h"
#include "lame-analysis.h"
#include "bitstream.h"
#include "VbrTag.h"
#include "quantize.h"
#include "quantize_pvt.h"



/*
 * auto-adjust of ATH, useful for low volume
 * Gabriel Bouvigne 3 feb 2001
 *
 * modifies some values in
 *   gfp->internal_flags->ATH
 *   (gfc->ATH)
 */
/**
 * \brief Scale the absolute threshold of hearing to the loudness of the
 *        granule.
 *
 * A listener turns a quiet passage up. So a threshold for one playback level
 * is wrong for quiet material: noise below the threshold at a normal level
 * becomes audible when the volume is turned up. This function lowers the
 * threshold when the granule gets quieter, by up to about 32 dB.
 *
 * It uses psycho_loudness_approx(), a power sum weighted by equal loudness,
 * shifted by a configurable sensitivity. With it, the caller can move the
 * range where the adjustment works.
 *
 * The response is asymmetric on purpose. A rise takes effect after the delay
 * of one frame, so that a loud passage is not encoded with a threshold that is
 * still low from the quiet passage before. A fall happens slowly, so that a
 * short dip does not move the threshold down and up again.
 *
 * Turning the adjustment off is a supported setting (lame_set_athaa_type()).
 * It is the only way to encode with a threshold that assumes no playback
 * level.
 */
static void
adjust_ATH(lame_internal_flags const *const gfc)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    FLOAT   gr2_max, max_pow;

    if (gfc->ATH->use_adjust == 0) {
        gfc->ATH->adjust_factor = 1.0; /* no adjustment */
        return;
    }

    /* jd - 2001 mar 12, 27, jun 30 */
    /* loudness based on equal loudness curve; */
    /* use granule with maximum combined loudness */
    max_pow = gfc->ov_psy.loudness_sq[0][0];
    gr2_max = gfc->ov_psy.loudness_sq[1][0];
    if (cfg->channels_out == 2) {
        max_pow += gfc->ov_psy.loudness_sq[0][1];
        gr2_max += gfc->ov_psy.loudness_sq[1][1];
    }
    else {
        max_pow += max_pow;
        gr2_max += gr2_max;
    }
    if (cfg->mode_gr == 2) {
        max_pow = Max(max_pow, gr2_max);
    }
    max_pow *= 0.5;     /* max_pow approaches 1.0 for full band noise */

    /* jd - 2001 mar 31, jun 30 */
    /* user tuning of ATH adjustment region */
    max_pow *= gfc->ATH->aa_sensitivity_p;

    /*  adjust ATH depending on range of maximum value
     */

    /* jd - 2001 feb27, mar12,20, jun30, jul22 */
    /* continuous curves based on approximation */
    /* to GB's original values. */
    /* For an increase in approximate loudness, */
    /* set ATH adjust to adjust_limit immediately */
    /* after a delay of one frame. */
    /* For a loudness decrease, reduce ATH adjust */
    /* towards adjust_limit gradually. */
    /* max_pow is a loudness squared or a power. */
    if (max_pow > 0.03125) { /* ((1 - 0.000625)/ 31.98) from curve below */
        if (gfc->ATH->adjust_factor >= 1.0) {
            gfc->ATH->adjust_factor = 1.0;
        }
        else {
            /* preceding frame has lower ATH adjust; */
            /* ascend only to the preceding adjust_limit */
            /* in case there is leading low volume */
            if (gfc->ATH->adjust_factor < gfc->ATH->adjust_limit) {
                gfc->ATH->adjust_factor = gfc->ATH->adjust_limit;
            }
        }
        gfc->ATH->adjust_limit = 1.0;
    }
    else {              /* adjustment curve */
        /* about 32 dB maximum adjust (0.000625) */
        FLOAT const adj_lim_new = 31.98 * max_pow + 0.000625;
        if (gfc->ATH->adjust_factor >= adj_lim_new) { /* descend gradually */
            gfc->ATH->adjust_factor *= adj_lim_new * 0.075 + 0.925;
            if (gfc->ATH->adjust_factor < adj_lim_new) { /* stop descent */
                gfc->ATH->adjust_factor = adj_lim_new;
            }
        }
        else {          /* ascend */
            if (gfc->ATH->adjust_limit >= adj_lim_new) {
                gfc->ATH->adjust_factor = adj_lim_new;
            }
            else {      /* preceding frame has lower ATH adjust; */
                /* ascend only to the preceding adjust_limit */
                if (gfc->ATH->adjust_factor < gfc->ATH->adjust_limit) {
                    gfc->ATH->adjust_factor = gfc->ATH->adjust_limit;
                }
            }
        }
        gfc->ATH->adjust_limit = adj_lim_new;
    }
}

/***********************************************************************
 *
 *  some simple statistics
 *
 *  bitrate index 0: free bitrate -> not allowed in VBR mode
 *  : bitrates, kbps depending on MPEG version
 *  bitrate index 15: forbidden
 *
 *  mode_ext:
 *  0:  LR
 *  1:  LR-i
 *  2:  MS
 *  3:  MS-i
 *
 ***********************************************************************/

static void
updateStats(lame_internal_flags * const gfc)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncResult_t *eov = &gfc->ov_enc;
    int     gr, ch;
    assert(0 <= eov->bitrate_index && eov->bitrate_index < 16);
    assert(0 <= eov->mode_ext && eov->mode_ext < 4);

    /* count bitrate indices */
    eov->bitrate_channelmode_hist[eov->bitrate_index][4]++;
    eov->bitrate_channelmode_hist[15][4]++;

    /* count 'em for every mode extension in case of 2 channel encoding */
    if (cfg->channels_out == 2) {
        eov->bitrate_channelmode_hist[eov->bitrate_index][eov->mode_ext]++;
        eov->bitrate_channelmode_hist[15][eov->mode_ext]++;
    }
    for (gr = 0; gr < cfg->mode_gr; ++gr) {
        for (ch = 0; ch < cfg->channels_out; ++ch) {
            int     bt = gfc->l3_side.tt[gr][ch].granule.block_type;
            if (gfc->l3_side.tt[gr][ch].granule.mixed_block_flag)
                bt = 4;
            eov->bitrate_blocktype_hist[eov->bitrate_index][bt]++;
            eov->bitrate_blocktype_hist[eov->bitrate_index][5]++;
            eov->bitrate_blocktype_hist[15][bt]++;
            eov->bitrate_blocktype_hist[15][5]++;
        }
    }
}




static void
lame_encode_frame_init(lame_internal_flags * gfc, const sample_t *const inbuf[2])
{
    SessionConfig_t const *const cfg = &gfc->cfg;

    int     ch, gr;

    if (gfc->lame_encode_frame_init == 0) {
        sample_t primebuff0[SUBBAND_LOOKBACK + 1152 + SAMPLES_PER_GRANULE];
        sample_t primebuff1[SUBBAND_LOOKBACK + 1152 + SAMPLES_PER_GRANULE];
        int const framesize = SAMPLES_PER_GRANULE * cfg->mode_gr;
        /* prime the MDCT/polyphase filterbank with a short block */
        int     i, j;
        gfc->lame_encode_frame_init = 1;
        memset(primebuff0, 0, sizeof(primebuff0));
        memset(primebuff1, 0, sizeof(primebuff1));
        for (i = 0, j = 0; i < SUBBAND_LOOKBACK + SAMPLES_PER_GRANULE * (1 + cfg->mode_gr); ++i) {
            if (i < framesize) {
                primebuff0[i] = 0;
                if (cfg->channels_out == 2)
                    primebuff1[i] = 0;
            }
            else {
                primebuff0[i] = inbuf[0][j];
                if (cfg->channels_out == 2)
                    primebuff1[i] = inbuf[1][j];
                ++j;
            }
        }
        /* polyphase filtering / mdct */
        for (gr = 0; gr < cfg->mode_gr; gr++) {
            for (ch = 0; ch < cfg->channels_out; ch++) {
                gfc->l3_side.tt[gr][ch].granule.block_type = SHORT_TYPE;
            }
        }
        mdct_sub48(gfc, primebuff0, primebuff1);

        /* check FFT will not use a negative starting offset */
#if SAMPLES_PER_GRANULE < FFTOFFSET
# error FFTOFFSET greater than SAMPLES_PER_GRANULE: FFT uses a negative offset
#endif
        /* check if we have enough data for FFT */
        assert(gfc->sv_enc.mf_size >= (BLKSIZE + framesize - FFTOFFSET));
        /* check if we have enough data for polyphase filterbank */
        assert(gfc->sv_enc.mf_size >= (512 + framesize - 32));
    }

}







/**
 * \internal
 * \brief Runs the psychoacoustic model over the granules of a frame.
 *
 * It also sets each granule's block type, and in joint stereo the share of
 * the side channel in the energy.
 *
 * \param gfc            the encoder.
 * \param inbuf          the input of the frame, per channel.
 * \param masking_LR     receives the masking of left and right.
 * \param masking_MS     receives the masking of mid and side.
 * \param pe             receives the perceptual entropy of left and right.
 * \param pe_MS          receives the perceptual entropy of mid and side.
 * \param ms_ener_ratio  receives the side channel's share of the energy,
 *                       in joint stereo only.
 */
static void
run_psymodel(lame_internal_flags * gfc, const sample_t *const inbuf[2],
             III_psy_ratio masking_LR[2][2], III_psy_ratio masking_MS[2][2],
             FLOAT pe[2][2], FLOAT pe_MS[2][2], FLOAT ms_ener_ratio[2])
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    FLOAT   tot_ener[2][4];
    const sample_t *bufp[2] = {0, 0}; /* address of beginning of left & right granule */
    int     blocktype[2];
    int     ch, gr;

    /* psy model has a 1 granule (576) delay that we must compensate for
     * (mt 6/99).
     */
    for (gr = 0; gr < cfg->mode_gr; gr++) {

        for (ch = 0; ch < cfg->channels_out; ch++) {
            bufp[ch] = &inbuf[ch][SAMPLES_PER_GRANULE + gr * SAMPLES_PER_GRANULE - FFTOFFSET];
        }
        L3psycho_anal_vbr(gfc, bufp, gr,
                          masking_LR, masking_MS,
                          pe[gr], pe_MS[gr], tot_ener[gr], blocktype);

        if (cfg->mode == JOINT_STEREO) {
            ms_ener_ratio[gr] = tot_ener[gr][2] + tot_ener[gr][3];
            if (ms_ener_ratio[gr] > 0)
                ms_ener_ratio[gr] = tot_ener[gr][3] / ms_ener_ratio[gr];
        }

        /* block type flags */
        for (ch = 0; ch < cfg->channels_out; ch++) {
            gr_info *const cod_info = &gfc->l3_side.tt[gr][ch];
            cod_info->granule.block_type = blocktype[ch];
            cod_info->granule.mixed_block_flag = 0;
        }
    }
}


/**
 * \internal
 * \brief Chooses between mid/side and left/right coding of a frame.
 *
 * \param gfc    the encoder, with the block types of the frame.
 * \param pe     the perceptual entropy of left and right.
 * \param pe_MS  the perceptual entropy of mid and side.
 * \return MPG_MD_MS_LR or MPG_MD_LR_LR.
 */
static int
choose_mode_ext(lame_internal_flags const *gfc, FLOAT pe[2][2], FLOAT pe_MS[2][2])
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    int     mode_ext = MPG_MD_LR_LR;
    int     ch, gr;

    if (cfg->force_ms) {
        mode_ext = MPG_MD_MS_LR;
    }
    else if (cfg->mode == JOINT_STEREO) {
        /* ms_ratio = is scaled, for historical reasons, to look like
           a ratio of side_channel / total.
           0 = signal is 100% mono
           .5 = L & R uncorrelated
         */

        /* [0] and [1] are the results for the two granules in MPEG-1,
         * in MPEG-2 it's only a faked averaging of the same value
         * _prev is the value of the last granule of the previous frame
         * _next is the value of the first granule of the next frame
         */

        FLOAT   sum_pe_MS = 0;
        FLOAT   sum_pe_LR = 0;
        for (gr = 0; gr < cfg->mode_gr; gr++) {
            for (ch = 0; ch < cfg->channels_out; ch++) {
                sum_pe_MS += pe_MS[gr][ch];
                sum_pe_LR += pe[gr][ch];
            }
        }

        /* based on PE: M/S coding would not use much more bits than L/R */
        if (sum_pe_MS <= 1.00 * sum_pe_LR) {

            gr_info const *const gi0 = &gfc->l3_side.tt[0][0];
            gr_info const *const gi1 = &gfc->l3_side.tt[cfg->mode_gr - 1][0];

            if (gi0[0].granule.block_type == gi0[1].granule.block_type
                && gi1[0].granule.block_type == gi1[1].granule.block_type) {

                mode_ext = MPG_MD_MS_LR;
            }
        }
    }
    return mode_ext;
}


/**
 * \internal
 * \brief Gives the frame analyzer what the model found in the frame.
 *
 * \param gfc            the encoder, with the analyzer's record.
 * \param ms_ener_ratio  the side channel's share of the energy.
 * \param pe_use         the perceptual entropy of the chosen channel mode.
 */
static void
copy_analysis_input(lame_internal_flags * gfc, FLOAT const ms_ener_ratio[2], FLOAT pe_use[2][2])
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    int     ch, gr;

    for (gr = 0; gr < cfg->mode_gr; gr++) {
        for (ch = 0; ch < cfg->channels_out; ch++) {
            gfc->pinfo->ms_ratio[gr] = 0;
            gfc->pinfo->ms_ener_ratio[gr] = ms_ener_ratio[gr];
            gfc->pinfo->blocktype[gr][ch] = gfc->l3_side.tt[gr][ch].granule.block_type;
            gfc->pinfo->pe[gr][ch] = pe_use[gr][ch];
            /* pinfo->xr is double but tt[].granule.xr is FLOAT: convert
               element-wise, a raw copy would reinterpret the bits */
            {
                int     i;
                for (i = 0; i < SAMPLES_PER_GRANULE; ++i)
                    gfc->pinfo->xr[gr][ch][i] = gfc->l3_side.tt[gr][ch].granule.xr[i];
            }
            /* in psymodel, LR and MS data was stored in pinfo.  
               switch to MS data: */
            if (gfc->ov_enc.mode_ext == MPG_MD_MS_LR) {
                gfc->pinfo->ers[gr][ch] = gfc->pinfo->ers[gr][ch + 2];
                memcpy(gfc->pinfo->energy[gr][ch], gfc->pinfo->energy[gr][ch + 2],
                       sizeof(gfc->pinfo->energy[gr][ch]));
            }
        }
    }
}


/**
 * \internal
 * \brief Smooths the perceptual entropy over the frames, for CBR and ABR.
 *
 * A low-pass filter runs over the sum of each frame's entropy, and the
 * entropy of this frame is scaled by the result.
 *
 * \param gfc     the encoder, with the history of the sums.
 * \param pe_use  the perceptual entropy of the frame; it is scaled in place.
 */
static void
smooth_pe(lame_internal_flags * gfc, FLOAT pe_use[2][2])
{
    static FLOAT const fircoef[9] = {
        -0.0207887 * 5, -0.0378413 * 5, -0.0432472 * 5, -0.031183 * 5,
        7.79609e-18 * 5, 0.0467745 * 5, 0.10091 * 5, 0.151365 * 5,
        0.187098 * 5
    };
    SessionConfig_t const *const cfg = &gfc->cfg;
    int     ch, gr, i;
    FLOAT   f;

    for (i = 0; i < 18; i++)
        gfc->sv_enc.pefirbuf[i] = gfc->sv_enc.pefirbuf[i + 1];

    f = 0.0;
    for (gr = 0; gr < cfg->mode_gr; gr++)
        for (ch = 0; ch < cfg->channels_out; ch++)
            f += pe_use[gr][ch];
    gfc->sv_enc.pefirbuf[18] = f;

    f = gfc->sv_enc.pefirbuf[9];
    for (i = 0; i < 9; i++)
        f += (gfc->sv_enc.pefirbuf[i] + gfc->sv_enc.pefirbuf[18 - i]) * fircoef[i];

    if (f > 0) {
        f = (670 * 5 * cfg->mode_gr * cfg->channels_out) / f;
        for (gr = 0; gr < cfg->mode_gr; gr++) {
            for (ch = 0; ch < cfg->channels_out; ch++) {
                pe_use[gr][ch] *= f;
            }
        }
    }
}


/**
 * \internal
 * \brief Gives the frame analyzer the frame's input samples and noise.
 *
 * \param gfc      the encoder, with the analyzer's record.
 * \param inbuf    the input of the frame, per channel.
 * \param masking  the masking of the chosen channel mode.
 */
static void
copy_analysis_output(lame_internal_flags * gfc, const sample_t *const inbuf[2],
                     const III_psy_ratio (*masking)[2])
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    int const framesize = SAMPLES_PER_GRANULE * cfg->mode_gr;
    int     ch;

    for (ch = 0; ch < cfg->channels_out; ch++) {
        int     j;
        for (j = 0; j < FFTOFFSET; j++)
            gfc->pinfo->pcmdata[ch][j] = gfc->pinfo->pcmdata[ch][j + framesize];
        for (j = FFTOFFSET; j < 1600; j++) {
            gfc->pinfo->pcmdata[ch][j] = inbuf[ch][j - FFTOFFSET];
        }
    }
    gfc->sv_qnt.masking_lower = 1.0;

    set_frame_pinfo(gfc, masking);
}


/************************************************************************
*
* encodeframe()           Layer 3
*
* encode a single frame
*
************************************************************************
lame_encode_frame()


                       gr 0            gr 1
inbuf:           |--------------|--------------|--------------|


Polyphase (18 windows, each shifted 32)
gr 0:
window1          <----512---->
window18                 <----512---->

gr 1:
window1                         <----512---->
window18                                <----512---->



MDCT output:  |--------------|--------------|--------------|

FFT's                    <---------1024---------->
                                         <---------1024-------->



    inbuf = buffer of PCM data size=MP3 framesize
    encoder acts on inbuf[ch][0], but output is delayed by MDCTDELAY
    so the MDCT coefficints are from inbuf[ch][-MDCTDELAY]

    psy-model FFT has a 1 granule delay, so we feed it data for the 
    next granule.
    FFT is centered over granule:  224+576+224
    So FFT starts at:   576-224-MDCTDELAY

    MPEG2:  FFT ends at:  BLKSIZE+576-224-MDCTDELAY      (1328)
    MPEG1:  FFT ends at:  BLKSIZE+2*576-224-MDCTDELAY    (1904)

    MPEG2:  polyphase first window:  [0..511]
                      18th window:   [544..1055]          (1056)
    MPEG1:            36th window:   [1120..1631]         (1632)
            data needed:  512+framesize-32

    A close look newmdct.c shows that the polyphase filterbank
    only uses data from [0..510] for each window.  Perhaps because the window
    used by the filterbank is zero for the last point, so Takehiro's
    code doesn't bother to compute with it.

    FFT starts at 576-224-MDCTDELAY (304)  = 576-FFTOFFSET

*/


int
lame_encode_mp3_frame(       /* Output */
                         lame_internal_flags * gfc, /* Context */
                         sample_t const *inbuf_l, /* Input */
                         sample_t const *inbuf_r, /* Input */
                         unsigned char *mp3buf, /* Output */
                         int mp3buf_size)
{                       /* Output */
    SessionConfig_t const *const cfg = &gfc->cfg;
    int     mp3count;
    III_psy_ratio masking_LR[2][2]; /*LR masking & energy */
    III_psy_ratio masking_MS[2][2]; /*MS masking & energy */
    const III_psy_ratio (*masking)[2]; /*pointer to selected maskings */
    const sample_t *inbuf[2];

    FLOAT   ms_ener_ratio[2] = { .5, .5 };
    FLOAT   pe[2][2] = { {0., 0.}, {0., 0.} }, pe_MS[2][2] = { {
    0., 0.}, {
    0., 0.}};
    FLOAT (*pe_use)[2];

    inbuf[0] = inbuf_l;
    inbuf[1] = inbuf_r;

    if (gfc->sv_enc.internal_error)
        return LAME_INTERNALERROR;

    if (gfc->lame_encode_frame_init == 0) {
        /*first run? */
        lame_encode_frame_init(gfc, inbuf);

    }


    /********************** padding *****************************/
    /* padding method as described in 
     * "MPEG-Layer3 / Bitstream Syntax and Decoding"
     * by Martin Sieler, Ralph Sperschneider
     *
     * note: there is no padding for the very first frame
     *
     * Robert Hegemann 2000-06-22
     */
    gfc->ov_enc.padding = FALSE;
    if ((gfc->sv_enc.slot_lag -= gfc->sv_enc.frac_SpF) < 0) {
        gfc->sv_enc.slot_lag += cfg->samplerate_out;
        gfc->ov_enc.padding = TRUE;
    }



    /****************************************
    *   Stage 1: psychoacoustic model       *
    ****************************************/

    run_psymodel(gfc, inbuf, masking_LR, masking_MS, pe, pe_MS, ms_ener_ratio);


    /* auto-adjust of ATH, useful for low volume */
    adjust_ATH(gfc);


    /****************************************
    *   Stage 2: MDCT                       *
    ****************************************/

    /* polyphase filtering / mdct */
    mdct_sub48(gfc, inbuf[0], inbuf[1]);


    /****************************************
    *   Stage 3: MS/LR decision             *
    ****************************************/

    /* Here will be selected MS or LR coding of the 2 stereo channels */
    gfc->ov_enc.mode_ext = choose_mode_ext(gfc, pe, pe_MS);

    /* bit and noise allocation */
    if (gfc->ov_enc.mode_ext == MPG_MD_MS_LR) {
        masking = (const III_psy_ratio (*)[2])masking_MS; /* use MS masking */
        pe_use = pe_MS;
    }
    else {
        masking = (const III_psy_ratio (*)[2])masking_LR; /* use LR masking */
        pe_use = pe;
    }


    /* copy data for MP3 frame analyzer */
    if (cfg->analysis && gfc->pinfo != NULL)
        copy_analysis_input(gfc, ms_ener_ratio, pe_use);


    /****************************************
    *   Stage 4: quantization loop          *
    ****************************************/

    if (cfg->vbr == vbr_off || cfg->vbr == vbr_abr)
        smooth_pe(gfc, pe_use);
    switch (cfg->vbr)
    {
    default:
    case vbr_off:
        CBR_iteration_loop(gfc, (const FLOAT (*)[2])pe_use, ms_ener_ratio, masking);
        break;
    case vbr_abr:
        ABR_iteration_loop(gfc, (const FLOAT (*)[2])pe_use, ms_ener_ratio, masking);
        break;
    case vbr_rh:
        VBR_old_iteration_loop(gfc, (const FLOAT (*)[2])pe_use, ms_ener_ratio, masking);
        break;
    case vbr_mt:
    case vbr_mtrh:
        VBR_new_iteration_loop(gfc, (const FLOAT (*)[2])pe_use, ms_ener_ratio, masking);
        break;
    }
    if (gfc->sv_enc.internal_error)
        return LAME_INTERNALERROR;


    /****************************************
    *   Stage 5: bitstream formatting       *
    ****************************************/


    /*  write the frame to the bitstream  */
    if (format_bitstream(gfc) != 0)
        return LAME_INTERNALERROR;

    /* copy mp3 bit buffer into array */
    mp3count = copy_buffer(gfc, mp3buf, mp3buf_size, 1);


    if (cfg->write_lame_tag) {
        AddVbrFrame(gfc);
    }

    if (cfg->analysis && gfc->pinfo != NULL)
        copy_analysis_output(gfc, inbuf, masking);

    ++gfc->ov_enc.frame_number;

    updateStats(gfc);

    return mp3count;
}
