/* -*- mode: C; mode: fold -*- */
/*
 *      LAME MP3 encoding engine
 *
 *      Copyright (c) 1999-2000 Mark Taylor
 *      Copyright (c) 2000-2005 Takehiro Tominaga
 *      Copyright (c) 2000-2019 Robert Hegemann
 *      Copyright (c) 2000-2005 Gabriel Bouvigne
 *      Copyright (c) 2000-2004 Alexander Leidinger
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

/*!
  \file   lame.c
  \brief  The LAME MP3 encoding engine.

  The group \ref api_encoding describes the public functions of this file.
  Most of this file is the engine behind them and is not part of the
  interface.
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "lame.h"
#include "machine.h"

#include "encoder.h"
#include "util.h"
#include "lame_global_flags.h"
#include "gain_analysis.h"
#include "bitstream.h"
#include "quantize_pvt.h"
#include "set_get.h"
#include "obsolete_api.h"
#include "quantize.h"
#include "psymodel.h"
#include "version.h"
#include "VbrTag.h"
#include "tables.h"


#if defined(__FreeBSD__) && !defined(__alpha__)
#include <floatingpoint.h>
#endif
#ifdef __riscos__
#include "asmstuff.h"
#endif

#ifdef __sun__
/* woraround for SunOS 4.x, it has SEEK_* defined here */
#include <unistd.h>
#endif


#define LAME_DEFAULT_QUALITY 3



int
is_lame_global_flags_valid(const lame_global_flags * gfp)
{
    if (gfp == NULL)
        return 0;
    if (gfp->class_id != LAME_ID)
        return 0;
    return 1;
}


int
is_lame_internal_flags_valid(const lame_internal_flags * gfc)
{
    if (gfc == NULL)
        return 0;
    if (gfc->class_id != LAME_ID)
        return 0;
    if (gfc->lame_init_params_successful <=0)
        return 0;
    return 1;
}



static  FLOAT
filter_coef(FLOAT x)
{
    if (x > 1.0)
        return 0.0;
    if (x <= 0.0)
        return 1.0;

    return cos(PI / 2 * x);
}

static void
lame_init_params_ppflt(lame_internal_flags * gfc)
{
    SessionConfig_t *const cfg = &gfc->cfg;
    
    /***************************************************************/
    /* compute info needed for polyphase filter (filter type==0, default) */
    /***************************************************************/

    int     band, maxband, minband;
    FLOAT   freq;
    int     lowpass_band = 32;
    int     highpass_band = -1;

    if (cfg->lowpass1 > 0) {
        minband = 999;
        for (band = 0; band <= 31; band++) {
            freq = band / 31.0;
            /* this band and above will be zeroed: */
            if (freq >= cfg->lowpass2) {
                lowpass_band = Min(lowpass_band, band);
            }
            if (cfg->lowpass1 < freq && freq < cfg->lowpass2) {
                minband = Min(minband, band);
            }
        }

        /* compute the *actual* transition band implemented by
         * the polyphase filter */
        if (minband == 999) {
            cfg->lowpass1 = (lowpass_band - .75) / 31.0;
        }
        else {
            cfg->lowpass1 = (minband - .75) / 31.0;
        }
        cfg->lowpass2 = lowpass_band / 31.0;
    }

    /* make sure highpass filter is within 90% of what the effective
     * highpass frequency will be */
    if (cfg->highpass2 > 0) {
        if (cfg->highpass2 < .9 * (.75 / 31.0)) {
            cfg->highpass1 = 0;
            cfg->highpass2 = 0;
            MSGF(gfc, "Warning: highpass filter disabled.  " "highpass frequency too small\n");
        }
    }

    if (cfg->highpass2 > 0) {
        maxband = -1;
        for (band = 0; band <= 31; band++) {
            freq = band / 31.0;
            /* this band and below will be zereod */
            if (freq <= cfg->highpass1) {
                highpass_band = Max(highpass_band, band);
            }
            if (cfg->highpass1 < freq && freq < cfg->highpass2) {
                maxband = Max(maxband, band);
            }
        }
        /* compute the *actual* transition band implemented by
         * the polyphase filter */
        cfg->highpass1 = highpass_band / 31.0;
        if (maxband == -1) {
            cfg->highpass2 = (highpass_band + .75) / 31.0;
        }
        else {
            cfg->highpass2 = (maxband + .75) / 31.0;
        }
    }

    for (band = 0; band < 32; band++) {
        FLOAT fc1, fc2;
        freq = band / 31.0f;
        if (cfg->highpass2 > cfg->highpass1) {
            fc1 = filter_coef((cfg->highpass2 - freq) / (cfg->highpass2 - cfg->highpass1 + 1e-20));
        }
        else {
            fc1 = 1.0f;
        }
        if (cfg->lowpass2 > cfg->lowpass1) {
            fc2 = filter_coef((freq - cfg->lowpass1)  / (cfg->lowpass2 - cfg->lowpass1 + 1e-20));
        }
        else {
            fc2 = 1.0f;
        }
        gfc->sv_enc.amp_filter[band] = fc1 * fc2;
    }
}


static void
optimum_bandwidth(double *const lowerlimit, double *const upperlimit, const int bitrate)
{
/*
 *  Input:
 *      bitrate     total bitrate in kbps
 *
 *   Output:
 *      lowerlimit: best lowpass frequency limit for input filter in Hz
 *      upperlimit: best highpass frequency limit for input filter in Hz
 */
    int     table_index;

    /* the lowpass in Hz, one row for each entry of full_bitrate_table */
    const int lowpass_by_bitrate[] = {
        2000,                /*   8 kbps */
        3700,                /*  16 */
        3900,                /*  24 */
        5500,                /*  32 */
        7000,                /*  40 */
        7500,                /*  48 */
        10000,               /*  56 */
        11000,               /*  64 */
        13500,               /*  80 */
        15100,               /*  96 */
        15600,               /* 112 */
        17000,               /* 128 */
        17500,               /* 160 */
        18600,               /* 192 */
        19400,               /* 224 */
        19700,               /* 256 */
        20500                /* 320 */
    };
    compiletime_assert(dimension_of(lowpass_by_bitrate) == dimension_of(full_bitrate_table));

    table_index = nearestBitrateFullIndex(bitrate);

    *lowerlimit = lowpass_by_bitrate[table_index];


/*
 *  Now we try to choose a good high pass filtering frequency.
 *  This value is currently not used.
 *    For fu < 16 kHz:  sqrt(fu*fl) = 560 Hz
 *    For fu = 18 kHz:  no high pass filtering
 *  This gives:
 *
 *   2 kHz => 160 Hz
 *   3 kHz => 107 Hz
 *   4 kHz =>  80 Hz
 *   8 kHz =>  40 Hz
 *  16 kHz =>  20 Hz
 *  17 kHz =>  10 Hz
 *  18 kHz =>   0 Hz
 *
 *  These are ad hoc values and these can be optimized if a high pass is available.
 */
/*    if (f_low <= 16000)
        f_high = 16000. * 20. / f_low;
    else if (f_low <= 18000)
        f_high = 180. - 0.01 * f_low;
    else
        f_high = 0.;*/

    /*
     *  When we sometimes have a good highpass filter, we can add the highpass
     *  frequency to the lowpass frequency
     */

    /*if (upperlimit != NULL)
     *upperlimit = f_high;*/
    (void) upperlimit;
}


/**
 * \internal
 * \brief Reads one signed 6-bit field of the exp_nspsytune value.
 * \param nsp    the packed value.
 * \param shift  the position of the lowest bit of the field.
 * \return the field in dB, in steps of a quarter dB.
 */
static float
nspsytune_db(int nsp, int shift)
{
    int     field = (nsp >> shift) & 63;
    if (field >= 32)
        field -= 64;
    return field * 0.25f;
}

static int
optimum_samplefreq(int lowpassfreq, int input_samplefreq)
{
/*
 * Rules:
 *  - if possible, sfb21 should NOT be used
 *
 */
    int     suggested_samplefreq = floorMP3Frequency(input_samplefreq);

    if (suggested_samplefreq == 0)
        suggested_samplefreq = 44100;

    if (lowpassfreq == -1)
        return suggested_samplefreq;

    if (lowpassfreq <= 15960)
        suggested_samplefreq = 44100;
    if (lowpassfreq <= 15250)
        suggested_samplefreq = 32000;
    if (lowpassfreq <= 11220)
        suggested_samplefreq = 24000;
    if (lowpassfreq <= 9970)
        suggested_samplefreq = 22050;
    if (lowpassfreq <= 7230)
        suggested_samplefreq = 16000;
    if (lowpassfreq <= 5420)
        suggested_samplefreq = 12000;
    if (lowpassfreq <= 4510)
        suggested_samplefreq = 11025;
    if (lowpassfreq <= 3970)
        suggested_samplefreq = 8000;

    if (input_samplefreq < suggested_samplefreq) {
        /* choose a valid MPEG sample frequency above the input sample frequency
           to avoid SFB21/12 bitrate bloat
           rh 061115
         */
        return map2MP3Frequency(input_samplefreq);
    }
    return suggested_samplefreq;
}





/* set internal feature flags.  USER should not access these since
 * some combinations will produce strange results */
static void
lame_init_qval(lame_global_flags * gfp)
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    SessionConfig_t *const cfg = &gfc->cfg;

    /* One row for each quality setting, 0 (best) to 9 (fastest).
       noise_shaping: 1 turns it on, unless the caller chose a level, and
       sets subblock_gain if the caller left it unset; 0 turns it off.
       substep: the substep_shaping, if the caller left it at 0. */
    static const struct {
        signed char noise_shaping, substep, amp, stop, best_huffman, full_outer_loop;
    } qval[10] = {
        /*    ns  substep amp stop huffman outer */
        /* 0 */ {1, 2, 2, 1, 1, 1},  /* type 2 huffman left out for its slowness, in favor of the full outer loop search */
        /* 1 */ {1, 2, 2, 1, 1, 0},
        /* 2 */ {1, 2, 1, 1, 1, 0},
        /* 3 */ {1, 0, 1, 1, 1, 0},
        /* 4 */ {1, 0, 0, 0, 1, 0},
        /* 5 */ {1, 0, 0, 0, 0, 0},
        /* 6 */ {1, 0, 0, 0, 0, 0},
        /* 7 */ {0, 0, 0, 0, 0, 0},  /* psymodel for short blocks and m/s switching, no noise shaping */
        /* 8 */ {0, 0, 0, 0, 0, 0},  /* not used: 8 becomes 7 */
        /* 9 */ {0, 0, 0, 0, 0, 0}   /* no psymodel, no noise shaping */
    };
    int     q = gfp->quality;

    if (q < 0 || q > 9)
        q = 9;
    if (q == 8) {
        gfp->quality = 7;
        q = 7;
    }
    if (qval[q].noise_shaping) {
        if (cfg->noise_shaping == 0)
            cfg->noise_shaping = 1;
        if (cfg->subblock_gain == -1)
            cfg->subblock_gain = 1;
    }
    else {
        cfg->noise_shaping = 0;
    }
    if (qval[q].substep != 0 && gfc->sv_qnt.substep_shaping == 0)
        gfc->sv_qnt.substep_shaping = qval[q].substep;
    cfg->noise_shaping_amp = qval[q].amp;
    cfg->noise_shaping_stop = qval[q].stop;
    cfg->use_best_huffman = qval[q].best_huffman;
    cfg->full_outer_loop = qval[q].full_outer_loop;
    if (q == 7 && (cfg->vbr == vbr_mt || cfg->vbr == vbr_mtrh)) {
        cfg->full_outer_loop = -1;
    }

    /*  Amplified noise shaping degrades CBR and ABR instead of improving them,
     *  which is why the faster quality settings, the ones that leave it off,
     *  sound better than the slower ones. It dates from the point where CBR
     *  and ABR were switched over to the newer VBR psychoacoustic model
     *  without adapting their encoding loops to it, so the amplification
     *  decides against a distortion measure that no longer means what it did.
     *  VBR is unaffected.
     *
     *  Until those loops are reconciled with the psychoacoustic model, keep
     *  the amplification out of the modes it hurts. This treats the symptom;
     *  the interaction itself is still to be repaired.
     */
    if (cfg->vbr == vbr_off || cfg->vbr == vbr_abr) {
        cfg->noise_shaping_amp = NS_AMP_ISO;
    }
}



static double
linear_int(double a, double b, double m)
{
    return a + m * (b - a);
}


/**
 * \internal
 * \brief Returns the default lowpass of a VBR quality.
 *
 * \param x           the lowpass in Hz of the qualities 0 to 10.
 * \param fallback    the lowpass for a quality outside 0 to 9.
 * \param VBR_q       the quality.
 * \param VBR_q_frac  the fraction of the quality, towards the next one.
 * \return the value of \a x at \a VBR_q, interpolated towards the next by
 *         \a VBR_q_frac.
 */
static double
default_lowpass(int const x[11], double fallback, int VBR_q, float VBR_q_frac)
{
    if (0 <= VBR_q && VBR_q <= 9) {
        double  a = x[VBR_q], b = x[VBR_q + 1], m = VBR_q_frac;
        return linear_int(a, b, m);
    }
    return fallback;
}


/* A frame carries its side information and then whatever is left, shared
   between its granules. Below this many bits per granule there is not enough
   left to place a granule at all, whatever the audio is: the bit allocation
   goes negative and the quantization loop cannot meet its target.

   Eight is measured rather than chosen. It is the boundary in every one of the
   28 combinations of MPEG version, sample rate, channel count and CRC that
   were tried, identically for near-silent and for full-scale material, and it
   is also exactly what the tightest configuration the standard's bitrate
   tables allow - MPEG-2 at 24 kHz and 8 kbit/s, two channels with CRC - leaves
   over. So it cannot be raised without refusing a stream that is legal and
   encodes correctly today. */
#define MIN_MEAN_BITS_PER_GRANULE 8

/* The bits a frame at this bitrate leaves for audio, per granule. Padding is
   not counted: a padded frame is one byte larger, so leaving it out asks the
   question of the smaller of the two frame sizes, which is the one that has to
   work. */
static int
mean_bits_per_granule(SessionConfig_t const *const cfg, int kbps)
{
    int const frame_bits = 8 * ((cfg->version + 1) * 72000 * kbps
                                / cfg->samplerate_out);

    return (frame_bits - 8 * cfg->sideinfo_len) / cfg->mode_gr;
}

static int
frame_has_room_for_audio(SessionConfig_t const *const cfg)
{
    return mean_bits_per_granule(cfg, cfg->avg_bitrate)
        >= MIN_MEAN_BITS_PER_GRANULE;
}

/* The lowest bitrate that would work for this sample rate and channel count,
   for a caller who has asked for one that does not, or 0 if the format has
   none. */
static int
lowest_usable_free_format_bitrate(SessionConfig_t const *const cfg)
{
    int     kbps;

    for (kbps = 8; kbps <= MAX_FREE_FORMAT_KBPS; ++kbps) {
        if (mean_bits_per_granule(cfg, kbps) >= MIN_MEAN_BITS_PER_GRANULE)
            return kbps;
    }
    return 0;
}



/********************************************************************
 *   initialize internal params based on data in gf
 *   (globalflags struct filled in by calling program)
 *
 *  OUTLINE:
 *
 * We first have some complex code to determine bitrate,
 * output samplerate and mode.  It is complicated by the fact
 * that we allow the user to set some or all of these parameters,
 * and need to determine best possible values for the rest of them:
 *
 *  1. set some CPU related flags
 *  2. check if we are mono->mono, stereo->mono or stereo->stereo
 *  3.  compute bitrate and output samplerate:
 *          user may have set compression ratio
 *          user may have set a bitrate
 *          user may have set a output samplerate
 *  4. set some options which depend on output samplerate
 *  5. compute the actual compression ratio
 *  6. set mode based on compression ratio
 *
 *  The remaining code is much simpler - it just sets options
 *  based on the mode & compression ratio:
 *
 *   set allow_diff_short based on mode
 *   select lowpass filter based on compression ratio & mode
 *   set the bitrate index, and min/max bitrates for VBR modes
 *   disable VBR tag if it is not appropriate
 *   initialize the bitstream
 *   initialize scalefac_band data
 *   set sideinfo_len (based on channels, CRC, out_samplerate)
 *   write an id3v2 tag into the bitstream
 *   write VBR tag into the bitstream
 *   set mpeg1/2 flag
 *   estimate the number of frames (based on a lot of data)
 *
 *   now we set more flags:
 *   nspsytune:
 *      see code
 *   VBR modes
 *      see code
 *   CBR/ABR
 *      see code
 *
 *  Finally, we set the algorithm flags based on the gfp->quality value
 *  lame_init_qval(gfp);
 *
 ********************************************************************/
static int init_params(lame_global_flags * gfp);

/*! Validate the settings and prepare the encoder for use. */
/*!
  \ingroup api_encoding
  Call it once: after \c lame_init() and after all the settings the caller
  wants to change, and before the first \c lame_encode_buffer(). It checks the
  settings and computes everything that was left at its default. It allocates
  the encoding buffers and writes the tags at the start of the stream (through
  \c lame_init_bitstream()).

  Each setter checks only its own value. This function reports a bad
  combination of settings. Every parameter has a default. So an instance
  straight from \c lame_init() initializes without an error, as 44.1 kHz
  stereo input in CD format. A caller that forgets to describe its input gets
  a wrong encode, not an error.

  A second call on the same instance fails and returns -1. So the parameters
  cannot be changed after this call. For different settings, create a new
  instance.

  \param gfp the encoder instance from \c lame_init().
  \retval 0  success.
  \retval -1 the settings cannot be used: the input sample rate, the output
             sample rate or the number of channels makes no sense, a buffer
             could not be allocated, or the instance was already initialized.
             The instance stays valid after a failure, and its settings are
             as they were before the call. The caller can change them and call
             this function again, or release the instance with
             \c lame_close().
*/
int
lame_init_params(lame_global_flags * gfp)
{
    lame_internal_flags *gfc;
    lame_global_flags settings;
    SessionConfig_t config;
    int     ret;

    if (!is_lame_global_flags_valid(gfp))
        return -1;

    gfc = gfp->internal_flags;
    if (gfc == 0)
        return -1;

    if (is_lame_internal_flags_valid(gfc))
        return -1; /* already initialized */

    /* lame_set_preset() writes some of the session configuration as well */
    settings = *gfp;
    config = gfc->cfg;
    ret = init_params(gfp);
    if (ret != 0) {
        *gfp = settings;
        gfc->cfg = config;
        free_init_state(gfc);
    }
    return ret;
}

/*! The body of \c lame_init_params(), for an instance that is not
    initialized yet. */
/*!
  \internal
  It computes the session configuration from the caller's settings and writes
  the values it settled on back into them. It builds the tables and buffers
  of the encoder.

  \param gfp the encoder instance. Its internal flags exist.
  \return 0 on success. -1 if the settings cannot be used or a buffer could
          not be allocated; the settings and the tables may then be changed
          in part.
*/
static int
init_params(lame_global_flags * gfp)
{

    int     i;
    int     j;
    lame_internal_flags *const gfc = gfp->internal_flags;
    SessionConfig_t *cfg;

    /* start updating lame internal flags */
    gfc->class_id = LAME_ID;
    gfc->lame_init_params_successful = 0; /* will be set to one, when we get through until the end */

    if (gfp->samplerate_in < 1)
        return -1; /* input sample rate makes no sense */
    if (gfp->num_channels < 1 || 2 < gfp->num_channels)
        return -1; /* number of input channels makes no sense */
    if (gfp->samplerate_out != 0) {
        int   v=0;
        if (SmpFrqIndex(gfp->samplerate_out, &v) < 0)
            return -1; /* output sample rate makes no sense */
    }

    cfg = &gfc->cfg;

    cfg->enforce_min_bitrate = gfp->VBR_hard_min;
    cfg->analysis = gfp->analysis;
    if (cfg->analysis)
        gfp->write_lame_tag = 0;

    /* some file options not allowed if output is: not specified or stdout */
    if (gfc->pinfo != NULL)
        gfp->write_lame_tag = 0; /* disable Xing VBR tag */

    /* report functions */
    gfc->report_msg = gfp->report.msgf;
    gfc->report_dbg = gfp->report.debugf;
    gfc->report_err = gfp->report.errorf;

    /* A request for no vector routines is also a request not to ask the
       processor about them.  On x86 these probes are the library's only use
       of the CPUID instruction, and there are machines - the reason the
       option is
       documented as a rescue for a failing start-up - where executing it is
       itself the failure, so a caller who has already decided against the
       vector code must be able to stop the question being asked.  Under AUTO
       the answer is what chooses the set, so there it has to be asked. */
    if (gfp->vector_routines_request != VECTOR_IMPL_NONE
        && gfp->asm_optimizations.sse) {
        gfc->CPU_features.SSE2 = has_SSE2();
        /* AVX2 sits on top of SSE2, so it can be disabled on its own while the
           SSE2 tier stays; disabling SSE removes the whole group above. */
        gfc->CPU_features.AVX2 = gfp->asm_optimizations.avx2 ? has_AVX2() : 0;
        /* The deprecated mask has no name for AVX-512 and must not grow one -
           that is what lame_set_vector_routines() is for - so the rung above
           inherits the AVX2 bit rather than being unreachable from the old
           API or, worse, immune to it: a caller of the old API asking for no
           AVX2 is asking for nothing wider than SSE2. */
        gfc->CPU_features.AVX512 = gfp->asm_optimizations.avx2 ? has_AVX512() : 0;
    }
    else {
        gfc->CPU_features.SSE2 = 0;
        gfc->CPU_features.AVX2 = 0;
        gfc->CPU_features.AVX512 = 0;
    }

    /* Set apart from the block above because the deprecated mask has no say
       over it: that enumeration names x86 instruction-set families, and
       teaching it a word for this one is precisely what
       lame_set_vector_routines() exists to avoid.  So the ARM rung answers to
       the request alone - which still means none turns it off, and still
       means none asks the operating system nothing. */
    gfc->CPU_features.NEON =
        (gfp->vector_routines_request != VECTOR_IMPL_NONE) ? has_NEON() : 0;

    /* One decision, here, before anything asks.  Every consumer of
       vector_implementation() - the Huffman, quantization and FFT init and
       the encode loops - runs later than this point. */
    vector_impl_init(gfc, gfp->vector_routines_request);


    cfg->vbr = gfp->VBR;
    cfg->error_protection = gfp->error_protection;
    cfg->copyright = gfp->copyright;
    cfg->original = gfp->original;
    cfg->extension = gfp->extension;
    cfg->emphasis = gfp->emphasis;

    cfg->channels_in = gfp->num_channels;
    if (cfg->channels_in == 1)
        gfp->mode = MONO;
    cfg->channels_out = (gfp->mode == MONO) ? 1 : 2;
    if (gfp->mode != JOINT_STEREO)
        gfp->force_ms = 0; /* forced mid/side stereo for j-stereo only */
    cfg->force_ms = gfp->force_ms;

    if (cfg->vbr == vbr_off && gfp->VBR_mean_bitrate_kbps != 128 && gfp->brate == 0)
        gfp->brate = gfp->VBR_mean_bitrate_kbps;

    switch (cfg->vbr) {
    case vbr_off:
    case vbr_mtrh:
    case vbr_mt:
        /* these modes can handle free format condition */
        break;
    default:
        gfp->free_format = 0; /* mode can't be mixed with free format */
        break;
    }

    cfg->free_format = gfp->free_format;

    if (cfg->vbr == vbr_off && gfp->brate == 0) {
        /* no bitrate or compression ratio specified, use 11.025 */
        if (EQ(gfp->compression_ratio, 0.0))
            gfp->compression_ratio = 11.025; /* rate to compress a CD down to exactly 128000 bps */
    }

    /* find bitrate if user specify a compression ratio */
    if (cfg->vbr == vbr_off && gfp->compression_ratio > 0) {

        if (gfp->samplerate_out == 0)
            gfp->samplerate_out = map2MP3Frequency((int) (0.97 * gfp->samplerate_in)); /* round up with a margin of 3% */

        /* choose a bitrate for the output samplerate which achieves
         * specified compression ratio
         */
        {
            /* a small enough ratio asks for more kbit/s than an int holds;
               cap it where bits per second still fit */
            double const kbps = gfp->samplerate_out * 16 * cfg->channels_out / (1.e3 * gfp->compression_ratio);
            gfp->brate = kbps < INT_MAX / 1000 ? (int) kbps : INT_MAX / 1000;
        }

        /* we need the version for the bitrate table look up */
        cfg->samplerate_index = SmpFrqIndex(gfp->samplerate_out, &cfg->version);
        assert(cfg->samplerate_index >=0);

        if (!cfg->free_format) /* for non Free Format find the nearest allowed bitrate */
            gfp->brate = FindNearestBitrate(gfp->brate, cfg->version, gfp->samplerate_out);
    }
    /* A free format bitrate above MAX_FREE_FORMAT_KBPS is encoded at that limit.
       A negative one becomes 0, which the check for room in a frame refuses. */
    if (cfg->free_format)
        gfp->brate = Max(0, Min(gfp->brate, MAX_FREE_FORMAT_KBPS));
    if (gfp->samplerate_out) {
        if (gfp->samplerate_out < 16000) {
            gfp->VBR_mean_bitrate_kbps = Max(gfp->VBR_mean_bitrate_kbps, 8);
            gfp->VBR_mean_bitrate_kbps = Min(gfp->VBR_mean_bitrate_kbps, 64);
        }
        else if (gfp->samplerate_out < 32000) {
            gfp->VBR_mean_bitrate_kbps = Max(gfp->VBR_mean_bitrate_kbps, 8);
            gfp->VBR_mean_bitrate_kbps = Min(gfp->VBR_mean_bitrate_kbps, 160);
        }
        else {
            gfp->VBR_mean_bitrate_kbps = Max(gfp->VBR_mean_bitrate_kbps, 32);
            gfp->VBR_mean_bitrate_kbps = Min(gfp->VBR_mean_bitrate_kbps, 320);
        }
    }
    /* WORK IN PROGRESS */
    /* mapping VBR scale to internal VBR quality settings */
    if (gfp->samplerate_out == 0 && (cfg->vbr == vbr_mt || cfg->vbr == vbr_mtrh)) {
        float const qval = gfp->VBR_q + gfp->VBR_q_frac;
        struct q_map { int sr_a; float qa, qb, ta, tb; };
        struct q_map const m[9]
        = { {48000, 0.0,6.5,  0.0,6.5}
          , {44100, 0.0,6.5,  0.0,6.5}
          , {32000, 6.5,8.0,  5.2,6.5}
          , {24000, 8.0,8.5,  5.2,6.0}
          , {22050, 8.5,9.01, 5.2,6.5}
          , {16000, 9.01,9.4, 4.9,6.5}
          , {12000, 9.4,9.6,  4.5,6.0}
          , {11025, 9.6,9.9,  5.1,6.5}
          , { 8000, 9.9,10.,  4.9,6.5}
        };
        for (i = 2; i < (int) dimension_of(m); ++i) {
            if (gfp->samplerate_in == m[i].sr_a) {
                if (qval < m[i].qa) {
                    double d = qval / m[i].qa;
                    d = d * m[i].ta;
                    gfp->VBR_q = (int)d;
                    gfp->VBR_q_frac = d - gfp->VBR_q;
                }
            }
            if (gfp->samplerate_in >= m[i].sr_a) {
                if (m[i].qa <= qval && qval < m[i].qb) {
                    float const q_ = m[i].qb-m[i].qa;
                    float const t_ = m[i].tb-m[i].ta;
                    double d = m[i].ta + t_ * (qval-m[i].qa) / q_;
                    gfp->VBR_q = (int)d;
                    gfp->VBR_q_frac = d - gfp->VBR_q;
                    gfp->samplerate_out = m[i].sr_a;
                    if (gfp->lowpassfreq == 0) {
                        gfp->lowpassfreq = -1;
                    }
                    break;
                }
            }
        }
    }

    /****************************************************************/
    /* if a filter has not been enabled, see if we should add one: */
    /****************************************************************/
    if (gfp->lowpassfreq == 0) {
        double  lowpass = 16000;
        double  highpass;

        switch (cfg->vbr) {
        case vbr_off:{
                optimum_bandwidth(&lowpass, &highpass, gfp->brate);
                break;
            }
        case vbr_abr:{
                optimum_bandwidth(&lowpass, &highpass, gfp->VBR_mean_bitrate_kbps);
                break;
            }
        case vbr_rh:{
                int const x[11] = {
                    19500, 19000, 18600, 18000, 17500, 16000, 15600, 14900, 12500, 10000, 3950
                };
                lowpass = default_lowpass(x, 19500, gfp->VBR_q, gfp->VBR_q_frac);
                break;
            }
        case vbr_mtrh:
        case vbr_mt:{
                int const x[11] = {
                    24000, 19500, 18500, 18000, 17500, 17000, 16500, 15600, 15200, 7230, 3950
                };
                lowpass = default_lowpass(x, 21500, gfp->VBR_q, gfp->VBR_q_frac);
                break;
            }
        default:{
                int const x[11] = {
                    19500, 19000, 18500, 18000, 17500, 16500, 15500, 14500, 12500, 9500, 3950
                };
                lowpass = default_lowpass(x, 19500, gfp->VBR_q, gfp->VBR_q_frac);
            }
        }

        if (gfp->mode == MONO && (cfg->vbr == vbr_off || cfg->vbr == vbr_abr))
            lowpass *= 1.5;

        gfp->lowpassfreq = lowpass;
    }

    if (gfp->samplerate_out == 0) {
        if (gfp->lowpassfreq > gfp->samplerate_in / 2) {
            gfp->lowpassfreq = gfp->samplerate_in / 2;
        }
        gfp->samplerate_out = optimum_samplefreq((int) gfp->lowpassfreq, gfp->samplerate_in);
    }
    if (cfg->vbr == vbr_mt || cfg->vbr == vbr_mtrh) {
        gfp->lowpassfreq = Min(24000, gfp->lowpassfreq);
    }
    else {
        gfp->lowpassfreq = Min(20500, gfp->lowpassfreq);
    }
    gfp->lowpassfreq = Min(gfp->samplerate_out / 2, gfp->lowpassfreq);

    if (cfg->vbr == vbr_off && gfp->brate > 0) {
        gfp->compression_ratio = gfp->samplerate_out * 16 * cfg->channels_out / (1.e3 * gfp->brate);
    }
    if (cfg->vbr == vbr_abr) {
        gfp->compression_ratio =
            gfp->samplerate_out * 16 * cfg->channels_out / (1.e3 * gfp->VBR_mean_bitrate_kbps);
    }

    cfg->disable_reservoir = gfp->disable_reservoir;
    cfg->lowpassfreq = gfp->lowpassfreq;
    cfg->highpassfreq = gfp->highpassfreq;
    cfg->samplerate_in = gfp->samplerate_in;
    cfg->samplerate_out = gfp->samplerate_out;
    /* What an encode call and the flush return grows with the upsampling
       ratio; past this one the buffer sizes lame.h gives no longer hold. */
    if (cfg->samplerate_out > MAX_UPSAMPLING_RATIO * (double) cfg->samplerate_in) {
        ERRORF(gfc, "Error: an input sample rate of %d Hz cannot be resampled to %d Hz,"
               " more than %d times as high\n", cfg->samplerate_in, cfg->samplerate_out,
               MAX_UPSAMPLING_RATIO);
        return -1;
    }
    cfg->mode_gr = cfg->samplerate_out <= 24000 ? 1 : 2; /* Number of granules per frame */


    /*
     *  sample freq       bitrate     compression ratio
     *     [kHz]      [kbps/channel]   for 16 bit input
     *     44.1            56               12.6
     *     44.1            64               11.025
     *     44.1            80                8.82
     *     22.05           24               14.7
     *     22.05           32               11.025
     *     22.05           40                8.82
     *     16              16               16.0
     *     16              24               10.667
     *
     */
    /*
     *  For VBR, take a guess at the compression_ratio.
     *  For example:
     *
     *    VBR_q    compression     like
     *     -        4.4         320 kbps/44 kHz
     *   0...1      5.5         256 kbps/44 kHz
     *     2        7.3         192 kbps/44 kHz
     *     4        8.8         160 kbps/44 kHz
     *     6       11           128 kbps/44 kHz
     *     9       14.7          96 kbps
     *
     *  for lower bitrates, downsample with --resample
     */

    assert(gfp->VBR_q <= 9);
    assert(gfp->VBR_q >= 0);
    switch (cfg->vbr) {
    case vbr_mt:
    case vbr_rh:
    case vbr_mtrh:
        {
            /*numbers are a bit strange, but they determine the lowpass value */
            FLOAT const cmp[] = { 5.7, 6.5, 7.3, 8.2, 10, 11.9, 13, 14, 15, 16.5 };
            gfp->compression_ratio = cmp[gfp->VBR_q];
        }
        break;
    case vbr_abr:
        gfp->compression_ratio =
            cfg->samplerate_out * 16 * cfg->channels_out / (1.e3 * gfp->VBR_mean_bitrate_kbps);
        break;
    default:
        if (gfp->brate > 0)
            gfp->compression_ratio = cfg->samplerate_out * 16 * cfg->channels_out / (1.e3 * gfp->brate);
        break;
    }


    /* mode = -1 (not set by user) or
     * mode = MONO (because of only 1 input channel).
     * If mode has not been set, then select J-STEREO
     */
    if (gfp->mode == NOT_SET) {
        gfp->mode = JOINT_STEREO;
    }

    cfg->mode = gfp->mode;


    /* apply user driven high pass filter */
    if (cfg->highpassfreq > 0) {
        cfg->highpass1 = 2. * cfg->highpassfreq;

        if (gfp->highpasswidth >= 0)
            cfg->highpass2 = 2. * (cfg->highpassfreq + gfp->highpasswidth);
        else            /* 0% above on default */
            cfg->highpass2 = (1 + 0.00) * 2. * cfg->highpassfreq;

        cfg->highpass1 /= cfg->samplerate_out;
        cfg->highpass2 /= cfg->samplerate_out;
    }
    else {
        cfg->highpass1 = 0;
        cfg->highpass2 = 0;
    }
    /* apply user driven low pass filter */
    cfg->lowpass1 = 0;
    cfg->lowpass2 = 0;
    if (cfg->lowpassfreq > 0 && cfg->lowpassfreq < (cfg->samplerate_out / 2) ) {
        cfg->lowpass2 = 2. * cfg->lowpassfreq;
        if (gfp->lowpasswidth >= 0) {
            cfg->lowpass1 = 2. * (cfg->lowpassfreq - gfp->lowpasswidth);
            if (cfg->lowpass1 < 0) /* has to be >= 0 */
                cfg->lowpass1 = 0;
        }
        else {          /* 0% below on default */
            cfg->lowpass1 = (1 - 0.00) * 2. * cfg->lowpassfreq;
        }
        cfg->lowpass1 /= cfg->samplerate_out;
        cfg->lowpass2 /= cfg->samplerate_out;
    }




  /**********************************************************************/
    /* compute info needed for polyphase filter (filter type==0, default) */
  /**********************************************************************/
    lame_init_params_ppflt(gfc);


  /*******************************************************
   * samplerate and bitrate index
   *******************************************************/
    cfg->samplerate_index = SmpFrqIndex(cfg->samplerate_out, &cfg->version);
    assert(cfg->samplerate_index >= 0);

    if (cfg->vbr == vbr_off) {
        if (cfg->free_format) {
            gfc->ov_enc.bitrate_index = 0;
        }
        else {
            gfp->brate = FindNearestBitrate(gfp->brate, cfg->version, cfg->samplerate_out);
            gfc->ov_enc.bitrate_index = BitrateIndex(gfp->brate, cfg->version, cfg->samplerate_out);
        }
    }
    else {
        gfc->ov_enc.bitrate_index = 1;
    }

    if (init_bit_stream_w(gfc) != 0)
        return -1;

    j = cfg->samplerate_index + (3 * cfg->version) + 6 * (cfg->samplerate_out < 16000);
    for (i = 0; i < SBMAX_l + 1; i++)
        gfc->scalefac_band.l[i] = sfBandIndex[j].l[i];

    for (i = 0; i < PSFB21 + 1; i++) {
        int const size = (gfc->scalefac_band.l[22] - gfc->scalefac_band.l[21]) / PSFB21;
        int const start = gfc->scalefac_band.l[21] + i * size;
        gfc->scalefac_band.psfb21[i] = start;
    }
    gfc->scalefac_band.psfb21[PSFB21] = SAMPLES_PER_GRANULE;

    for (i = 0; i < SBMAX_s + 1; i++)
        gfc->scalefac_band.s[i] = sfBandIndex[j].s[i];

    for (i = 0; i < PSFB12 + 1; i++) {
        int const size = (gfc->scalefac_band.s[13] - gfc->scalefac_band.s[12]) / PSFB12;
        int const start = gfc->scalefac_band.s[12] + i * size;
        gfc->scalefac_band.psfb12[i] = start;
    }
    gfc->scalefac_band.psfb12[PSFB12] = 192;

    /* determine the mean bitrate for main data */
    if (cfg->mode_gr == 2) /* MPEG 1 */
        cfg->sideinfo_len = (cfg->channels_out == 1) ? 4 + 17 : 4 + 32;
    else                /* MPEG 2 */
        cfg->sideinfo_len = (cfg->channels_out == 1) ? 4 + 9 : 4 + 17;

    if (cfg->error_protection)
        cfg->sideinfo_len += 2;

    {
        int     k;

        for (k = 0; k < 19; k++)
            gfc->sv_enc.pefirbuf[k] = 700 * cfg->mode_gr * cfg->channels_out;

    }

    switch (cfg->vbr) {

    case vbr_mt:
    case vbr_mtrh:{
            if (gfp->strict_ISO < 0) {
                gfp->strict_ISO = MDB_MAXIMUM;
            }
            if (gfp->useTemporal < 0) {
                gfp->useTemporal = 0; /* off by default for this VBR mode */
            }

            (void) apply_preset(gfp, V0 - 10 * gfp->VBR_q, 0);
            /*  The newer VBR code supports only a limited
               subset of quality levels:
               9-5=5 are the same, uses x^3/4 quantization
               4-0=0 are the same  5 plus best huffman divide code
             */
            if (gfp->quality < 0)
                gfp->quality = LAME_DEFAULT_QUALITY;
            if (gfp->quality < 5)
                gfp->quality = 0;
            if (gfp->quality > 7)
                gfp->quality = 7;

            /*  sfb21 extra only with MPEG-1 at higher sampling rates
             */
            if (gfp->experimentalY)
                gfc->sv_qnt.sfb21_extra = 0;
            else
                gfc->sv_qnt.sfb21_extra = (cfg->samplerate_out > 44000);

            break;

        }
    case vbr_rh:{

            (void) apply_preset(gfp, V0 - 10 * gfp->VBR_q, 0);

            /*  sfb21 extra only with MPEG-1 at higher sampling rates
             */
            if (gfp->experimentalY)
                gfc->sv_qnt.sfb21_extra = 0;
            else
                gfc->sv_qnt.sfb21_extra = (cfg->samplerate_out > 44000);

            /*  VBR needs at least the output of GPSYCHO,
             *  so we have to garantee that by setting a minimum
             *  quality level, actually level 6 does it.
             *  down to level 6
             */
            if (gfp->quality > 6)
                gfp->quality = 6;


            if (gfp->quality < 0)
                gfp->quality = LAME_DEFAULT_QUALITY;

            break;
        }

    default:           /* cbr/abr */  {

            /*  no sfb21 extra with CBR code
             */
            gfc->sv_qnt.sfb21_extra = 0;

            if (gfp->quality < 0)
                gfp->quality = LAME_DEFAULT_QUALITY;


            if (cfg->vbr == vbr_off)
                (void) lame_set_VBR_mean_bitrate_kbps(gfp, gfp->brate);
            /* second, set parameters depending on bitrate */
            (void) apply_preset(gfp, gfp->VBR_mean_bitrate_kbps, 0);
            gfp->VBR = cfg->vbr;

            break;
        }
    }

    /*initialize default values common for all modes */

    gfc->sv_qnt.mask_adjust = gfp->maskingadjust;
    gfc->sv_qnt.mask_adjust_short = gfp->maskingadjust_short;

    /*  just another daily changing developer switch  */
    if (gfp->tune) {
        gfc->sv_qnt.mask_adjust += gfp->tune_value_a;
        gfc->sv_qnt.mask_adjust_short += gfp->tune_value_a;
    }


    if (cfg->vbr != vbr_off) { /* choose a min/max bitrate for VBR */
        /* if the user didn't specify VBR_max_bitrate: */
        cfg->vbr_min_bitrate_index = 1; /* default: allow   8 kbps (MPEG-2) or  32 kbps (MPEG-1) */
        cfg->vbr_max_bitrate_index = 14; /* default: allow 160 kbps (MPEG-2) or 320 kbps (MPEG-1) */
        if (cfg->samplerate_out < 16000)
            cfg->vbr_max_bitrate_index = 8; /* default: allow 64 kbps (MPEG-2.5) */
        if (gfp->VBR_min_bitrate_kbps) {
            gfp->VBR_min_bitrate_kbps =
                FindNearestBitrate(gfp->VBR_min_bitrate_kbps, cfg->version, cfg->samplerate_out);
            cfg->vbr_min_bitrate_index =
                BitrateIndex(gfp->VBR_min_bitrate_kbps, cfg->version, cfg->samplerate_out);
        }
        if (gfp->VBR_max_bitrate_kbps) {
            gfp->VBR_max_bitrate_kbps =
                FindNearestBitrate(gfp->VBR_max_bitrate_kbps, cfg->version, cfg->samplerate_out);
            cfg->vbr_max_bitrate_index =
                BitrateIndex(gfp->VBR_max_bitrate_kbps, cfg->version, cfg->samplerate_out);
        }
        gfp->VBR_min_bitrate_kbps = bitrate_table[cfg->version][cfg->vbr_min_bitrate_index];
        gfp->VBR_max_bitrate_kbps = bitrate_table[cfg->version][cfg->vbr_max_bitrate_index];
        if (cfg->vbr_min_bitrate_index > cfg->vbr_max_bitrate_index) {
            ERRORF(gfc, "Error: the minimum bitrate of %d kbps is above the maximum of %d kbps\n",
                   gfp->VBR_min_bitrate_kbps, gfp->VBR_max_bitrate_kbps);
            return -1;
        }
        gfp->VBR_mean_bitrate_kbps =
            Min(bitrate_table[cfg->version][cfg->vbr_max_bitrate_index],
                gfp->VBR_mean_bitrate_kbps);
        gfp->VBR_mean_bitrate_kbps =
            Max(bitrate_table[cfg->version][cfg->vbr_min_bitrate_index],
                gfp->VBR_mean_bitrate_kbps);
    }

    cfg->preset = gfp->preset;
    cfg->write_lame_tag = gfp->write_lame_tag;
    gfc->sv_qnt.substep_shaping = gfp->substep_shaping;
    cfg->noise_shaping = gfp->noise_shaping;
    cfg->subblock_gain = gfp->subblock_gain;
    cfg->use_best_huffman = gfp->use_best_huffman;
    cfg->avg_bitrate = gfp->brate;
    cfg->vbr_avg_bitrate_kbps = gfp->VBR_mean_bitrate_kbps;
    cfg->compression_ratio = gfp->compression_ratio;

    /* A free format bitrate is the caller's own number rather than one from
       the standard's tables, so this is the first point at which anything has
       looked at whether a frame that size can hold a frame at all. It is asked
       here because it needs the side information length, the number of
       granules and the output sample rate, and this is where the last of them
       is known. */
    if (cfg->free_format && !frame_has_room_for_audio(cfg)) {
        int const lowest = lowest_usable_free_format_bitrate(cfg);

        if (lowest > 0) {
            ERRORF(gfc, "Error: a free format stream at %d Hz with %d channel(s) "
                   "cannot be encoded at %d kbit/s - a frame that size has no "
                   "room for audio beside its own side information. The lowest "
                   "usable bitrate here is %d kbit/s.\n",
                   cfg->samplerate_out, cfg->channels_out, cfg->avg_bitrate,
                   lowest);
        }
        else {
            ERRORF(gfc, "Error: a free format stream at %d Hz with %d channel(s) "
                   "cannot be encoded at any bitrate this format allows.\n",
                   cfg->samplerate_out, cfg->channels_out);
        }
        return -1;
    }

    /* initialize internal qval settings */
    lame_init_qval(gfp);


    /*  automatic ATH adjustment on
     */
    if (gfp->athaa_type < 0)
        gfp->athaa_type = 3;
    gfc->ATH->use_adjust = gfp->athaa_type;


    /* initialize internal adaptive ATH settings  -jd */
    gfc->ATH->aa_sensitivity_p = pow(10.0, gfp->athaa_sensitivity / -10.0);


    if (gfp->short_blocks == short_block_not_set) {
        gfp->short_blocks = short_block_allowed;
    }

    /*Note Jan/2003: Many hardware decoders cannot handle short blocks in regular
       stereo mode unless they are coupled (same type in both channels)
       it is a rare event (1 frame per min. or so) that LAME would use
       uncoupled short blocks, so lets turn them off until we decide
       how to handle this.  No other encoders allow uncoupled short blocks,
       even though it is in the standard.  */
    /* rh 20040217: coupling makes no sense for mono and dual-mono streams
     */
    if (gfp->short_blocks == short_block_allowed
        && (cfg->mode == JOINT_STEREO || cfg->mode == STEREO)) {
        gfp->short_blocks = short_block_coupled;
    }

    cfg->short_blocks = gfp->short_blocks;


    if (lame_get_quant_comp(gfp) < 0)
        (void) lame_set_quant_comp(gfp, 1);
    if (lame_get_quant_comp_short(gfp) < 0)
        (void) lame_set_quant_comp_short(gfp, 0);

    if (lame_get_msfix(gfp) < 0)
        lame_set_msfix(gfp, 0);

    /* select psychoacoustic model */
    (void) lame_set_exp_nspsytune(gfp, lame_get_exp_nspsytune(gfp) | 1);

    if (gfp->ATHtype < 0)
        gfp->ATHtype = 4;

    if (gfp->ATHcurve < 0)
        gfp->ATHcurve = 4;

    if (gfp->interChRatio < 0)
        gfp->interChRatio = 0;

    if (gfp->useTemporal < 0)
        gfp->useTemporal = 1; /* on by default */


    cfg->interChRatio = gfp->interChRatio;
    cfg->msfix = gfp->msfix;
    cfg->ATH_offset_db = 0-gfp->ATH_lower_db;
    cfg->ATH_offset_factor = powf(10.f, cfg->ATH_offset_db * 0.1f);
    cfg->ATHcurve = gfp->ATHcurve;
    cfg->ATHtype = gfp->ATHtype;
    cfg->ATHonly = gfp->ATHonly;
    cfg->ATHshort = gfp->ATHshort;
    cfg->noATH = gfp->noATH;

    cfg->quant_comp = gfp->quant_comp;
    cfg->quant_comp_short = gfp->quant_comp_short;

    cfg->use_temporal_masking_effect = gfp->useTemporal;
    if (cfg->mode == JOINT_STEREO) {
        cfg->use_safe_joint_stereo = gfp->exp_nspsytune & 2;
    }
    else {
        cfg->use_safe_joint_stereo = 0;
    }
    {
        cfg->adjust_bass_db = nspsytune_db(gfp->exp_nspsytune, 2);
        cfg->adjust_alto_db = nspsytune_db(gfp->exp_nspsytune, 8);
        cfg->adjust_treble_db = nspsytune_db(gfp->exp_nspsytune, 14);
        /*  to be compatible with Naoki's original code, the next 6 bits
         *  define only the amount of changing treble for sfb21 */
        cfg->adjust_sfb21_db = nspsytune_db(gfp->exp_nspsytune, 20) + cfg->adjust_treble_db;
    }

    /* Setting up the PCM input data transform matrix, to apply 
     * user defined re-scaling, and or two-to-one channel downmix.
     */
    {
        FLOAT   m[2][2] = { {1.0f, 0.0f}, {0.0f, 1.0f} };

        /* user selected scaling of the samples */
        m[0][0] *= gfp->scale;
        m[0][1] *= gfp->scale;
        m[1][0] *= gfp->scale;
        m[1][1] *= gfp->scale;
        /* user selected scaling of the channel 0 (left) samples */
        m[0][0] *= gfp->scale_left;
        m[0][1] *= gfp->scale_left;
        /* user selected scaling of the channel 1 (right) samples */
        m[1][0] *= gfp->scale_right;
        m[1][1] *= gfp->scale_right;
        /* Downsample to Mono if 2 channels in and 1 channel out */
        if (cfg->channels_in == 2 && cfg->channels_out == 1) {
            m[0][0] = 0.5f * (m[0][0] + m[1][0]);
            m[0][1] = 0.5f * (m[0][1] + m[1][1]);
            m[1][0] = 0;
            m[1][1] = 0;
        }
        cfg->pcm_transform[0][0] = m[0][0];
        cfg->pcm_transform[0][1] = m[0][1];
        cfg->pcm_transform[1][0] = m[1][0];
        cfg->pcm_transform[1][1] = m[1][1];
    }

    /* padding method as described in
     * "MPEG-Layer3 / Bitstream Syntax and Decoding"
     * by Martin Sieler, Ralph Sperschneider
     *
     * note: there is no padding for the very first frame
     *
     * Robert Hegemann 2000-06-22
     */
    gfc->sv_enc.slot_lag = gfc->sv_enc.frac_SpF = 0;
    if (cfg->vbr == vbr_off)
        gfc->sv_enc.slot_lag = gfc->sv_enc.frac_SpF
            = ((cfg->version + 1) * 72000L * cfg->avg_bitrate) % cfg->samplerate_out;

    (void) lame_init_bitstream(gfp);

    iteration_init(gfc);
    if (psymodel_init(gfp) != 0)
        return -1;

    cfg->buffer_constraint = get_max_frame_buffer_size_by_constraint(cfg, gfp->strict_ISO);


    cfg->findReplayGain = gfp->findReplayGain;
    cfg->decode_on_the_fly = gfp->decode_on_the_fly;

    if (cfg->decode_on_the_fly)
        cfg->findPeakSample = 1;

    if (cfg->findReplayGain) {
        (void) InitGainAnalysis(gfc->sv_rpg.rgdata, cfg->samplerate_out);
    }

#ifdef HAVE_MPG123
    if (cfg->decode_on_the_fly && !gfp->decode_only) {
        if (gfc->hip) {
            hip_decode_exit(gfc->hip);
        }
        gfc->hip = hip_decode_init();
        if (gfc->hip == 0) {
            ERRORF(gfc, "Error: could not start the decoder that measures the encoded output\n");
            return -1;
        }
        /* report functions */
        hip_set_errorf(gfc->hip, gfp->report.errorf);
        hip_set_debugf(gfc->hip, gfp->report.debugf);
        hip_set_msgf(gfc->hip, gfp->report.msgf);
    }
#endif
    /* updating lame internal flags finished successful */
    gfc->lame_init_params_successful = 1;
    return 0;
}

/*! Report the encoding parameters that were settled on. */
/*!
  \ingroup api_encoding
  Writes a short summary for people: the version, the CPU features in use, the
  input and output sample rates and any resampling between them, and the
  encoding mode. It writes through the message function (\c lame_set_msgf()),
  which prints to \c stderr by default. The \c lame tool prints this at the
  start of an encode.

  Call it after \c lame_init_params(), because that function computes most of
  the values it reports. The text is for people, not for programs. It changes
  between versions. A program that needs a value should call the getter for
  it.

  \param gfp an initialized encoder instance.
*/
void
lame_print_config(const lame_global_flags * gfp)
{
    lame_internal_flags const *gfc;
    SessionConfig_t const *cfg;
    double  out_samplerate, in_samplerate;

    if (!is_lame_global_flags_valid(gfp) || gfp->internal_flags == 0) {
        return;
    }
    gfc = gfp->internal_flags;
    cfg = &gfc->cfg;
    out_samplerate = cfg->samplerate_out;
    in_samplerate = cfg->samplerate_in;

    MSGF(gfc, "LAME %s %s (%s)\n", get_lame_version(), get_lame_os_bitness(), get_lame_url());

#if (LAME_ALPHA_VERSION)
    MSGF(gfc, "warning: alpha versions should be used for testing only\n");
#endif
    {
        char    text[256];
        char    disp[VECTOR_IMPL_NAME_MAX];

        /* What could be selected here, rather than what the processor
           happens to carry: a set this build does not have is not a choice,
           and reporting it invites the question of why it was not used.

           Not asked at all when the caller asked for none, because building
           this list is the probe: the promise that none leaves the processor
           uninterrogated would be worth nothing if a report could break it,
           and a request for diagnostic output is exactly what someone whose
           machine cannot survive the probe would reach for. */
        if (gfp->vector_routines_request != VECTOR_IMPL_NONE
            && vector_impl_available_list(text, sizeof text)[0] != '\0')
            MSGF(gfc, "CPU features: %s\n", text);

        /* Always printed, "NONE" included. It answers "which set did this
           encode run", and no line at all would be indistinguishable from a
           version that does not report it. */
        MSGF(gfc, "vector routines: %s\n",
             vector_impl_display(vector_impl_name(vector_implementation(gfc)),
                                 disp, sizeof disp));
    }

    if (cfg->channels_in == 2 && cfg->channels_out == 1 /* mono */ ) {
        MSGF(gfc, "Autoconverting from stereo to mono. Setting encoding to mono mode.\n");
    }

    if (isResamplingNecessary(cfg)) {
        MSGF(gfc, "Resampling:  input %g kHz  output %g kHz\n",
             1.e-3 * in_samplerate, 1.e-3 * out_samplerate);
    }

    if (cfg->highpass2 > 0.)
        MSGF(gfc,
             "Using polyphase highpass filter, transition band: %5.0f Hz - %5.0f Hz\n",
             0.5 * cfg->highpass1 * out_samplerate, 0.5 * cfg->highpass2 * out_samplerate);
    if (0. < cfg->lowpass1 || 0. < cfg->lowpass2) {
        MSGF(gfc,
             "Using polyphase lowpass filter, transition band: %5.0f Hz - %5.0f Hz\n",
             0.5 * cfg->lowpass1 * out_samplerate, 0.5 * cfg->lowpass2 * out_samplerate);
    }
    else {
        MSGF(gfc, "polyphase lowpass filter disabled\n");
    }

    if (cfg->free_format) {
        MSGF(gfc, "Warning: many decoders cannot handle free format bitstreams\n");
        if (cfg->avg_bitrate > 320) {
            MSGF(gfc,
                 "Warning: many decoders cannot handle free format bitrates >320 kbps (see documentation)\n");
        }
    }
}


/** \internal \brief Names the Huffman table search for lame_print_internals().
    \param use_best_huffman  the setting.
    \return the name. */
static const char *
huffman_search_name(int use_best_huffman)
{
    switch (use_best_huffman) {
    case 1:
        return "best (outside loop)";
    case 2:
        return "best (inside loop, slow)";
    default:
        return "normal";
    }
}

/** \internal \brief Names the MPEG version for lame_print_internals().
    MPEG-2.5 is the extension below 16 kHz that the frame header marks.
    \param cfg  the session configuration.
    \return "1", "2" or "2.5". */
static const char *
mpeg_version_name(SessionConfig_t const *cfg)
{
    if (cfg->version == 1)
        return "1";
    if (cfg->samplerate_out < 16000)
        return "2.5";
    return "2";
}

/** \internal \brief Names the channel mode for lame_print_internals().
    \param mode  the mode.
    \return the name. */
static const char *
channel_mode_name(MPEG_mode mode)
{
    switch (mode) {
    case JOINT_STEREO:
        return "joint stereo";
    case STEREO:
        return "stereo";
    case DUAL_CHANNEL:
        return "dual channel";
    case MONO:
        return "mono";
    case NOT_SET:
        return "not set (error)";
    default:
        return "unknown (error)";
    }
}

/** \internal \brief Describes the bitrate mode for lame_print_internals().
    \param vbr  the mode.
    \return the description, or NULL for a mode this function does not know. */
static const char *
bitrate_mode_name(vbr_mode vbr)
{
    switch (vbr) {
    case vbr_off:
        return "constant bitrate - CBR";
    case vbr_abr:
        return "variable bitrate - ABR";
    case vbr_rh:
        return "variable bitrate - VBR rh";
    case vbr_mt:
        return "variable bitrate - VBR mt";
    case vbr_mtrh:
        return "variable bitrate - VBR mtrh";
    default:
        return NULL;
    }
}

/** \internal \brief Names the short block setting for lame_print_internals().
    \param short_blocks  the setting.
    \return the name. */
static const char *
short_blocks_name(short_block_t short_blocks)
{
    switch (short_blocks) {
    case short_block_allowed:
        return "allowed";
    case short_block_coupled:
        return "channel coupled";
    case short_block_dispensed:
        return "dispensed";
    case short_block_forced:
        return "forced";
    default:
        return "?";
    }
}

/** \internal \brief Describes how the ATH is used, for lame_print_internals().
    \param cfg  the session configuration.
    \return the description. */
static const char *
ath_use_name(SessionConfig_t const *cfg)
{
    if (cfg->noATH)
        return "not used";
    if (cfg->ATHonly)
        return "the only masking";
    if (cfg->ATHshort)
        return "the only masking for short blocks";
    return "using";
}

/** \internal \brief Prints the scaling and search settings of lame_print_internals().
    \param gfp  the encoder instance.
    \param gfc  its internal state. */
static void
print_internals_misc(const lame_global_flags * gfp, lame_internal_flags const *gfc)
{
    MSGF(gfc, "\nmisc:\n\n");
    MSGF(gfc, "\tscaling: %g\n", gfp->scale);
    MSGF(gfc, "\tch0 (left) scaling: %g\n", gfp->scale_left);
    MSGF(gfc, "\tch1 (right) scaling: %g\n", gfp->scale_right);
    MSGF(gfc, "\thuffman search: %s\n", huffman_search_name(gfc->cfg.use_best_huffman));
    MSGF(gfc, "\texperimental Y=%d\n", gfp->experimentalY);
    MSGF(gfc, "\t...\n");
}

/** \internal \brief Prints the stream format section of lame_print_internals().
    \param gfc  the internal state. */
static void
print_internals_stream_format(lame_internal_flags const *gfc)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    const char *mode = bitrate_mode_name(cfg->vbr);
    const char *note = "";

    MSGF(gfc, "\nstream format:\n\n");
    MSGF(gfc, "\tMPEG-%s Layer 3\n", mpeg_version_name(cfg));
    MSGF(gfc, "\t%d channel - %s\n", cfg->channels_out, channel_mode_name(cfg->mode));
    MSGF(gfc, "\tpadding: %s\n", cfg->vbr == vbr_off ? "off" : "all");
    if (vbr_default == cfg->vbr)
        note = "(default)";
    else if (cfg->free_format)
        note = "(free format)";
    if (mode != NULL)
        MSGF(gfc, "\t%s %s\n", mode, note);
    else
        MSGF(gfc, "\t ?? oops, some new one ?? \n");
    if (cfg->write_lame_tag)
        MSGF(gfc, "\tusing LAME Tag\n");
    MSGF(gfc, "\t...\n");
}

/** \internal \brief Prints the psychoacoustic section of lame_print_internals().
    \param gfc  the internal state. */
static void
print_internals_psychoacoustic(lame_internal_flags const *gfc)
{
    SessionConfig_t const *const cfg = &gfc->cfg;

    MSGF(gfc, "\npsychoacoustic:\n\n");
    MSGF(gfc, "\tusing short blocks: %s\n", short_blocks_name(cfg->short_blocks));
    MSGF(gfc, "\tsubblock gain: %d\n", cfg->subblock_gain);
    MSGF(gfc, "\tadjust masking: %g dB\n", gfc->sv_qnt.mask_adjust);
    MSGF(gfc, "\tadjust masking short: %g dB\n", gfc->sv_qnt.mask_adjust_short);
    MSGF(gfc, "\tquantization comparison: %d\n", cfg->quant_comp);
    MSGF(gfc, "\t ^ comparison short blocks: %d\n", cfg->quant_comp_short);
    MSGF(gfc, "\tnoise shaping: %d\n", cfg->noise_shaping);
    MSGF(gfc, "\t ^ amplification: %d\n", cfg->noise_shaping_amp);
    MSGF(gfc, "\t ^ stopping: %d\n", cfg->noise_shaping_stop);
    MSGF(gfc, "\tATH: %s\n", ath_use_name(cfg));
    MSGF(gfc, "\t ^ type: %d\n", cfg->ATHtype);
    MSGF(gfc, "\t ^ shape: %g%s\n", cfg->ATHcurve, " (only for type 4)");
    MSGF(gfc, "\t ^ level adjustement: %g dB\n", cfg->ATH_offset_db);
    MSGF(gfc, "\t ^ adjust type: %d\n", gfc->ATH->use_adjust);
    MSGF(gfc, "\t ^ adjust sensitivity power: %f\n", gfc->ATH->aa_sensitivity_p);

    MSGF(gfc, "\texperimental psy tunings by Naoki Shibata\n");
    MSGF(gfc, "\t   adjust masking bass=%g dB, alto=%g dB, treble=%g dB, sfb21=%g dB\n",
         10 * log10(gfc->sv_qnt.longfact[0]),
         10 * log10(gfc->sv_qnt.longfact[7]),
         10 * log10(gfc->sv_qnt.longfact[14]), 10 * log10(gfc->sv_qnt.longfact[21]));

    MSGF(gfc, "\tusing temporal masking effect: %s\n", cfg->use_temporal_masking_effect ? "yes" : "no");
    MSGF(gfc, "\tinterchannel masking ratio: %g\n", cfg->interChRatio);
    MSGF(gfc, "\t...\n");
}

/*! Report the full internal encoder configuration. */
/*!
  \ingroup api_encoding
  A much more detailed form of \c lame_print_config(). It writes many lines
  about the psychoacoustic settings, the filters, the quantization parameters
  and the VBR configuration, also through the message function
  (\c lame_set_msgf()). The \c lame tool prints this with \c --verbose.

  It is meant for diagnosing an encode, so call it after
  \c lame_init_params(). The layout can change between versions. Do not parse
  it.

  \param gfp an initialized encoder instance.

  \todo The output lists the settings densely. A clearer layout would help,
        and the parts of the configuration that it does not print yet should
        be added.
*/
void
lame_print_internals(const lame_global_flags * gfp)
{
    lame_internal_flags const *gfc;

    if (!is_lame_global_flags_valid(gfp) || gfp->internal_flags == 0) {
        return;
    }
    gfc = gfp->internal_flags;
    print_internals_misc(gfp, gfc);
    print_internals_stream_format(gfc);
    print_internals_psychoacoustic(gfc);
    MSGF(gfc, "\n");
}


static void
save_gain_values(lame_internal_flags * gfc)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    RpgStateVar_t const *const rsv = &gfc->sv_rpg;
    RpgResult_t *const rov = &gfc->ov_rpg;
    /* save the ReplayGain value */
    if (cfg->findReplayGain) {
        FLOAT const RadioGain = (FLOAT) GetTitleGain(rsv->rgdata);
        if (NEQ(RadioGain, (FLOAT) GAIN_NOT_ENOUGH_SAMPLES)) {
            rov->RadioGain = (int) floor(RadioGain * 10.0 + 0.5); /* round to nearest */
        }
        else {
            rov->RadioGain = 0;
        }
    }

    /* find the gain and scale change required for no clipping */
    if (cfg->findPeakSample) {
        if (rov->PeakSample > 0) {
            rov->noclipGainChange = (int) ceil(log10(rov->PeakSample / 32767.0) * 20.0 * 10.0); /* round up */

            if (rov->noclipGainChange > 0) { /* clipping occurs */
                rov->noclipScale = floor((32767.0f / rov->PeakSample) * 100.0f) / 100.0f; /* round down */
            }
            else        /* no clipping */
                rov->noclipScale = -1.0f;
        }
        else {
            /* Material with no non-zero sample has no peak to measure against,
               and the logarithm of zero is not a number this can carry. It
               cannot clip either, so it gets the answer that says so. */
            rov->noclipGainChange = 0;
            rov->noclipScale = -1.0f;
        }
    }
}



static int
update_inbuffer_size(lame_internal_flags * gfc, const int nsamples)
{
    EncStateVar_t *const esv = &gfc->sv_enc;
    if (esv->in_buffer_0 == 0 || esv->in_buffer_nsamples < nsamples) {
        if (esv->in_buffer_0) {
            free(esv->in_buffer_0);
        }
        if (esv->in_buffer_1) {
            free(esv->in_buffer_1);
        }
        esv->in_buffer_0 = lame_calloc(sample_t, nsamples);
        esv->in_buffer_1 = lame_calloc(sample_t, nsamples);
        esv->in_buffer_nsamples = nsamples;
    }
    if (esv->in_buffer_0 == NULL || esv->in_buffer_1 == NULL) {
        if (esv->in_buffer_0) {
            free(esv->in_buffer_0);
        }
        if (esv->in_buffer_1) {
            free(esv->in_buffer_1);
        }
        esv->in_buffer_0 = 0;
        esv->in_buffer_1 = 0;
        esv->in_buffer_nsamples = 0;
        ERRORF(gfc, "Error: can't allocate in_buffer buffer\n");
        return -2;
    }
    return 0;
}


static int
calcNeeded(SessionConfig_t const * cfg)
{
    int     mf_needed;
    int     pcm_samples_per_frame = SAMPLES_PER_GRANULE * cfg->mode_gr;

    /* some sanity checks */
#if ENCDELAY < MDCTDELAY
# error ENCDELAY is less than MDCTDELAY, see encoder.h
#endif
#if FFTOFFSET > BLKSIZE
# error FFTOFFSET is greater than BLKSIZE, see encoder.h
#endif

    mf_needed = BLKSIZE + pcm_samples_per_frame - FFTOFFSET; /* amount needed for FFT */
    /*mf_needed = Max(mf_needed, 286 + 576 * (1 + gfc->mode_gr)); */
    mf_needed = Max(mf_needed, 512 + pcm_samples_per_frame - 32);

    assert(MFSIZE >= mf_needed);
    
    return mf_needed;
}


/*
 * THE MAIN LAME ENCODING INTERFACE
 * mt 3/00
 *
 * input pcm data, output (maybe) mp3 frames.
 * This routine handles all buffering, resampling and filtering for you.
 * The required mp3buffer_size can be computed from num_samples,
 * samplerate and encoding rate, but here is a worst case estimate:
 *
 * mp3buffer_size in bytes = 1.25*num_samples + 7200
 *
 * return code = number of bytes output in mp3buffer.  can be 0
 *
 * NOTE: this routine uses LAME's internal PCM data representation,
 * 'sample_t'.  It should not be used by any application.
 * applications should use lame_encode_buffer(),
 *                         lame_encode_buffer_float()
 *                         lame_encode_buffer_int()
 * etc... depending on what type of data they are working with.
*/
static int
lame_encode_buffer_sample_t(lame_internal_flags * gfc,
                            int nsamples, unsigned char *mp3buf, const int mp3buf_size)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncStateVar_t *const esv = &gfc->sv_enc;
    int     pcm_samples_per_frame = SAMPLES_PER_GRANULE * cfg->mode_gr;
    int     mp3size = 0, ret, ch, mf_needed;
    int     mp3out;
    sample_t *mfbuf[2];
    sample_t *in_buffer[2];

    if (gfc->class_id != LAME_ID)
        return -3;

    if (nsamples == 0)
        return 0;

    /* copy out any tags that may have been written into bitstream */
    {   /* if user specifed buffer size = 0, dont check size */
        int const buf_size = mp3buf_size == 0 ? INT_MAX : mp3buf_size;
        mp3out = copy_buffer(gfc, mp3buf, buf_size, 0);
    }
    if (mp3out < 0)
        return mp3out;  /* not enough buffer space */
    if (mp3out > 0) {
        mp3buf += mp3out;
        mp3size += mp3out;
    }

    in_buffer[0] = esv->in_buffer_0;
    in_buffer[1] = esv->in_buffer_1;

    mf_needed = calcNeeded(cfg);

    while (nsamples > 0) {
        sample_t const *in_buffer_ptr[2];
        int     n_in = 0;    /* number of input samples processed with fill_buffer */
        int     n_out = 0;   /* number of samples output with fill_buffer */
        /* n_in <> n_out if we are resampling */

        /* move the samples in use to the start of mfbuf only when the next
           frame of input would not fit behind them */
        if (esv->mf_size < mf_needed
            && esv->mf_start + esv->mf_size + pcm_samples_per_frame > MFBUF_SIZE) {
            for (ch = 0; ch < cfg->channels_out; ch++)
                memmove(&esv->mfbuf[ch][0], &esv->mfbuf[ch][esv->mf_start],
                        esv->mf_size * sizeof(esv->mfbuf[0][0]));
            esv->mf_start = 0;
        }
        mfbuf[0] = &esv->mfbuf[0][esv->mf_start];
        mfbuf[1] = &esv->mfbuf[1][esv->mf_start];

        in_buffer_ptr[0] = in_buffer[0];
        in_buffer_ptr[1] = in_buffer[1];
        /* copy in new samples into mfbuf, with resampling - unless a frame is
           still waiting there from a call whose output did not fit; it is
           encoded first */
        if (esv->mf_size < mf_needed
            && fill_buffer(gfc, mfbuf, &in_buffer_ptr[0], nsamples, &n_in, &n_out) < 0)
            return -2;

        /* compute ReplayGain of resampled input if requested */
        if (cfg->findReplayGain && !cfg->decode_on_the_fly)
            if (AnalyzeSamples
                (gfc->sv_rpg.rgdata, &mfbuf[0][esv->mf_size], &mfbuf[1][esv->mf_size], n_out,
                 cfg->channels_out) == GAIN_ANALYSIS_ERROR)
                return -6;



        /* update in_buffer counters */
        nsamples -= n_in;
        in_buffer[0] += n_in;
        if (cfg->channels_out == 2)
            in_buffer[1] += n_in;

        /* update mfbuf[] counters */
        esv->mf_size += n_out;
        assert(esv->mf_size <= MFSIZE);
        assert(esv->mf_start + esv->mf_size <= MFBUF_SIZE);
        
        /* lame_encode_flush may have set gfc->mf_sample_to_encode to 0
         * so we have to reinitialize it here when that happened.
         */
        if (esv->mf_samples_to_encode < 1) {
            esv->mf_samples_to_encode = ENCDELAY + POSTDELAY;
        }        
        esv->mf_samples_to_encode += n_out;


        if (esv->mf_size >= mf_needed) {
            /* encode the frame.  */
            /* mp3buf              = pointer to current location in buffer */
            /* mp3buf_size         = size of original mp3 output buffer */
            /*                     = 0 if we should not worry about the */
            /*                       buffer size because calling program is  */
            /*                       to lazy to compute it */
            /* mp3size             = size of data written to buffer so far */
            /* mp3buf_size-mp3size = amount of space avalable  */

            int     buf_size = mp3buf_size - mp3size;
            if (mp3buf_size == 0)
                buf_size = INT_MAX;

            ret = lame_encode_mp3_frame(gfc, mfbuf[0], mfbuf[1], mp3buf, buf_size);

            if (ret < 0)
                return ret;
            if (ret > 0) {
                mp3buf += ret;
                mp3size += ret;
            }

            /* the next frame starts one frame later */
            esv->mf_start += pcm_samples_per_frame;
            esv->mf_size -= pcm_samples_per_frame;
            esv->mf_samples_to_encode -= pcm_samples_per_frame;
        }
    }
    assert(nsamples == 0);

    return mp3size;
}

enum PCMSampleType 
{   pcm_short_type
,   pcm_int_type
,   pcm_long_type
,   pcm_float_type
,   pcm_double_type
};

/** \internal \brief How the samples of one encode entry point are laid out. */
typedef struct pcm_layout {
    enum PCMSampleType type;    /**< the C type of one sample */
    int     stride;             /**< 1 for two arrays, 2 for one interleaved array */
    FLOAT   to_16bit;           /**< the factor that scales a sample to 16-bit full scale */
} pcm_layout_t;

/** \internal \brief The factor for an \c int sample at full scale. */
#define INT_TO_16BIT ((FLOAT) (1.0 / (1L << (8 * sizeof(int) - 16))))
/** \internal \brief The factor for a \c long sample at full scale. */
#define LONG_TO_16BIT ((FLOAT) (1.0 / (1L << (8 * sizeof(long) - 16))))

static const pcm_layout_t layout_short = { pcm_short_type, 1, 1.0 };
static const pcm_layout_t layout_short_interleaved = { pcm_short_type, 2, 1.0 };
static const pcm_layout_t layout_int = { pcm_int_type, 1, INT_TO_16BIT };
static const pcm_layout_t layout_int_interleaved = { pcm_int_type, 2, INT_TO_16BIT };
static const pcm_layout_t layout_long = { pcm_long_type, 1, 1.0 };
static const pcm_layout_t layout_long_full = { pcm_long_type, 1, LONG_TO_16BIT };
static const pcm_layout_t layout_float = { pcm_float_type, 1, 1.0 };
static const pcm_layout_t layout_ieee_float = { pcm_float_type, 1, 32767.0 };
static const pcm_layout_t layout_ieee_float_interleaved = { pcm_float_type, 2, 32767.0 };
static const pcm_layout_t layout_ieee_double = { pcm_double_type, 1, 32767.0 };
static const pcm_layout_t layout_ieee_double_interleaved = { pcm_double_type, 2, 32767.0 };

static int
lame_copy_inbuffer(lame_internal_flags* gfc,
                   void const* l, void const* r, int nsamples,
                   pcm_layout_t const *layout)
{
    int const stride = layout->stride;
    FLOAT const s = layout->to_16bit;
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncStateVar_t *const esv = &gfc->sv_enc;
    sample_t* ib0 = esv->in_buffer_0;
    sample_t* ib1 = esv->in_buffer_1;
    FLOAT   m[2][2];
    FLOAT const loudest = MAX_INPUT_SCALE * 32768.0f;
    double  gain_l, gain_r, gain;

    /* Apply user defined re-scaling */
    m[0][0] = s * cfg->pcm_transform[0][0];
    m[0][1] = s * cfg->pcm_transform[0][1];
    m[1][0] = s * cfg->pcm_transform[1][0];
    m[1][1] = s * cfg->pcm_transform[1][1];
    gain_l = fabs(m[0][0]) + fabs(m[0][1]);
    gain_r = fabs(m[1][0]) + fabs(m[1][1]);
    gain = gain_l > gain_r ? gain_l : gain_r;

    /* Integer sample types cannot represent NaN or infinity; only the floating
     * point ones are screened for them. Every type is refused beyond the
     * loudest input the encoder takes, once scaled - checked per sample only
     * where the type's full range times the scaling can get there.
     */
#define VALIDATE_NONE(sl, sr)
#define VALIDATE_FLOAT(sl, sr) \
    if (!float_is_finite(sl) || !float_is_finite(sr)) { \
        return -1; \
    }
#define VALIDATE_DOUBLE(sl, sr) \
    if (!double_is_finite(sl) || !double_is_finite(sr)) { \
        return -1; \
    }

#define BOUND_NONE(u, v)
#define BOUND_CHECK(u, v) \
    if (u > loudest || u < -loudest || v > loudest || v < -loudest) { \
        return -1; \
    }

    /* make a copy of input buffer, changing type to sample_t */
#define COPY_AND_TRANSFORM(T, VALIDATE, BOUND) \
{ \
    T const *bl = l, *br = r; \
    int     i; \
    for (i = 0; i < nsamples; i++) { \
        VALIDATE(*bl, *br) \
        { \
            sample_t const xl = *bl; \
            sample_t const xr = *br; \
            sample_t const u = xl * m[0][0] + xr * m[0][1]; \
            sample_t const v = xl * m[1][0] + xr * m[1][1]; \
            BOUND(u, v) \
            ib0[i] = u; \
            ib1[i] = v; \
        } \
        bl += stride; \
        br += stride; \
    } \
}
#define COPY_INTEGER(T, FULL_RANGE) \
    if ((FULL_RANGE) * gain > loudest) \
        COPY_AND_TRANSFORM(T, VALIDATE_NONE, BOUND_CHECK) \
    else \
        COPY_AND_TRANSFORM(T, VALIDATE_NONE, BOUND_NONE)

    switch ( layout->type ) {
    case pcm_short_type:
        COPY_INTEGER(short int, 32768.0);
        break;
    case pcm_int_type:
        COPY_INTEGER(int, 2147483648.0);
        break;
    case pcm_long_type:
        COPY_INTEGER(long int, (double) LONG_MAX + 1.0);
        break;
    case pcm_float_type:
        COPY_AND_TRANSFORM(float, VALIDATE_FLOAT, BOUND_CHECK);
        break;
    case pcm_double_type:
        COPY_AND_TRANSFORM(double, VALIDATE_DOUBLE, BOUND_CHECK);
        break;
    }
    return 0;
}


static int
lame_encode_buffer_template(lame_global_flags * gfp,
                            void const* buffer_l, void const* buffer_r, const int nsamples,
                            unsigned char *mp3buf, const int mp3buf_size,
                            pcm_layout_t const *layout)
{
    lame_internal_flags *gfc;
    int     rc;

    if (!is_lame_global_flags_valid(gfp))
        return -3;
    gfc = gfp->internal_flags;
    if (!is_lame_internal_flags_valid(gfc))
        return -3;
    if (nsamples == 0)
        return 0;
    if (nsamples < 0)
        return LAME_BADINPUTDATA;
    if (update_inbuffer_size(gfc, nsamples) != 0)
        return -2;

    /* make a copy of input buffer, changing type to sample_t */
    if (gfc->cfg.channels_in > 1) {
        if (buffer_l == 0 || buffer_r == 0)
            return 0;
        rc = lame_copy_inbuffer(gfc, buffer_l, buffer_r, nsamples, layout);
    }
    else {
        if (buffer_l == 0)
            return 0;
        /* An interleaved entry point reads the two channels from a
         * single buffer with a stride of two. On a mono session
         * there is only one channel in that buffer and no length to
         * bound the second read, so this walks one channel's worth
         * of samples past its end. Reject the combination rather
         * than read outside the caller's buffer; a mono session
         * takes its samples through the non-interleaved entry
         * points, which is what the interleaving front ends already
         * do.
         */
        if (layout->stride != 1)
            return LAME_BADINPUTDATA;
        rc = lame_copy_inbuffer(gfc, buffer_l, buffer_l, nsamples, layout);
    }
    /* A non-finite sample would spread through the psycho acoustic
     * model and turn the whole frame into noise; one beyond the
     * loudest input is more than any frame can carry.
     */
    if (rc != 0)
        return LAME_BADINPUTDATA;

    return lame_encode_buffer_sample_t(gfc, nsamples, mp3buf, mp3buf_size);
}

int
lame_encode_buffer(lame_global_flags * gfp,
                   const short int pcm_l[], const short int pcm_r[], const int nsamples,
                   unsigned char *mp3buf, const int mp3buf_size)
{
    return lame_encode_buffer_template(gfp, pcm_l, pcm_r, nsamples, mp3buf, mp3buf_size, &layout_short);
}


int
lame_encode_buffer_float(lame_global_flags * gfp,
                         const float pcm_l[], const float pcm_r[], const int nsamples,
                         unsigned char *mp3buf, const int mp3buf_size)
{
    /* input is assumed to be normalized to +/- 32768 for full scale */
    return lame_encode_buffer_template(gfp, pcm_l, pcm_r, nsamples, mp3buf, mp3buf_size, &layout_float);
}


int
lame_encode_buffer_ieee_float(lame_t gfp,
                         const float pcm_l[], const float pcm_r[], const int nsamples,
                         unsigned char *mp3buf, const int mp3buf_size)
{
    /* input is assumed to be normalized to +/- 1.0 for full scale */
    return lame_encode_buffer_template(gfp, pcm_l, pcm_r, nsamples, mp3buf, mp3buf_size, &layout_ieee_float);
}


int
lame_encode_buffer_interleaved_ieee_float(lame_t gfp,
                         const float pcm[], const int nsamples,
                         unsigned char *mp3buf, const int mp3buf_size)
{
    /* input is assumed to be normalized to +/- 1.0 for full scale */
    return lame_encode_buffer_template(gfp, pcm, pcm+1, nsamples, mp3buf, mp3buf_size, &layout_ieee_float_interleaved);
}


int
lame_encode_buffer_ieee_double(lame_t gfp,
                         const double pcm_l[], const double pcm_r[], const int nsamples,
                         unsigned char *mp3buf, const int mp3buf_size)
{
    /* input is assumed to be normalized to +/- 1.0 for full scale */
    return lame_encode_buffer_template(gfp, pcm_l, pcm_r, nsamples, mp3buf, mp3buf_size, &layout_ieee_double);
}


int
lame_encode_buffer_interleaved_ieee_double(lame_t gfp,
                         const double pcm[], const int nsamples,
                         unsigned char *mp3buf, const int mp3buf_size)
{
    /* input is assumed to be normalized to +/- 1.0 for full scale */
    return lame_encode_buffer_template(gfp, pcm, pcm+1, nsamples, mp3buf, mp3buf_size, &layout_ieee_double_interleaved);
}


/*! Encode PCM given as \c int, using the full range of the type. */
/*!
  \ingroup api_encoding
  As \c lame_encode_buffer(), but the samples are \c int. Full scale is the
  whole range of \c int, +/- 2^(8*sizeof(int)-1). The \c short entry point
  uses +/- 32768. So this function differs in the expected range, not only in
  the type.

  \param gfp          the encoder instance.
  \param pcm_l        PCM data for the left channel.
  \param pcm_r        PCM data for the right channel.
  \param nsamples     number of samples per channel.
  \param mp3buf       receives the encoded MP3 stream.
  \param mp3buf_size  size of \a mp3buf in bytes. 0 means that LAME does not
                      check the size.
  \return As \c lame_encode_buffer().
*/
int
lame_encode_buffer_int(lame_global_flags * gfp,
                       const int pcm_l[], const int pcm_r[], const int nsamples,
                       unsigned char *mp3buf, const int mp3buf_size)
{
    return lame_encode_buffer_template(gfp, pcm_l, pcm_r, nsamples, mp3buf, mp3buf_size, &layout_int);
}


/*! Encode PCM given as \c long, using the full range of the type. */
/*!
  \ingroup api_encoding
  As \c lame_encode_buffer(), but the samples are \c long, and full scale is
  the whole range of \c long, +/- 2^(8*sizeof(long)-1).

  Use this function for data that fills a \c long.
  \c lame_encode_buffer_long() takes the same type, but expects the range of a
  \c short.

  \param gfp          the encoder instance.
  \param pcm_l        PCM data for the left channel.
  \param pcm_r        PCM data for the right channel.
  \param nsamples     number of samples per channel.
  \param mp3buf       receives the encoded MP3 stream.
  \param mp3buf_size  size of \a mp3buf in bytes. 0 means that LAME does not
                      check the size.
  \return As \c lame_encode_buffer().
*/
int
lame_encode_buffer_long2(lame_global_flags * gfp,
                         const long pcm_l[],  const long pcm_r[], const int nsamples,
                         unsigned char *mp3buf, const int mp3buf_size)
{
    return lame_encode_buffer_template(gfp, pcm_l, pcm_r, nsamples, mp3buf, mp3buf_size, &layout_long_full);
}


/*! Encode PCM given as \c long, scaled as if it were \c short. */
/*!
  \ingroup api_encoding
  As \c lame_encode_buffer(), but the samples are \c long. The range is still
  that of a \c short, +/- 32768, so the wider type gives no more precision
  than a \c short.

  \c lame_encode_buffer_long2() takes the same type with the range of \c long.
  Use it for data that fills a \c long.

  \param gfp          the encoder instance.
  \param pcm_l        PCM data for the left channel.
  \param pcm_r        PCM data for the right channel.
  \param nsamples     number of samples per channel.
  \param mp3buf       receives the encoded MP3 stream.
  \param mp3buf_size  size of \a mp3buf in bytes. 0 means that LAME does not
                      check the size.
  \return As \c lame_encode_buffer().
*/
int
lame_encode_buffer_long(lame_global_flags * gfp,
                        const long pcm_l[], const long pcm_r[], const int nsamples,
                        unsigned char *mp3buf, const int mp3buf_size)
{
    /* input is assumed to be normalized to +/- 32768 for full scale */
    return lame_encode_buffer_template(gfp, pcm_l, pcm_r, nsamples, mp3buf, mp3buf_size, &layout_long);
}



/*! Encode interleaved PCM given as \c short. */
/*!
  \ingroup api_encoding
  As \c lame_encode_buffer(), but both channels are in one buffer. The samples
  alternate, left channel first.

  If the instance is set to one input channel (\c lame_set_num_channels()),
  the call returns -14. This function always reads two
  channels from the buffer. For one input channel, use a non-interleaved entry
  point such as \c lame_encode_buffer(). Encoding two input channels to a mono
  output works as usual.

  \param gfp          the encoder instance.
  \param pcm          PCM data for both channels, interleaved.
  \param nsamples     number of samples in one channel, which is half the
                      number of values in \a pcm.
  \param mp3buf       receives the encoded MP3 stream.
  \param mp3buf_size  size of \a mp3buf in bytes. 0 means that LAME does not
                      check the size.
  \return As \c lame_encode_buffer().
*/
int
lame_encode_buffer_interleaved(lame_global_flags * gfp,
                               short int pcm[], int nsamples,
                               unsigned char *mp3buf, int mp3buf_size)
{
    /* input is assumed to be normalized to +/- MAX_SHORT for full scale */
    return lame_encode_buffer_template(gfp, pcm, pcm+1, nsamples, mp3buf, mp3buf_size, &layout_short_interleaved);
}


/*! Encode interleaved PCM given as \c int, using the full range of the type. */
/*!
  \ingroup api_encoding
  As \c lame_encode_buffer_interleaved(), but the samples are \c int, and full
  scale is the whole range of \c int, +/- 2^(8*sizeof(int)-1), as for
  \c lame_encode_buffer_int().

  If the instance is set to one input channel, the call returns -14, as for
  \c lame_encode_buffer_interleaved().

  \param gfp          the encoder instance.
  \param pcm          PCM data for both channels, interleaved.
  \param nsamples     number of samples in one channel, which is half the
                      number of values in \a pcm.
  \param mp3buf       receives the encoded MP3 stream.
  \param mp3buf_size  size of \a mp3buf in bytes. 0 means that LAME does not
                      check the size.
  \return As \c lame_encode_buffer().
*/
int
lame_encode_buffer_interleaved_int(lame_t gfp,
                                   const int pcm[], const int nsamples,
                                   unsigned char *mp3buf, const int mp3buf_size)
{
    return lame_encode_buffer_template(gfp, pcm, pcm + 1, nsamples, mp3buf, mp3buf_size, &layout_int_interleaved);
}




/*! Finish one MP3 of a gapless series, keeping the encoder running. */
/*!
  \ingroup api_encoding
  Fills the current frame with ancillary data and returns the MP3 data in the
  buffer. It keeps the buffered PCM and the encoder state, so the encode
  continues into the next output file. It also resets the bit reservoir. So
  the files can be decoded separately, and joined without a gap.

  Unlike \c lame_encode_flush(), this function writes no ID3v1 tag. If the
  next file needs its own tags at the start, call \c lame_init_bitstream()
  before you write it.

  This is the "no gap" part of the support for split files. The caller must
  also tell the encoder how many files there are and which one this is
  (\c lame_set_nogap_total(), \c lame_set_nogap_currentindex()).

  \param gfp             the encoder instance.
  \param mp3buffer       where the remaining MP3 data is written. At least
                         7200 bytes, which holds everything a flush can write.
  \param mp3buffer_size  size of \a mp3buffer in bytes. 0 means that LAME
                         does not check the size.
  \return the number of bytes written to \a mp3buffer, which can be 0. A
          negative value on failure. -3 if the instance is not usable.
*/
int
lame_encode_flush_nogap(lame_global_flags * gfp, unsigned char *mp3buffer, int mp3buffer_size)
{
    int     rc = -3;
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            flush_bitstream(gfc);
            /* if user specifed buffer size = 0, dont check size */
            if (mp3buffer_size == 0)
                mp3buffer_size = INT_MAX;
            rc = copy_buffer(gfc, mp3buffer, mp3buffer_size, 1);
            save_gain_values(gfc);
        }
    }
    return rc;
}


/*! Start a new bitstream: write the leading tags and reset the counters. */
/*!
  \ingroup api_encoding
  Writes the ID3v2 tag (unless the caller writes it, see
  \c lame_set_write_id3tag_automatic()) and the LAME tag frame at the start of
  the stream. It sets the frame number, the histograms and the peak sample to
  0.

  \c lame_init_params() already does this, so a caller that encodes one file
  never needs it. It is for the next file: call it after
  \c lame_encode_flush_nogap() to give the next output file its own tags and
  its own statistics. Call it only after \c lame_init_params().

  \param gfp the encoder instance.
  \retval 0  success.
  \retval -3 the instance is not usable.
*/
int
lame_init_bitstream(lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags *const gfc = gfp->internal_flags;
        if (gfc != 0) {
            gfc->ov_enc.frame_number = 0;

            if (gfp->write_id3tag_automatic) {
                (void) id3tag_write_v2(gfp);
            }
            /* initialize histogram data optionally used by frontend */
            memset(gfc->ov_enc.bitrate_channelmode_hist, 0,
                   sizeof(gfc->ov_enc.bitrate_channelmode_hist));
            memset(gfc->ov_enc.bitrate_blocktype_hist, 0,
                   sizeof(gfc->ov_enc.bitrate_blocktype_hist));

            gfc->ov_rpg.PeakSample = 0.0;

            /* Write initial VBR Header to bitstream and init VBR data */
            if (gfc->cfg.write_lame_tag)
                (void) InitVbrTag(gfp);


            return 0;
        }
    }
    return -3;
}

static int
calc_mp3buffer_size_remaining( int mp3buffer_size, int mp3count)
{
    /* if user specifed buffer size = 0, dont check size */
    if (mp3buffer_size == 0)
        return INT_MAX;
    else if (mp3buffer_size > 0 && mp3count >= 0 ) {
        int const mp3buffer_size_remaining = mp3buffer_size - mp3count;
        if (mp3buffer_size_remaining > 0) {
            return mp3buffer_size_remaining;
        }
        assert(mp3buffer_size_remaining >= 0);
        /* we reached the end of the output buffer, set to -1
           because we have to distinguish this case from the case above,
           where the caller did not specify any buffer size
         */
        return -1;
    }
    else {
        /* values are invalid, block writing into output buffer */
        return -1;
    }
}

/*! Finish the stream: encode what is still buffered and write the last frames. */
/*!
  \ingroup api_encoding
  The last encoding call for a file. It fills the buffered PCM with silence so
  that the last frame is complete, encodes it, and returns the rest of the MP3
  data. If the library writes the tags itself (see
  \c lame_set_write_id3tag_automatic()), it also adds the ID3v1 tag. The LAME
  tag records how much padding was added, so a decoder that reads the tag does
  not play it.

  A second call does no harm. It finds nothing in the buffer and returns 0.

  This function does not release the instance. Call \c lame_close() after it.
  Use the statistics and tag functions (\c lame_get_lametag_frame(),
  \c lame_mp3_tags_fid(), the histograms) between the two, while the encoder
  state still exists.

  \param gfp             the encoder instance.
  \param mp3buffer       where the last MP3 data is written. At least 7200
                         bytes, which holds everything a flush can write.
  \param mp3buffer_size  size of \a mp3buffer in bytes. 0 means that LAME
                         does not check the size.
  \return the number of bytes written to \a mp3buffer, which can be 0. A
          negative value on failure. -3 if the instance is not usable.
*/
int
lame_encode_flush(lame_global_flags * gfp, unsigned char *mp3buffer, int mp3buffer_size)
{
    lame_internal_flags *gfc;
    SessionConfig_t const *cfg;
    EncStateVar_t *esv;
    short int buffer[2][1152];
    int     imp3 = 0, mp3count, mp3buffer_size_remaining;

    /* we always add POSTDELAY=288 padding to make sure granule with real
     * data can be complety decoded (because of 50% overlap with next granule */
    int     end_padding;
    int     frames_left;
    int     samples_to_encode;
    int     pcm_samples_per_frame;
    int     mf_needed;
    int     is_resampling_necessary;
    double  resample_ratio = 1;

    if (!is_lame_global_flags_valid(gfp)) {
        return -3;
    }
    gfc = gfp->internal_flags;
    if (!is_lame_internal_flags_valid(gfc)) {
        return -3;
    }
    cfg = &gfc->cfg;
    esv = &gfc->sv_enc;
    
    /* Was flush already called? */
    if (esv->mf_samples_to_encode < 1) {
        return 0;
    }
    pcm_samples_per_frame = SAMPLES_PER_GRANULE * cfg->mode_gr;
    mf_needed = calcNeeded(cfg);

    samples_to_encode = esv->mf_samples_to_encode - POSTDELAY;

    memset(buffer, 0, sizeof(buffer));
    mp3count = 0;

    is_resampling_necessary = isResamplingNecessary(cfg);
    if (is_resampling_necessary) {
        resample_ratio = (double)cfg->samplerate_in / (double)cfg->samplerate_out;
        /* delay due to resampling; needs to be fixed, if resampling code gets changed */
        samples_to_encode += 16. / resample_ratio;
    }
    end_padding = pcm_samples_per_frame - (samples_to_encode % pcm_samples_per_frame);
    if (end_padding < SAMPLES_PER_GRANULE)
        end_padding += pcm_samples_per_frame;
    gfc->ov_enc.encoder_padding = end_padding;
    
    frames_left = (samples_to_encode + end_padding) / pcm_samples_per_frame;
    while (frames_left > 0 && imp3 >= 0) {
        int const frame_num = gfc->ov_enc.frame_number;
        int     bunch = mf_needed - esv->mf_size;

        bunch *= resample_ratio;
        if (bunch > 1152) bunch = 1152;
        if (bunch < 1) bunch = 1;

        mp3buffer_size_remaining = calc_mp3buffer_size_remaining(mp3buffer_size, mp3count);

        /* send in a frame of 0 padding until all internal sample buffers
         * are flushed
         */
        imp3 = lame_encode_buffer(gfp, buffer[0], buffer[1], bunch,
                                  mp3buffer, mp3buffer_size_remaining);
        if (imp3 > 0) {
            mp3buffer += imp3;
            mp3count += imp3;
        }
        {   /* even a single pcm sample can produce several frames!
             * for example: 1 Hz input file resampled to 8 kHz mpeg2.5
             */
            int const new_frames = gfc->ov_enc.frame_number - frame_num;
            if (new_frames > 0)
                frames_left -=  new_frames;
        }
    }
    /* Set esv->mf_samples_to_encode to 0, so we may detect
     * and break loops calling it more than once in a row.
     */
    esv->mf_samples_to_encode = 0;

    if (imp3 < 0) {
        /* some type of fatal error */
        return imp3;
    }

    mp3buffer_size_remaining = calc_mp3buffer_size_remaining(mp3buffer_size, mp3count);

    /* mp3 related stuff.  bit buffer might still contain some mp3 data */
    flush_bitstream(gfc);
    imp3 = copy_buffer(gfc, mp3buffer, mp3buffer_size_remaining, 1);
    save_gain_values(gfc);
    if (imp3 < 0) {
        /* some type of fatal error */
        return imp3;
    }
    if (imp3 > 0) {
        mp3buffer += imp3;
        mp3count += imp3;
    }
    mp3buffer_size_remaining = mp3buffer_size - mp3count;
    /* if user specifed buffer size = 0, dont check size */
    if (mp3buffer_size == 0)
        mp3buffer_size_remaining = INT_MAX;

    if (gfp->write_id3tag_automatic) {
        /* write a id3 tag to the bitstream */
        (void) id3tag_write_v1(gfp);

        imp3 = copy_buffer(gfc, mp3buffer, mp3buffer_size_remaining, 0);

        if (imp3 < 0) {
            return imp3;
        }
        mp3count += imp3;
    }
    return mp3count;
}

/*! Release an encoder instance and everything it allocated. */
/*!
  \ingroup api_encoding
  The last call for an instance. It frees the internal buffers. For an
  instance from \c lame_init(), it also frees the instance itself, so do not
  use the pointer after this call.

  Call it for every instance that \c lame_init() returned, also when
  \c lame_init_params() failed, because a failed initialization still leaves
  buffers to free. It does not replace \c lame_encode_flush(). Closing without
  flushing first discards the PCM still in the buffer, and the file then has
  no last frames and no ID3v1 tag.

  \param gfp the encoder instance, or \c NULL, which does nothing.
  \retval 0  success.
  \retval -3 the instance had already lost its internal state. Everything
             that could be freed was freed.
*/
int
lame_close(lame_global_flags * gfp)
{
    int     ret = 0;
    if (gfp && gfp->class_id == LAME_ID) {
        lame_internal_flags *const gfc = gfp->internal_flags;
        gfp->class_id = 0;
        if (NULL == gfc || gfc->class_id != LAME_ID) {
            ret = -3;
        }
        if (NULL != gfc) {
            gfc->lame_init_params_successful = 0;
            gfc->class_id = 0;
            /* this routine will free all malloc'd data in gfc, and then free gfc: */
            freegfc(gfc);
            gfp->internal_flags = NULL;
        }
        if (gfp->lame_allocated_gfp) {
            gfp->lame_allocated_gfp = 0;
            free(gfp);
        }
    }
    return ret;
}


/*! Flush the stream and release the instance in one call. */
/*!
  \ingroup api_encoding
  \deprecated \c lame.h does not declare this function, so no new program can
  call it. The library still builds and exports it, so that programs built
  against an older release still work. New code calls \c lame_encode_flush()
  and then \c lame_close().

  This function does both at once, so nothing can run between them. After it
  returns, the encoder state is gone. The statistics functions and
  \c lame_mp3_tags_fid() then cannot complete the LAME tag.

  \param gfp             the encoder instance.
  \param mp3buffer       where the last MP3 data is written. At least 7200
                         bytes.
  \param mp3buffer_size  size of \a mp3buffer in bytes. 0 means that LAME
                         does not check the size.
  \return the result of \c lame_encode_flush(): the number of bytes written,
          or a negative value on failure. The instance is released in both
          cases. An error from the release is not reported.
*/
int
lame_encode_finish(lame_global_flags * gfp, unsigned char *mp3buffer, int mp3buffer_size)
{
    int const ret = lame_encode_flush(gfp, mp3buffer, mp3buffer_size);

    (void) lame_close(gfp);

    return ret;
}

void    lame_mp3_tags_fid(lame_global_flags * gfp, FILE * fpStream);

/*! Write the finished LAME tag into an already written MP3 file. */
/*!
  \ingroup api_tags
  The LAME tag is the first frame of the stream. It contains the Xing/Info
  header, the seek table and LAME's own fields. It counts what the encode
  produced, so it can only be completed at the end. LAME reserves a frame for
  it at the start of the stream, and this call fills in that frame.

  Call it last: after \c lame_encode_flush(), and after all MP3 data is written
  to \a fpStream. The function skips an ID3v2 tag at the start of the file, if
  there is one, and writes the frame after it. So \a fpStream must be a file
  that can seek and is open for reading and writing.

  The function writes nothing and reports nothing if the encode does not
  produce a LAME tag (\c lame_set_bWriteVbrTag()).

  To place the frame yourself, use \c lame_get_lametag_frame() instead.

  \param gfp       the encoder instance.
  \param fpStream  the MP3 file, positioned anywhere, open for update.
*/
void
lame_mp3_tags_fid(lame_global_flags * gfp, FILE * fpStream)
{
    lame_internal_flags *gfc;
    SessionConfig_t const *cfg;
    if (!is_lame_global_flags_valid(gfp)) {
        return;
    }
    gfc = gfp->internal_flags;
    if (!is_lame_internal_flags_valid(gfc)) {
        return;
    }
    cfg = &gfc->cfg;
    if (!cfg->write_lame_tag) {
        return;
    }
    /* Write Xing header again */
    if (fpStream && !fseek(fpStream, 0, SEEK_SET)) {
        int     rc = PutVbrTag(gfp, fpStream);
        switch (rc) {
        default:
            /* OK */
            break;

        case -1:
            ERRORF(gfc, "Error: could not update LAME tag.\n");
            break;

        case -2:
            ERRORF(gfc, "Error: could not update LAME tag, file not seekable.\n");
            break;

        case -3:
            ERRORF(gfc, "Error: could not update LAME tag, file not readable.\n");
            break;
        }
    }
}


static int
lame_init_internal_flags(lame_internal_flags* gfc)
{
    if (NULL == gfc)
        return -1;

    gfc->cfg.vbr_min_bitrate_index = 1; /* not  0 ????? */
    gfc->cfg.vbr_max_bitrate_index = 13; /* not 14 ????? */
    gfc->cfg.decode_on_the_fly = 0;
    gfc->cfg.findReplayGain = 0;
    gfc->cfg.findPeakSample = 0;

    gfc->sv_qnt.OldValue[0] = 180;
    gfc->sv_qnt.OldValue[1] = 180;
    gfc->sv_qnt.CurrentStep[0] = 4;
    gfc->sv_qnt.CurrentStep[1] = 4;
    gfc->sv_qnt.masking_lower = 1;

    /* The reason for
     *       int mf_samples_to_encode = ENCDELAY + POSTDELAY;
     * ENCDELAY = internal encoder delay.  And then we have to add POSTDELAY=288
     * because of the 50% MDCT overlap.  A 576 MDCT granule decodes to
     * 1152 samples.  To synthesize the 576 samples centered under this granule
     * we need the previous granule for the first 288 samples (no problem), and
     * the next granule for the next 288 samples (not possible if this is last
     * granule).  So we need to pad with 288 samples to make sure we can
     * encode the 576 samples we are interested in.
     */
    gfc->sv_enc.mf_samples_to_encode = ENCDELAY + POSTDELAY;
    gfc->sv_enc.mf_size = ENCDELAY - MDCTDELAY; /* we pad input with this many 0's */
    gfc->ov_enc.encoder_padding = 0;
    gfc->ov_enc.encoder_delay = ENCDELAY;

    gfc->ov_rpg.RadioGain = 0;
    gfc->ov_rpg.noclipGainChange = 0;
    gfc->ov_rpg.noclipScale = -1.0;

    gfc->ATH = lame_calloc(ATH_t, 1);
    if (NULL == gfc->ATH)
        return -2;      /* maybe error codes should be enumerated in lame.h ?? */

    gfc->sv_rpg.rgdata = lame_calloc(replaygain_t, 1);
    if (NULL == gfc->sv_rpg.rgdata) {
        return -2;
    }
    return 0;
}

/* initialize mp3 encoder */
#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
static
#else
#endif
int
lame_init_old(lame_global_flags * gfp)
{
    disable_FPE();      /* disable floating point exceptions */

    memset(gfp, 0, sizeof(lame_global_flags));

    gfp->class_id = LAME_ID;

    /* Global flags.  set defaults here for non-zero values */
    /* see lame.h for description */
    /* set integer values to -1 to mean that LAME will compute the
     * best value, UNLESS the calling program as set it
     * (and the value is no longer -1)
     */
    gfp->strict_ISO = MDB_MAXIMUM;

    gfp->mode = NOT_SET;
    gfp->original = 1;
    gfp->samplerate_in = 44100;
    gfp->num_channels = 2;
    gfp->num_samples = MAX_U_32_NUM;

    gfp->write_lame_tag = 1;
    gfp->quality = -1;
    gfp->short_blocks = short_block_not_set;
    gfp->subblock_gain = -1;

    gfp->lowpassfreq = 0;
    gfp->highpassfreq = 0;
    gfp->lowpasswidth = -1;
    gfp->highpasswidth = -1;

    gfp->VBR = vbr_off;
    gfp->VBR_q = 4;
    gfp->VBR_mean_bitrate_kbps = 128;
    gfp->VBR_min_bitrate_kbps = 0;
    gfp->VBR_max_bitrate_kbps = 0;
    gfp->VBR_hard_min = 0;

    gfp->quant_comp = -1;
    gfp->quant_comp_short = -1;

    gfp->msfix = -1;

    gfp->attackthre = -1;
    gfp->attackthre_s = -1;

    gfp->scale = 1;
    gfp->scale_left = 1;
    gfp->scale_right = 1;

    gfp->ATHcurve = -1;
    gfp->ATHtype = -1;  /* default = -1 = set in lame_init_params */
    /* 2 = equal loudness curve */
    gfp->athaa_sensitivity = 0.0; /* no offset */
    gfp->athaa_type = -1;
    gfp->useTemporal = -1;
    gfp->interChRatio = -1;

    gfp->findReplayGain = 0;
    gfp->decode_on_the_fly = 0;

    gfp->asm_optimizations.sse = 1;
    gfp->asm_optimizations.avx2 = 1;
    gfp->vector_routines_request = VECTOR_IMPL_AUTO;

    gfp->preset = 0;

    gfp->write_id3tag_automatic = 1;

    gfp->report.debugf = &lame_report_def;
    gfp->report.errorf = &lame_report_def;
    gfp->report.msgf = &lame_report_def;

    gfp->internal_flags = lame_calloc(lame_internal_flags, 1);

    if (lame_init_internal_flags(gfp->internal_flags) < 0) {
        freegfc(gfp->internal_flags);
        gfp->internal_flags = 0;
        return -1;
    }
    return 0;
}


/*! Create an encoder instance and fill it with the default settings. */
/*!
  \ingroup api_encoding
  The first call of the encoding API. It creates the encoder instance that
  every other \c lame_* function takes, and sets every parameter to its
  default. So a caller only sets what it wants to change.

  This function checks nothing, and no encoding state exists yet.
  \c lame_init_params() does that, after all parameters are set. The usual
  order is:

  \code
  lame_global_flags *gfp = lame_init();
  if (gfp == NULL)
      return -1;                       // out of memory
  lame_set_in_samplerate(gfp, 44100);
  lame_set_num_channels(gfp, 2);
  lame_set_VBR(gfp, vbr_default);
  if (lame_init_params(gfp) < 0) {     // checks the settings, prepares the encoder
      lame_close(gfp);
      return -1;                       // the settings cannot be used
  }
  // ... lame_encode_buffer() per block of PCM ...
  lame_encode_flush(gfp, mp3buf, sizeof mp3buf);
  lame_close(gfp);
  \endcode

  Release every instance from this function with \c lame_close(), also after a
  failed \c lame_init_params().

  All encoder state is in the returned instance. Two encoders need one
  instance each. This does not make LAME thread-safe: **LAME gives no
  thread-safety guarantee**. A caller that uses LAME from more than one thread
  must do its own locking.

  \return a new encoder instance, owned by the caller, or \c NULL if memory
          allocation fails.
*/
lame_global_flags *
lame_init(void)
{
    lame_global_flags *gfp;
    int     ret;

    init_log_table();

    gfp = lame_calloc(lame_global_flags, 1);
    if (gfp == NULL)
        return NULL;

    ret = lame_init_old(gfp);
    if (ret != 0) {
        free(gfp);
        return NULL;
    }

    gfp->lame_allocated_gfp = 1;
    return gfp;
}


/***********************************************************************
 *
 *  some simple statistics
 *
 *  Robert Hegemann 2000-10-11
 *
 ***********************************************************************/

/*  histogram of used bitrate indexes:
 *  One has to weight them to calculate the average bitrate in kbps
 *
 *  bitrate indices:
 *  there are 14 possible bitrate indices, 0 has the special meaning
 *  "free format" which is not possible to mix with VBR and 15 is forbidden
 *  anyway.
 *
 *  stereo modes:
 *  0: LR   number of left-right encoded frames
 *  1: LR-I number of left-right and intensity encoded frames
 *  2: MS   number of mid-side encoded frames
 *  3: MS-I number of mid-side and intensity encoded frames
 *
 *  4: number of encoded frames
 *
 */

/*! The bitrate each slot of the histograms stands for. */
/*!
  \ingroup api_statistics
  Fills \a bitrate_kbps with the 14 bitrates of the MPEG version that this
  instance encodes, in the order of the slots of \c lame_bitrate_hist(). Read
  the two together to turn a count into a bitrate.

  A free-format encode has one bitrate, not 14. Slot 0 then holds it, and the
  other 13 are -1.

  \param gfp           the encoder instance.
  \param bitrate_kbps  receives 14 bitrates in kbps.
*/
void
lame_bitrate_kbps(const lame_global_flags * gfp, int bitrate_kbps[14])
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            SessionConfig_t const *const cfg = &gfc->cfg;
            int     i;
            if (cfg->free_format) {
                for (i = 0; i < 14; i++)
                    bitrate_kbps[i] = -1;
                bitrate_kbps[0] = cfg->avg_bitrate;
            }
            else {
                for (i = 0; i < 14; i++)
                    bitrate_kbps[i] = bitrate_table[cfg->version][i + 1];
            }
        }
    }
}


/*! How many frames were written at each bitrate. */
/*!
  \ingroup api_statistics
  Fills \a bitrate_count with the number of frames written at each of the 14
  bitrates. For a VBR encode this is the bitrate distribution. A CBR encode
  puts every frame into one slot. \c lame_bitrate_kbps() returns the bitrate
  of each slot.

  \param gfp            the encoder instance.
  \param bitrate_count  receives 14 frame counts.
*/
void
lame_bitrate_hist(const lame_global_flags * gfp, int bitrate_count[14])
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            SessionConfig_t const *const cfg = &gfc->cfg;
            EncResult_t const *const eov = &gfc->ov_enc;
            int     i;

            if (cfg->free_format) {
                for (i = 0; i < 14; i++) {
                    bitrate_count[i] = 0;
                }
                bitrate_count[0] = eov->bitrate_channelmode_hist[0][4];
            }
            else {
                for (i = 0; i < 14; i++) {
                    bitrate_count[i] = eov->bitrate_channelmode_hist[i + 1][4];
                }
            }
        }
    }
}


/*! How many frames were written in each stereo mode. */
/*!
  \ingroup api_statistics
  Fills \a stmode_count with the number of frames written in each of the four
  stereo modes, over all bitrates. It shows how much of a joint stereo encode
  used mid/side.

  \param gfp           the encoder instance.
  \param stmode_count  receives 4 frame counts.
*/
void
lame_stereo_mode_hist(const lame_global_flags * gfp, int stmode_count[4])
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            EncResult_t const *const eov = &gfc->ov_enc;
            int     i;

            for (i = 0; i < 4; i++) {
                stmode_count[i] = eov->bitrate_channelmode_hist[15][i];
            }
        }
    }
}



/*! Stereo modes broken down by bitrate. */
/*!
  \ingroup api_statistics
  Fills \a bitrate_stmode_count with the frame counts of
  \c lame_stereo_mode_hist(), split by bitrate slot:
  <tt>[bitrate][stereo mode]</tt>. The sum of a column is the value of
  \c lame_stereo_mode_hist().

  \param gfp                   the encoder instance.
  \param bitrate_stmode_count  receives 14 by 4 frame counts.
*/
void
lame_bitrate_stereo_mode_hist(const lame_global_flags * gfp, int bitrate_stmode_count[14][4])
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            SessionConfig_t const *const cfg = &gfc->cfg;
            EncResult_t const *const eov = &gfc->ov_enc;
            int     i;
            int     j;

            if (cfg->free_format) {
                for (j = 0; j < 14; j++)
                    for (i = 0; i < 4; i++) {
                        bitrate_stmode_count[j][i] = 0;
                    }
                for (i = 0; i < 4; i++) {
                    bitrate_stmode_count[0][i] = eov->bitrate_channelmode_hist[0][i];
                }
            }
            else {
                for (j = 0; j < 14; j++) {
                    for (i = 0; i < 4; i++) {
                        bitrate_stmode_count[j][i] = eov->bitrate_channelmode_hist[j + 1][i];
                    }
                }
            }
        }
    }
}


/*! How often each block type was chosen. */
/*!
  \ingroup api_statistics
  Fills \a btype_count with the number of times each block type was used, over
  all bitrates. The slots are 0 normal, 1 start, 2 short, 3 stop, 4 mixed, and
  5 the total of the other five.

  These are not frame counts. The block type is chosen for each granule and
  channel. So one frame adds its number of granules times its number of
  channels: four for an MPEG-1 stereo frame. Divide by slot 5 to get a
  proportion.

  \param gfp          the encoder instance.
  \param btype_count  receives 5 counts and their total.
*/
void
lame_block_type_hist(const lame_global_flags * gfp, int btype_count[6])
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            EncResult_t const *const eov = &gfc->ov_enc;
            int     i;

            for (i = 0; i < 6; ++i) {
                btype_count[i] = eov->bitrate_blocktype_hist[15][i];
            }
        }
    }
}



/*! Block types broken down by bitrate. */
/*!
  \ingroup api_statistics
  Fills \a bitrate_btype_count with the counts of \c lame_block_type_hist(),
  split by bitrate slot: <tt>[bitrate][block type]</tt>, with the total for
  each bitrate in column 5. The sum of a column is the value of
  \c lame_block_type_hist().

  \param gfp                  the encoder instance.
  \param bitrate_btype_count  receives 14 by 6 counts.
*/
void
lame_bitrate_block_type_hist(const lame_global_flags * gfp, int bitrate_btype_count[14][6])
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            SessionConfig_t const *const cfg = &gfc->cfg;
            EncResult_t const *const eov = &gfc->ov_enc;
            int     i, j;

            if (cfg->free_format) {
                for (j = 0; j < 14; ++j) {
                    for (i = 0; i < 6; ++i) {
                        bitrate_btype_count[j][i] = 0;
                    }
                }
                for (i = 0; i < 6; ++i) {
                    bitrate_btype_count[0][i] = eov->bitrate_blocktype_hist[0][i];
                }
            }
            else {
                for (j = 0; j < 14; ++j) {
                    for (i = 0; i < 6; ++i) {
                        bitrate_btype_count[j][i] = eov->bitrate_blocktype_hist[j + 1][i];
                    }
                }
            }
        }
    }
}

/* end of lame.c */
