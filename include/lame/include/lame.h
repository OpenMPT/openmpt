/*
 *	Interface to MP3 LAME encoding engine
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
 *  \file lame.h
 *  \brief The public libmp3lame interface.
 *
 *  Everything a program linking against libmp3lame may use is declared here.
 *  Nothing else in the source tree is part of the interface, and no other
 *  header is installed.
 *
 *  A program creates an encoder, sets the parameters and the tag fields,
 *  calls lame_init_params() once, then feeds audio and collects MP3 frames.
 *  Functions that report on the encode need lame_init_params() first. A call
 *  outside this order is a programming error: the library may stop with an
 *  assertion, and promises nothing else.
 */


#ifndef LAME_LAME_H
#define LAME_LAME_H

/* for size_t typedef */
#include <stddef.h>
/* for va_list typedef */
#include <stdarg.h>
/* for FILE typedef, TODO: remove when removing lame_mp3_tags_fid */
#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(WIN32) || defined(_WIN32)
#include <windows.h>  /* OpenMPT */
#if 0  /* OpenMPT */
#undef CDECL
#define CDECL __cdecl
#endif  /* OpenMPT */
#else
#ifndef CDECL  /* OpenMPT */
#define CDECL
#endif  /* OpenMPT */
#endif

/**
 * The type of the functions that receive the library's messages, errors and
 * debug reports: see lame_set_msgf(), lame_set_errorf() and
 * lame_set_debugf(). Such a function uses the calling convention of the
 * library's functions, also in a program built with another default.
 */
typedef void (CDECL *lame_report_function)(const char *format, va_list ap);

#define DEPRECATED_OR_OBSOLETE_CODE_REMOVED 1

/**
 * \ingroup api_settings
 * How the encoder chooses a bitrate. Set with lame_set_VBR().
 */
typedef enum vbr_mode_e {
  vbr_off=0,            /**< constant bitrate */
  vbr_mt,               /**< obsolete, same as \c vbr_mtrh */
  vbr_rh,               /**< variable bitrate, the older implementation
                             (\c --vbr-old in the lame tool) */
  vbr_abr,              /**< average bitrate */
  vbr_mtrh,             /**< variable bitrate, the faster implementation
                             (\c --vbr-new in the lame tool) */
  vbr_max_indicator,    /* Don't use this! It's used for sanity checks.       */
  vbr_default=vbr_mtrh  /**< the recommended variable bitrate mode. A new
                             encoder instance uses \c vbr_off. */
} vbr_mode;


/**
 * \ingroup api_settings
 * How the encoder codes the channels. Set with lame_set_mode().
 */
typedef enum MPEG_mode_e {
  STEREO = 0,     /**< two channels, coded independently of each other */
  JOINT_STEREO,   /**< two channels. Mid/side coding is used in frames where
                       it saves bits. */
  DUAL_CHANNEL,   /**< dual channel. LAME does not implement this mode.
                       lame_set_mode() accepts it without an error. */
  MONO,           /**< one channel. Two-channel input is mixed to one
                       channel. */
  NOT_SET,        /**< the default. LAME chooses the mode in
                       lame_init_params(). */
  MAX_INDICATOR   /* Don't use this! It's used for sanity checks. */
} MPEG_mode;

/**
 * \ingroup api_settings
 * \deprecated The encoder decides the padding of each frame. No function takes
 * this type.
 */
typedef enum Padding_type_e {
  PAD_NO = 0,         /**< never pad */
  PAD_ALL,            /**< always pad */
  PAD_ADJUST,         /**< pad as needed. The encoder always does this. */
  PAD_MAX_INDICATOR   /* Don't use this! It's used for sanity checks. */
} Padding_type;



/**
 * \ingroup api_settings
 * The values that lame_set_preset() accepts:
 *
 * - 8 to 320: average bitrate (ABR) in kbps. Pass the bitrate itself.
 * - 410 to 500: a VBR quality level.
 * - 1000 to 1007: the older named presets.
 *
 * Each VBR quality level has two names for the same value. \c Vx is the name
 * the lame tool uses. \c VBR_xx counts in the opposite direction. Each named
 * preset selects a VBR quality level and \c vbr_mtrh. The exception is
 * \c INSANE, which selects 320 kbps constant bitrate.
 */
typedef enum preset_mode_e {
    ABR_8 = 8,      /**< average bitrate, 8 kbps, the lowest ABR value */
    ABR_320 = 320,  /**< average bitrate, 320 kbps, the highest ABR value */

    V9 = 410,       /**< VBR quality 9, the smallest files */
    VBR_10 = 410,   /**< another spelling of \c V9 */
    V8 = 420,       /**< VBR quality 8 */
    VBR_20 = 420,   /**< another spelling of \c V8 */
    V7 = 430,       /**< VBR quality 7 */
    VBR_30 = 430,   /**< another spelling of \c V7 */
    V6 = 440,       /**< VBR quality 6 */
    VBR_40 = 440,   /**< another spelling of \c V6 */
    V5 = 450,       /**< VBR quality 5 */
    VBR_50 = 450,   /**< another spelling of \c V5 */
    V4 = 460,       /**< VBR quality 4 */
    VBR_60 = 460,   /**< another spelling of \c V4 */
    V3 = 470,       /**< VBR quality 3 */
    VBR_70 = 470,   /**< another spelling of \c V3 */
    V2 = 480,       /**< VBR quality 2 */
    VBR_80 = 480,   /**< another spelling of \c V2 */
    V1 = 490,       /**< VBR quality 1 */
    VBR_90 = 490,   /**< another spelling of \c V1 */
    V0 = 500,       /**< VBR quality 0, the largest files */
    VBR_100 = 500,  /**< another spelling of \c V0 */



    /*still there for compatibility*/
    R3MIX = 1000,        /**< \c V3 */
    STANDARD = 1001,     /**< \c V2 */
    EXTREME = 1002,      /**< \c V0 */
    INSANE = 1003,       /**< 320 kbps, constant bitrate */
    STANDARD_FAST = 1004,/**< \c V2, as \c STANDARD */
    EXTREME_FAST = 1005, /**< \c V0, as \c EXTREME */
    MEDIUM = 1006,       /**< \c V4 */
    MEDIUM_FAST = 1007   /**< \c V4, as \c MEDIUM */
} preset_mode;


/**
 * \ingroup api_settings
 * asm optimizations
 *
 * \deprecated Use lame_set_vector_routines() and the calls beside it.
 *
 * lame_set_asm_optimizations() and lame_get_asm_optimizations() still work.
 * \c SSE switches all vector routines on or off. These routines use SSE2.
 *
 * The library has no MMX or 3DNow! code. For \c MMX and \c AMD_3DNOW the
 * setter returns -2 and the getter returns 0. These values stay in the enum
 * so that existing source code still compiles.
 */
typedef enum asm_optimizations_e {
    MMX = 1,        /**< no MMX code in this library. The setter returns -2. */
    AMD_3DNOW = 2,  /**< no 3DNow! code in this library. The setter returns
                         -2. */
    SSE = 3,        /**< switches all vector routines */
    AVX2 = 4        /**< AVX2 */
} asm_optimizations;


/* psychoacoustic model */
typedef enum Psy_model_e {
    PSY_GPSYCHO = 1,
    PSY_NSPSYTUNE = 2
} Psy_model;


/**
 * \ingroup api_settings
 * The maximum size of the bit reservoir. Set with lame_set_strict_ISO().
 */
typedef enum buffer_constraint_e {
    MDB_DEFAULT=0,     /**< a limit that all common decoders support */
    MDB_STRICT_ISO=1,  /**< the limit that the ISO standard allows */
    MDB_MAXIMUM=2      /**< the largest size the format allows. This is the
                            default value. \c MDB_DEFAULT is not the default,
                            despite its name. */
} buffer_constraint;


struct lame_global_struct;
typedef struct lame_global_struct lame_global_flags;
typedef lame_global_flags *lame_t;




/***********************************************************************
 *
 *  The LAME API
 *  Call lame_init_params() once for each encoder instance, after all
 *  settings.  See the file "API" for more documentation
 *
 ***********************************************************************/


/*
 * REQUIRED:
 * Creates an encoder and sets all parameters to their default values.
 * Returns the encoder instance, which every other encoder call takes as
 * its first argument. Returns NULL if memory allocation fails.
 */
lame_global_flags * CDECL lame_init(void);
#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
#else
/* obsolete version */
int CDECL lame_init_old(lame_global_flags *);
#endif

/*
 * OPTIONAL:
 * set as needed to override defaults.
 * Set these before lame_init_params(). A call after it has no defined effect.
 * Before lame_init_params(), a getter returns the value that was set. After
 * it, the getter returns the value the encoder uses.
 */

/********************************************************************
 *  input stream description
 ***********************************************************************/
/* number of samples.  default = 2^32-1
   Set it before lame_init_params(). The ID3v2 tag and
   lame_get_totalframes() use it. */
int CDECL lame_set_num_samples(lame_global_flags *, unsigned long);
unsigned long CDECL lame_get_num_samples(const lame_global_flags *);

/* input sample rate in Hz.  default = 44100 Hz */
int CDECL lame_set_in_samplerate(lame_global_flags *, int);
int CDECL lame_get_in_samplerate(const lame_global_flags *);

/* number of channels in input stream. default=2  */
int CDECL lame_set_num_channels(lame_global_flags *, int);
int CDECL lame_get_num_channels(const lame_global_flags *);

/*
  scale the input by this amount before encoding.  default=1
  The decoder does not use it.
*/
int CDECL lame_set_scale(lame_global_flags *, float);
float CDECL lame_get_scale(const lame_global_flags *);

/*
  scale the channel 0 (left) input by this amount before encoding.  default=1
  The decoder does not use it.
*/
int CDECL lame_set_scale_left(lame_global_flags *, float);
float CDECL lame_get_scale_left(const lame_global_flags *);

/*
  scale the channel 1 (right) input by this amount before encoding.  default=1
  The decoder does not use it.
*/
int CDECL lame_set_scale_right(lame_global_flags *, float);
float CDECL lame_get_scale_right(const lame_global_flags *);

/*
  output sample rate in Hz.  default = 0: LAME chooses the sample rate from
  the bitrate.  MPEG allows only these rates (kHz):
  MPEG1    32, 44.1,   48
  MPEG2    16, 22.05,  24
  MPEG2.5   8, 11.025, 12
  lame_init_params() fails if the output sample rate is more than 128 times
  the input sample rate. This also applies when LAME chooses the rate.
  The decoder does not use it.
*/
int CDECL lame_set_out_samplerate(lame_global_flags *, int);
int CDECL lame_get_out_samplerate(const lame_global_flags *);


/********************************************************************
 *  general control parameters
 ***********************************************************************/
/* 1=cause LAME to collect data for an MP3 frame analyzer. default=0 */
int CDECL lame_set_analysis(lame_global_flags *, int);
int CDECL lame_get_analysis(const lame_global_flags *);

/*
  1 = write the LAME tag frame (a Xing/Info header) at the start of the stream.
  default = 1
*/
int CDECL lame_set_bWriteVbrTag(lame_global_flags *, int);
int CDECL lame_get_bWriteVbrTag(const lame_global_flags *);

/* 1=decode only.  the lame tool uses it to convert mp3 to wav.  default=0 */
int CDECL lame_set_decode_only(lame_global_flags *, int);
int CDECL lame_get_decode_only(const lame_global_flags *);

#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
#else
/* 1=encode a Vorbis .ogg file.  default=0 */
/* DEPRECATED */
int CDECL lame_set_ogg(lame_global_flags *, int);
int CDECL lame_get_ogg(const lame_global_flags *);
#endif

/*
  selects the encoding algorithms, from 0 to 9.  The bitrate decides most of
  the quality.  This setting chooses between slow algorithms with better
  quality and fast ones.  0 is the best and slowest, 9 the worst and fastest.
  recommended:  2     near-best quality, not too slow
                5     good quality, fast
                7     acceptable quality, very fast
*/
int CDECL lame_set_quality(lame_global_flags *, int);
int CDECL lame_get_quality(const lame_global_flags *);

/*
  mode = 0,1,2,3 = stereo, joint stereo, dual channel (not supported), mono
  default: LAME chooses from the compression ratio and the number of input
  channels
*/
int CDECL lame_set_mode(lame_global_flags *, MPEG_mode);
MPEG_mode CDECL lame_get_mode(const lame_global_flags *);

#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
#else
/*
  mode_automs.  Use a M/S mode with a switching threshold based on
  compression ratio
  DEPRECATED
*/
int CDECL lame_set_mode_automs(lame_global_flags *, int);
int CDECL lame_get_mode_automs(const lame_global_flags *);
#endif

/*
  force_ms.  Force M/S for all frames.  For testing only.
  default = 0 (disabled)
*/
int CDECL lame_set_force_ms(lame_global_flags *, int);
int CDECL lame_get_force_ms(const lame_global_flags *);

/* use free_format?  default = 0 (disabled)
   The bitrate is then at most 640 kbps; a higher one is encoded at 640. */
int CDECL lame_set_free_format(lame_global_flags *, int);
int CDECL lame_get_free_format(const lame_global_flags *);

/* perform ReplayGain analysis?  default = 0 (disabled) */
int CDECL lame_set_findReplayGain(lame_global_flags *, int);
int CDECL lame_get_findReplayGain(const lame_global_flags *);

/* decode on the fly. Search for the peak sample. If the ReplayGain
 * analysis is enabled then perform the analysis on the decoded data
 * stream. default = 0 (disabled)
 * NOTE: if this option is set the built-in decoder should not be used */
int CDECL lame_set_decode_on_the_fly(lame_global_flags *, int);
int CDECL lame_get_decode_on_the_fly(const lame_global_flags *);

#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
#else
/* DEPRECATED: now does the same as lame_set_findReplayGain()
   default = 0 (disabled) */
int CDECL lame_set_ReplayGain_input(lame_global_flags *, int);
int CDECL lame_get_ReplayGain_input(const lame_global_flags *);

/* DEPRECATED: now does the same as
   lame_set_decode_on_the_fly() && lame_set_findReplayGain()
   default = 0 (disabled) */
int CDECL lame_set_ReplayGain_decode(lame_global_flags *, int);
int CDECL lame_get_ReplayGain_decode(const lame_global_flags *);

/* DEPRECATED: now does the same as lame_set_decode_on_the_fly()
   default = 0 (disabled) */
int CDECL lame_set_findPeakSample(lame_global_flags *, int);
int CDECL lame_get_findPeakSample(const lame_global_flags *);
#endif

/* counters for gapless encoding. These can also be set after
   lame_init_params(). Set them before the LAME tag of the file is written. */
int CDECL lame_set_nogap_total(lame_global_flags*, int);
int CDECL lame_get_nogap_total(const lame_global_flags*);

int CDECL lame_set_nogap_currentindex(lame_global_flags* , int);
int CDECL lame_get_nogap_currentindex(const lame_global_flags*);


/*
 * OPTIONAL:
 * Sets the functions that print error, debug and info messages.
 * The second argument is a pointer to a function like this one:
 *   void my_debugf(const char *format, va_list ap)
 *   {
 *       (void) vfprintf(stdout, format, ap);
 *   }
 * By default, LAME uses its own function, which prints to stderr.
 * If you pass NULL, LAME prints nothing on that stream.
 * Call them before lame_init_params(); it takes the functions over.
 */
int CDECL lame_set_errorf(lame_global_flags *, lame_report_function);
int CDECL lame_set_debugf(lame_global_flags *, lame_report_function);
int CDECL lame_set_msgf  (lame_global_flags *, lame_report_function);



/* set either the bitrate or the compression ratio.  default is a compression
   ratio of 11.025 */
int CDECL lame_set_brate(lame_global_flags *, int);
int CDECL lame_get_brate(const lame_global_flags *);
int CDECL lame_set_compression_ratio(lame_global_flags *, float);
float CDECL lame_get_compression_ratio(const lame_global_flags *);


int CDECL lame_set_preset( lame_global_flags*  gfp, int );
int CDECL lame_set_asm_optimizations( lame_global_flags*  gfp, int, int );
int CDECL lame_get_asm_optimizations( const lame_global_flags*  gfp, int );

/*
 * Which vector routines the encoder runs.
 *
 * This replaces lame_set_asm_optimizations() and lame_get_asm_optimizations().
 * Each set of routines has a name.  Names are lowercase and name the real
 * instruction set ("sse2", not "sse").  The displayed form is the name in
 * uppercase.  lame_get_vector_routines() returns NULL before
 * lame_init_params().
 */
int CDECL lame_get_num_vector_routines(void);
const char* CDECL lame_get_vector_routines_name(int index);
int CDECL lame_set_vector_routines(lame_global_flags*, const char* name);
const char* CDECL lame_get_vector_routines(const lame_global_flags*);



/********************************************************************
 *  frame params
 ***********************************************************************/
/* mark as copyright.  default=0 */
int CDECL lame_set_copyright(lame_global_flags *, int);
int CDECL lame_get_copyright(const lame_global_flags *);

/* mark as original.  default=1 */
int CDECL lame_set_original(lame_global_flags *, int);
int CDECL lame_get_original(const lame_global_flags *);

/* error_protection.  Use 2 bytes from each frame for CRC checksum. default=0 */
int CDECL lame_set_error_protection(lame_global_flags *, int);
int CDECL lame_get_error_protection(const lame_global_flags *);

#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
#else
/* padding_type. 0=pad no frames  1=pad all frames 2=adjust padding(default) */
int CDECL lame_set_padding_type(lame_global_flags *, Padding_type);
Padding_type CDECL lame_get_padding_type(const lame_global_flags *);
#endif

/* MP3 'private extension' bit.  LAME gives it no meaning.  default=0 */
int CDECL lame_set_extension(lame_global_flags *, int);
int CDECL lame_get_extension(const lame_global_flags *);

/* limit the bit reservoir, see buffer_constraint.  default=MDB_MAXIMUM */
int CDECL lame_set_strict_ISO(lame_global_flags *, int);
int CDECL lame_get_strict_ISO(const lame_global_flags *);


/********************************************************************
 * quantization/noise shaping
 ***********************************************************************/

/* disable the bit reservoir. For testing only. default=0 */
int CDECL lame_set_disable_reservoir(lame_global_flags *, int);
int CDECL lame_get_disable_reservoir(const lame_global_flags *);

/* select a different "best quantization" function. default=0  */
int CDECL lame_set_quant_comp(lame_global_flags *, int);
int CDECL lame_get_quant_comp(const lame_global_flags *);
int CDECL lame_set_quant_comp_short(lame_global_flags *, int);
int CDECL lame_get_quant_comp_short(const lame_global_flags *);

int CDECL lame_set_experimentalX(lame_global_flags *, int); /* compatibility*/
int CDECL lame_get_experimentalX(const lame_global_flags *);

/* another experimental option.  for testing only */
int CDECL lame_set_experimentalY(lame_global_flags *, int);
int CDECL lame_get_experimentalY(const lame_global_flags *);

/* another experimental option.  for testing only */
int CDECL lame_set_experimentalZ(lame_global_flags *, int);
int CDECL lame_get_experimentalZ(const lame_global_flags *);

/* Naoki's psychoacoustic model.  default=0 */
int CDECL lame_set_exp_nspsytune(lame_global_flags *, int);
int CDECL lame_get_exp_nspsytune(const lame_global_flags *);

void CDECL lame_set_msfix(lame_global_flags *, double);
float CDECL lame_get_msfix(const lame_global_flags *);


/********************************************************************
 * VBR control
 ***********************************************************************/
/* Types of VBR.  default = vbr_off = CBR */
int CDECL lame_set_VBR(lame_global_flags *, vbr_mode);
vbr_mode CDECL lame_get_VBR(const lame_global_flags *);

/* VBR quality level.  0=highest  9=lowest  */
int CDECL lame_set_VBR_q(lame_global_flags *, int);
int CDECL lame_get_VBR_q(const lame_global_flags *);

/* VBR quality level.  0=highest  9=lowest.  From 0 to less than 10  */
int CDECL lame_set_VBR_quality(lame_global_flags *, float);
float CDECL lame_get_VBR_quality(const lame_global_flags *);

/* average bitrate in kbps for ABR mode.  CBR mode also uses it when
   lame_set_brate() was not called */
int CDECL lame_set_VBR_mean_bitrate_kbps(lame_global_flags *, int);
int CDECL lame_get_VBR_mean_bitrate_kbps(const lame_global_flags *);

int CDECL lame_set_VBR_min_bitrate_kbps(lame_global_flags *, int);
int CDECL lame_get_VBR_min_bitrate_kbps(const lame_global_flags *);

int CDECL lame_set_VBR_max_bitrate_kbps(lame_global_flags *, int);
int CDECL lame_get_VBR_max_bitrate_kbps(const lame_global_flags *);

/*
  1 = every frame uses at least the minimum VBR bitrate.  By default, silent
  frames can use less
*/
int CDECL lame_set_VBR_hard_min(lame_global_flags *, int);
int CDECL lame_get_VBR_hard_min(const lame_global_flags *);

/* for preset */
#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
#else
int CDECL lame_set_preset_expopts(lame_global_flags *, int);
#endif

/********************************************************************
 * Filtering control
 ***********************************************************************/
/* freq in Hz to apply lowpass. Default = 0: LAME chooses.  -1 = disabled */
int CDECL lame_set_lowpassfreq(lame_global_flags *, int);
int CDECL lame_get_lowpassfreq(const lame_global_flags *);
/* width of transition band, in Hz.  Default = one polyphase filter band */
int CDECL lame_set_lowpasswidth(lame_global_flags *, int);
int CDECL lame_get_lowpasswidth(const lame_global_flags *);

/* freq in Hz to apply highpass. Default = 0: LAME chooses.  -1 = disabled */
int CDECL lame_set_highpassfreq(lame_global_flags *, int);
int CDECL lame_get_highpassfreq(const lame_global_flags *);
/* width of transition band, in Hz.  Default = one polyphase filter band */
int CDECL lame_set_highpasswidth(lame_global_flags *, int);
int CDECL lame_get_highpasswidth(const lame_global_flags *);


/********************************************************************
 * psychoacoustics and other arguments which you should not change
 * unless you know what you are doing
 ***********************************************************************/

/* only use ATH for masking */
int CDECL lame_set_ATHonly(lame_global_flags *, int);
int CDECL lame_get_ATHonly(const lame_global_flags *);

/* only use ATH for short blocks */
int CDECL lame_set_ATHshort(lame_global_flags *, int);
int CDECL lame_get_ATHshort(const lame_global_flags *);

/* disable ATH */
int CDECL lame_set_noATH(lame_global_flags *, int);
int CDECL lame_get_noATH(const lame_global_flags *);

/* select ATH formula */
int CDECL lame_set_ATHtype(lame_global_flags *, int);
int CDECL lame_get_ATHtype(const lame_global_flags *);

/* lower ATH by this many dB */
int CDECL lame_set_ATHlower(lame_global_flags *, float);
float CDECL lame_get_ATHlower(const lame_global_flags *);

/* select ATH adaptive adjustment type */
int CDECL lame_set_athaa_type( lame_global_flags *, int);
int CDECL lame_get_athaa_type( const lame_global_flags *);

#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
#else
/* select the loudness approximation used by the ATH adaptive auto-leveling  */
int CDECL lame_set_athaa_loudapprox( lame_global_flags *, int);
int CDECL lame_get_athaa_loudapprox( const lame_global_flags *);
#endif

/* adjust (in dB) the point below which adaptive ATH level adjustment occurs */
int CDECL lame_set_athaa_sensitivity( lame_global_flags *, float);
float CDECL lame_get_athaa_sensitivity( const lame_global_flags* );

#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
#else
/* OBSOLETE: predictability limit (ISO tonality formula) */
int CDECL lame_set_cwlimit(lame_global_flags *, int);
int CDECL lame_get_cwlimit(const lame_global_flags *);
#endif

/*
  allow block types to differ between channels?
  default: 0 for stereo and joint stereo
*/
int CDECL lame_set_allow_diff_short(lame_global_flags *, int);
int CDECL lame_get_allow_diff_short(const lame_global_flags *);

/* use temporal masking effect.  default = 1, but 0 with vbr_mtrh */
int CDECL lame_set_useTemporal(lame_global_flags *, int);
int CDECL lame_get_useTemporal(const lame_global_flags *);

/* inter-channel masking ratio, 0 to 1 (default = 0) */
int CDECL lame_set_interChRatio(lame_global_flags *, float);
float CDECL lame_get_interChRatio(const lame_global_flags *);

/* disable short blocks */
int CDECL lame_set_no_short_blocks(lame_global_flags *, int);
int CDECL lame_get_no_short_blocks(const lame_global_flags *);

/* force short blocks */
int CDECL lame_set_force_short_blocks(lame_global_flags *, int);
int CDECL lame_get_force_short_blocks(const lame_global_flags *);

/* Marks the input as emphasized PCM, as on a few CDs.  Do not use this.
   LAME only writes the field.  The psychoacoustic model does not take the
   emphasis into account, and many decoders ignore the field */
int CDECL lame_set_emphasis(lame_global_flags *, int);
int CDECL lame_get_emphasis(const lame_global_flags *);



/************************************************************************/
/* internal variables, cannot be set...                                 */
/* provided because they may be of use to calling application           */
/* Call these after lame_init_params(). Before it, they return 0.       */
/************************************************************************/
/* version  0=MPEG-2  1=MPEG-1  (2=MPEG-2.5)
   Before lame_init_params() it returns 0, which is also the value for
   MPEG-2. */
int CDECL lame_get_version(const lame_global_flags *);

/* encoder delay   */
int CDECL lame_get_encoder_delay(const lame_global_flags *);

/*
  padding appended to the input, so that a decoder can decode all of the
  input.  LAME computes this value in lame_encode_flush().  Before that
  call it is 0.
*/
int CDECL lame_get_encoder_padding(const lame_global_flags *);

/* size of MPEG frame */
int CDECL lame_get_framesize(const lame_global_flags *);

/* number of PCM samples buffered, but not yet encoded to mp3 data. */
int CDECL lame_get_mf_samples_to_encode( const lame_global_flags*  gfp );

/*
  number of bytes of MP3 data that the encoder has produced but not yet
  returned.  lame_encode_flush_nogap() returns this many bytes.
  lame_encode_flush() returns more, because it first encodes the PCM
  samples that are still in the buffer.
*/
int CDECL lame_get_size_mp3buffer( const lame_global_flags*  gfp );

/* number of frames encoded so far */
int CDECL lame_get_frameNum(const lame_global_flags *);

/*
  LAME's estimate of the total number of frames to be encoded.
  Valid only after lame_init_params(), and only if the program set
  num_samples.
*/
int CDECL lame_get_totalframes(const lame_global_flags *);

/* RadioGain value. Multiplied by 10 and rounded to the nearest. */
int CDECL lame_get_RadioGain(const lame_global_flags *);

/* AudiophileGain value. Multiplied by 10 and rounded to the nearest. */
int CDECL lame_get_AudiophileGain(const lame_global_flags *);

/* the peak sample */
float CDECL lame_get_PeakSample(const lame_global_flags *);

/* Gain change required for preventing clipping. The value is correct only if
   peak sample searching was enabled. If negative then the waveform
   already does not clip. The value is multiplied by 10 and rounded up. */
int CDECL lame_get_noclipGainChange(const lame_global_flags *);

/* user-specified scale factor required for preventing clipping. Value is
   correct only if peak sample searching was enabled and no user-specified
   scaling was performed. If negative then either the waveform already does
   not clip or the value cannot be determined */
float CDECL lame_get_noclipScale(const lame_global_flags *);

/* returns the largest number of PCM samples that one encode call can take,
   so that the output fits into a buffer of buffer_size bytes.
   Call it after lame_init_params(). Before it, it returns -1. */
int CDECL lame_get_maximum_number_of_samples(lame_t gfp, size_t buffer_size);





/*
 * REQUIRED:
 * checks the settings and prepares the encoder.
 * returns -1 if something failed.
 * Call it once, after the settings and before the first encode call. A
 * second call after a success returns -1. After a failure the settings are
 * as they were, and it can be called again.
 */
int CDECL lame_init_params(lame_global_flags *);


/*
 * OPTIONAL:
 * get the version number as a string, such as
 * "3.63 (beta)" or just "3.63".
 */
const char*  CDECL get_lame_version       ( void );
const char*  CDECL get_lame_short_version ( void );
const char*  CDECL get_lame_very_short_version ( void );
const char*  CDECL get_psy_version        ( void );
const char*  CDECL get_lame_url           ( void );
const char*  CDECL get_lame_os_bitness    ( void );

/**
 * \ingroup api_version
 * The version of LAME and of its psychoacoustic model in comparable form.
 *
 * Filled in by get_lame_version_numerical(). Every field is an integer, so a
 * caller can test for a minimum version without parsing a version string.
 */
typedef struct {
    /* generic LAME version */
    int major;               /**< major version number                      */
    int minor;               /**< minor version number                      */
    int alpha;               /**< alpha patch level, 0 if not an alpha version */
    int beta;                /**< beta patch level, 0 if not a beta version */

    /* version of the psy model */
    int psy_major;           /**< major version of the psychoacoustic model */
    int psy_minor;           /**< minor version of the psychoacoustic model */
    int psy_alpha;           /**< 0 if not an alpha version                 */
    int psy_beta;            /**< 0 if not a beta version                   */

    /* compile time features */
    const char *features;    /**< retained for compatibility, always empty.
                                  Don't make assumptions about the contents! */
} lame_version_t;
void CDECL get_lame_version_numerical(lame_version_t *);


/*
 * OPTIONAL:
 * print the LAME configuration through the message function.
 * Call them after lame_init_params(). Before it, they print nothing.
 */
void CDECL lame_print_config(const lame_global_flags*  gfp);

void CDECL lame_print_internals( const lame_global_flags *gfp);


/**
 * \ingroup api_encoding
 * Encodes PCM samples and writes all complete MP3 frames to @p mp3buf.
 * This routine handles all buffering, resampling and filtering for you.
 *
 * The @p mp3buf_size needed depends on the settings. A worst case for the
 * standard bitrates, with the samples counted at the output sample rate:
 *
 *     mp3buf_size in bytes = 1.25 * nsamples * max(1, out_rate / in_rate) + 7200
 *
 * The first call after lame_init_params() also returns the ID3v2 tag, when one
 * is written automatically: add its size (lame_get_id3v2_tag()) for that call.
 * The formula does not apply to free format, because free format allows
 * higher bitrates. For any settings, including free format,
 * lame_get_maximum_number_of_samples() returns how many samples fit into a
 * buffer of a given size.
 *
 * @note If the input has 2 channels and the mode is \c MONO, the encoder
 *       averages the two channels and encodes the result. The data in
 *       @p buffer_l and @p buffer_r is not changed.
 *
 * @param gfp          the encoder instance.
 * @param buffer_l     PCM data for the left channel.
 * @param buffer_r     PCM data for the right channel.
 * @param nsamples     number of samples per channel.
 * @param mp3buf       receives the encoded MP3 stream.
 * @param mp3buf_size  size of @p mp3buf in bytes. If it is 0, LAME does not
 *                     check whether @p mp3buf is large enough.
 * @return The number of bytes written to @p mp3buf. This is 0 while the
 *         encoder is still collecting samples for a full frame. A negative
 *         value means failure:
 *         @li -1 @p mp3buf was too small, or NULL while there were bytes to
 *             write.
 *         @li -2 malloc() problem.
 *         @li -3 lame_init_params() not called.
 *         @li -4 a problem in the psychoacoustic model.
 *         @li -6 the ReplayGain analysis of the resampled input failed.
 *         @li -13 the encoder could not fit a frame into its bits, or the
 *             bits it wrote disagree with its bit reservoir. This call and
 *             every later encode or flush call fail.
 *         @li -14 @p nsamples is negative, or a sample is louder than 4096
 *             times full scale after lame_set_scale() and the per-channel
 *             scales. Nothing of the call is encoded.
 */
int CDECL lame_encode_buffer (
        lame_global_flags*  gfp,           /* encoder instance              */
        const short int     buffer_l [],   /* PCM data for left channel     */
        const short int     buffer_r [],   /* PCM data for right channel    */
        const int           nsamples,      /* number of samples per channel */
        unsigned char*      mp3buf,        /* pointer to encoded MP3 stream */
        const int           mp3buf_size ); /* number of valid octets in this
                                              stream                        */

/*
 * as above, but input has L & R channel data interleaved.
 * NOTE:
 * num_samples = number of samples in the L (or R)
 * channel, not the total number of samples in pcm[]
 */
int CDECL lame_encode_buffer_interleaved(
        lame_global_flags*  gfp,           /* encoder instance              */
        short int           pcm[],         /* PCM data for left and right
                                              channel, interleaved          */
        int                 num_samples,   /* number of samples per channel,
                                              _not_ number of samples in
                                              pcm[]                         */
        unsigned char*      mp3buf,        /* pointer to encoded MP3 stream */
        int                 mp3buf_size ); /* number of valid octets in this
                                              stream                        */


/**
 * \ingroup api_encoding
 * As lame_encode_buffer(), but the samples are of type float.
 *
 * \note Full scale is +/- 32768, the same range as short int samples.
 *
 * @return As lame_encode_buffer(), and additionally -14 when a
 *         sample is not a finite number, as for lame_encode_buffer_ieee_float().
 */
int CDECL lame_encode_buffer_float(
        lame_global_flags*  gfp,           /* encoder instance              */
        const float         pcm_l [],      /* PCM data for left channel     */
        const float         pcm_r [],      /* PCM data for right channel    */
        const int           nsamples,      /* number of samples per channel */
        unsigned char*      mp3buf,        /* pointer to encoded MP3 stream */
        const int           mp3buf_size ); /* number of valid octets in this
                                              stream                        */

/**
 * \ingroup api_encoding
 * As lame_encode_buffer(), but the samples are of type float.
 *
 * \note Full scale is +/- 1.0.
 *
 * Every sample must be a finite number. If a sample is NaN or infinite, the
 * call returns -14 and encodes nothing.
 *
 * @return As lame_encode_buffer(), and additionally -14 when a
 *         sample is not a finite number. Nothing is encoded in that case.
 */
int CDECL lame_encode_buffer_ieee_float(
        lame_t          gfp,
        const float     pcm_l [],          /* PCM data for left channel     */
        const float     pcm_r [],          /* PCM data for right channel    */
        const int       nsamples,
        unsigned char * mp3buf,
        const int       mp3buf_size);
/**
 * \ingroup api_encoding
 * As lame_encode_buffer_ieee_float(), but for interleaved data.
 *
 * \note Full scale is +/- 1.0.
 *
 * @return As lame_encode_buffer(), and additionally -14 when a
 *         sample is not a finite number, as for lame_encode_buffer_ieee_float().
 */
int CDECL lame_encode_buffer_interleaved_ieee_float(
        lame_t          gfp,
        const float     pcm[],             /* PCM data for left and right
                                              channel, interleaved          */
        const int       nsamples,
        unsigned char * mp3buf,
        const int       mp3buf_size);

/**
 * \ingroup api_encoding
 * As lame_encode_buffer(), but for 'double's.
 *
 * \note Full scale is +/- 1.0.
 *
 * @return As lame_encode_buffer(), and additionally -14 when a
 *         sample is not a finite number, as for lame_encode_buffer_ieee_float().
 */
int CDECL lame_encode_buffer_ieee_double(
        lame_t          gfp,
        const double    pcm_l [],          /* PCM data for left channel     */
        const double    pcm_r [],          /* PCM data for right channel    */
        const int       nsamples,
        unsigned char * mp3buf,
        const int       mp3buf_size);
/**
 * \ingroup api_encoding
 * As lame_encode_buffer_ieee_double(), but for interleaved data.
 *
 * \note Full scale is +/- 1.0.
 *
 * @return As lame_encode_buffer(), and additionally -14 when a
 *         sample is not a finite number, as for lame_encode_buffer_ieee_float().
 */
int CDECL lame_encode_buffer_interleaved_ieee_double(
        lame_t          gfp,
        const double    pcm[],             /* PCM data for left and right
                                              channel, interleaved          */
        const int       nsamples,
        unsigned char * mp3buf,
        const int       mp3buf_size);

/* as lame_encode_buffer, but the samples are of type long.
 * NOTE: full scale is +/- 32768, the same range as short int samples.
 *
 * This scaling does not use the full precision of type long.
 * Use lame_encode_buffer_long2() instead.
 */
int CDECL lame_encode_buffer_long(
        lame_global_flags*  gfp,           /* encoder instance              */
        const long     buffer_l [],       /* PCM data for left channel     */
        const long     buffer_r [],       /* PCM data for right channel    */
        const int           nsamples,      /* number of samples per channel */
        unsigned char*      mp3buf,        /* pointer to encoded MP3 stream */
        const int           mp3buf_size ); /* number of valid octets in this
                                              stream                        */

/* Same as lame_encode_buffer_long(), but full scale is the whole range of
 * type long:  +/- 2^(8*sizeof(long)-1)
 */
int CDECL lame_encode_buffer_long2(
        lame_global_flags*  gfp,           /* encoder instance              */
        const long     buffer_l [],       /* PCM data for left channel     */
        const long     buffer_r [],       /* PCM data for right channel    */
        const int           nsamples,      /* number of samples per channel */
        unsigned char*      mp3buf,        /* pointer to encoded MP3 stream */
        const int           mp3buf_size ); /* number of valid octets in this
                                              stream                        */

/* as lame_encode_buffer, but the samples are of type int.
 * NOTE: full scale is the whole range of int, +/- 2147483648 for a
 * 4-byte int.  The other lame_encode_buffer() functions use a different
 * scale, which would lose precision here.
 */
int CDECL lame_encode_buffer_int(
        lame_global_flags*  gfp,           /* encoder instance              */
        const int      buffer_l [],       /* PCM data for left channel     */
        const int      buffer_r [],       /* PCM data for right channel    */
        const int           nsamples,      /* number of samples per channel */
        unsigned char*      mp3buf,        /* pointer to encoded MP3 stream */
        const int           mp3buf_size ); /* number of valid octets in this
                                              stream                        */

/*
 * as above, but for interleaved data.
 * NOTE: full scale is +/- 2^(8*sizeof(int32_t)-1).
 * NOTE:
 * num_samples = number of samples in the L (or R)
 * channel, not the total number of samples in pcm[]
 */
int CDECL lame_encode_buffer_interleaved_int(
        lame_t          gfp,
        const int       pcm [],            /* PCM data for left and right
                                              channel, interleaved          */
        const int       nsamples,          /* number of samples per channel,
                                              _not_ number of samples in
                                              pcm[]                         */
        unsigned char*  mp3buf,            /* pointer to encoded MP3 stream */
        const int       mp3buf_size );     /* number of valid octets in this
                                              stream                        */



/*
 * REQUIRED:
 * lame_encode_flush encodes the PCM samples still in the internal buffers,
 * pads the last frame with zeros, and returns the last MP3 frames.
 * 'mp3buf' should be at least 7200 bytes long
 * to hold all possible emitted data at the standard bitrates. For free
 * format it should hold 8 frames at the chosen bitrate plus 2048 bytes,
 * and 128 more when an ID3v1 tag is written; a frame is
 * 144000 * kbps / samplerate + 1 bytes for MPEG-1 and
 * 72000 * kbps / samplerate + 1 bytes for MPEG-2 and 2.5, samplerate
 * being the output sample rate.
 *
 * It also writes the ID3v1 tag into the stream, if there is one.
 *
 * return code = number of bytes written to mp3buf. Can be 0.  -1 when
 * mp3buf is too small for them, or NULL.  -2 malloc() problem.  -3 if
 * lame_init_params() was not called.  -6 the ReplayGain analysis failed.
 * -13 as for lame_encode_buffer().
 */
int CDECL lame_encode_flush(
        lame_global_flags *  gfp,    /* encoder instance                      */
        unsigned char*       mp3buf, /* pointer to encoded MP3 stream         */
        int                  size);  /* number of valid octets in this stream */

/*
 * OPTIONAL:
 * lame_encode_flush_nogap returns the MP3 data still in the internal
 * buffers.  It pads the last frame with ancillary data, so that it is a
 * complete MP3 frame.
 *
 * 'mp3buf' should be at least 7200 bytes long
 * to hold all possible emitted data at the standard bitrates. For free
 * format it should hold 8 frames at the chosen bitrate plus 2048 bytes,
 * counted as for lame_encode_flush().
 *
 * After this call, the MP3 data written so far is complete.  You can
 * continue to encode new PCM samples and write the MP3 data to a different
 * file.  The two MP3 files play back without a gap when they are joined.
 *
 * This function does NOT write an ID3v1 tag into the stream.
 *
 * return code = number of bytes written to mp3buf. Can be 0.  -1 when
 * mp3buf is too small for them, or NULL.  -3 if lame_init_params() was not
 * called.  -6 the ReplayGain analysis failed.
 */
int CDECL lame_encode_flush_nogap(
        lame_global_flags *  gfp,    /* encoder instance                      */
        unsigned char*       mp3buf, /* pointer to encoded MP3 stream         */
        int                  size);  /* number of valid octets in this stream */

/*
 * OPTIONAL:
 * Normally, lame_init_params() calls this.  It writes the ID3v2 tag and the
 * LAME tag frame at the start of the stream, and sets the frame counters and
 * the bitrate histogram to 0.  You can also call this after
 * lame_encode_flush_nogap().  Call it only after lame_init_params().
 */
int CDECL lame_init_bitstream(
        lame_global_flags *  gfp);    /* encoder instance                      */



/*
 * OPTIONAL:    some simple statistics
 * a bitrate histogram to visualize the distribution of used frame sizes
 * a stereo mode histogram to visualize the distribution of used stereo
 *   modes, useful in joint-stereo mode only
 *   0: LR    left-right encoded
 *   1: LR-I  left-right and intensity encoded (not supported)
 *   2: MS    mid-side encoded
 *   3: MS-I  mid-side and intensity encoded (not supported)
 *
 * attention: call them before lame_close()
 * suggested: lame_encode_flush -> lame_*_hist -> lame_close
 * Before lame_init_params(), they do not write the arrays.
 */

void CDECL lame_bitrate_hist(
        const lame_global_flags * gfp,
        int bitrate_count[14] );
void CDECL lame_bitrate_kbps(
        const lame_global_flags * gfp,
        int bitrate_kbps [14] );
void CDECL lame_stereo_mode_hist(
        const lame_global_flags * gfp,
        int stereo_mode_count[4] );

void CDECL lame_bitrate_stereo_mode_hist (
        const lame_global_flags * gfp,
        int bitrate_stmode_count[14][4] );

void CDECL lame_block_type_hist (
        const lame_global_flags * gfp,
        int btype_count[6] );

void CDECL lame_bitrate_block_type_hist (
        const lame_global_flags * gfp,
        int bitrate_btype_count[14][6] );

#if (DEPRECATED_OR_OBSOLETE_CODE_REMOVED && 0)
#else
/*
 * OPTIONAL:
 * lame_mp3_tags_fid writes the final LAME tag frame into the MP3 file fid.
 * It seeks forwards and backwards, so fid must be a real file.  Call it
 * after lame_encode_flush(), when all MP3 data is written to the file.
 * NOTE:
 * if the LAME tag is turned off by the user or by LAME, this call does
 * nothing.
 * NOTE:
 * LAME reads the file to skip an ID3v2 tag, so open the file for reading
 * and writing.
 * NOTE:
 * To write the LAME tag yourself, call lame_get_lametag_frame instead.
*/
void CDECL lame_mp3_tags_fid(lame_global_flags *, FILE* fid);
#endif

/*
 * OPTIONAL:
 * lame_get_lametag_frame copies the final LAME tag frame into 'buffer'.
 * It returns the number of bytes copied.  If 'buffer' is too small, it
 * copies nothing and returns the required size.  So a return value larger
 * than 'size' means failure.  Call lame_encode_flush() first.
 * NOTE:
 * if the LAME tag is turned off by the user or by LAME, this call does
 * nothing and returns 0.
 * NOTE:
 * At the start of the MP3 data, LAME writes an empty frame.  After encoding,
 * overwrite that frame with this buffer.  Without an ID3v2 tag, this frame
 * is usually at the start of the file.  If you write other data before the
 * MP3 data, you must track where the frame is yourself.
 */
size_t CDECL lame_get_lametag_frame(
        const lame_global_flags *, unsigned char* buffer, size_t size);

/*
 * REQUIRED:
 * frees the encoder instance and all its buffers.  Call it last.
 */
int  CDECL lame_close (lame_global_flags *);

#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
#else
/*
 * OBSOLETE:
 * lame_encode_finish combines lame_encode_flush() and lame_close() in
 * one call.  However, once this call is made, the statistics routines
 * will no longer work because the data will have been cleared, and
 * lame_mp3_tags_fid() cannot be called to add data to the VBR header
 */
int CDECL lame_encode_finish(
        lame_global_flags*  gfp,
        unsigned char*      mp3buf,
        int                 size );
#endif






/*********************************************************************
 *
 * decoding
 *
 * a simple interface to the mpg123 decoder is also included when
 * libmp3lame is built with mpg123 support
 *
 *********************************************************************/

struct hip_global_struct;
typedef struct hip_global_struct hip_global_flags;
typedef hip_global_flags *hip_t;


typedef struct {
  int header_parsed;   /* 1 if header was parsed and following data was
                          computed                                       */
  int stereo;          /* number of channels                             */
  int samplerate;      /* sample rate                                    */
  int bitrate;         /* bitrate                                        */
  int mode;            /* channel mode from the frame header             */
  int mode_ext;        /* mode extension from the frame header           */
  int framesize;       /* number of samples per mp3 frame                */

  /* the decoder does not fill in the three fields below */
  unsigned long nsamp; /* number of samples in mp3 file.                 */
  int totalframes;     /* total number of frames in mp3 file             */
  int framenum;        /* frames decoded counter                         */
} mp3data_struct;

/* required call to initialize decoder */
hip_t CDECL hip_decode_init(void);
/* like hip_decode_init, but the decoder removes the encoder delay and
   padding.  Returns NULL without libmpg123. */
hip_t CDECL hip_decode_init_gapless(void);

/* cleanup call to exit decoder  */
int CDECL hip_decode_exit(hip_t gfp);

/* HIP reporting functions */
void CDECL hip_set_errorf(hip_t gfp, lame_report_function f);
void CDECL hip_set_debugf(hip_t gfp, lame_report_function f);
void CDECL hip_set_msgf  (hip_t gfp, lame_report_function f);

/* Analysis hooks, for a frontend that plots what the decoder saw.

   plotting_data is declared but not defined here.  Its layout is internal
   and can change, so pass it only by pointer.  The fields are in an internal
   header that is not part of the API.

   hip_set_pinfo sets the block that the decoder fills in.  Call it before
   decoding starts.  hip_finish_pinfo completes the data of the last frame at
   the end of the input.  It does nothing if no block was set.

   Both functions accept NULL as the decoder.  Both do nothing if the library
   was built without the mpg123 decoder. */
#ifndef plotting_data_defined
#define plotting_data_defined
struct plotting_data;
typedef struct plotting_data plotting_data;
#endif

void CDECL hip_set_pinfo(hip_t gfp, plotting_data* pinfo);
void CDECL hip_finish_pinfo(hip_t gfp);

/*********************************************************************
 * decodes MP3 data and returns the PCM samples.
 *
 *  nout = hip_decode(hip, mp3buf,len,pcm_l,pcm_r);
 *
 * input:
 *    len          :  number of bytes of mp3 data in mp3buf
 *    mp3buf[len]  :  mp3 data to be decoded
 *
 * output:
 *    nout:  -1    : decoding error
 *            0    : more data is needed to complete a frame
 *           >0    : number of samples per channel written to pcm_l, pcm_r
 *    pcm_l[nout]  : left channel data
 *    pcm_r[nout]  : right channel data
 *
 * pcm_l and pcm_r need room for every frame that the call completes,
 * also frames from the input of earlier calls.  That number does not
 * follow from len (free-format frames can be a few bytes long), and the
 * function cannot check the size of the arrays.  With arrays of a fixed
 * size, call hip_decode1() in a loop instead: it writes at most one
 * frame, 1152 samples per channel.
 *
 * An application that decodes MP3 for its own use should use libmpg123.
 * Its API takes the size of the output.
 *
 *********************************************************************/
int CDECL hip_decode( hip_t           gfp
                    , unsigned char * mp3buf
                    , size_t          len
                    , short           pcm_l[]
                    , short           pcm_r[]
                    );

/* same as hip_decode, and also returns mp3 header data */
int CDECL hip_decode_headers( hip_t           gfp
                            , unsigned char*  mp3buf
                            , size_t          len
                            , short           pcm_l[]
                            , short           pcm_r[]
                            , mp3data_struct* mp3data
                            );

/* same as hip_decode, but returns at most one frame */
int CDECL hip_decode1( hip_t          gfp
                     , unsigned char* mp3buf
                     , size_t         len
                     , short          pcm_l[]
                     , short          pcm_r[]
                     );

/* same as hip_decode1, but returns at most one frame and mp3 header data */
int CDECL hip_decode1_headers( hip_t           gfp
                             , unsigned char*  mp3buf
                             , size_t          len
                             , short           pcm_l[]
                             , short           pcm_r[]
                             , mp3data_struct* mp3data
                             );

/* same as hip_decode1_headers, but also returns enc_delay and enc_padding
   from the LAME tag (-1 if there is none) */
int CDECL hip_decode1_headersB( hip_t gfp
                              , unsigned char*   mp3buf
                              , size_t           len
                              , short            pcm_l[]
                              , short            pcm_r[]
                              , mp3data_struct*  mp3data
                              , int             *enc_delay
                              , int             *enc_padding
                              );



/* OBSOLETE:
 * the lame_decode... functions keep old code working.
 * Use the hip_decode... functions above instead.
 */
#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
#else
int CDECL lame_decode_init(void);
int CDECL lame_decode(
        unsigned char *  mp3buf,
        int              len,
        short            pcm_l[],
        short            pcm_r[] );
int CDECL lame_decode_headers(
        unsigned char*   mp3buf,
        int              len,
        short            pcm_l[],
        short            pcm_r[],
        mp3data_struct*  mp3data );
int CDECL lame_decode1(
        unsigned char*  mp3buf,
        int             len,
        short           pcm_l[],
        short           pcm_r[] );
int CDECL lame_decode1_headers(
        unsigned char*   mp3buf,
        int              len,
        short            pcm_l[],
        short            pcm_r[],
        mp3data_struct*  mp3data );
int CDECL lame_decode1_headersB(
        unsigned char*   mp3buf,
        int              len,
        short            pcm_l[],
        short            pcm_r[],
        mp3data_struct*  mp3data,
        int              *enc_delay,
        int              *enc_padding );
int CDECL lame_decode_exit(void);

#endif /* obsolete lame_decode API calls */


/*********************************************************************
 *
 * ID3 tags
 *
 *********************************************************************/

/*
 * id3tag.h -- Interface to write ID3 version 1 and 2 tags.
 *
 * Copyright (C) 2000 Don Melton.
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

/* calls handler for each genre name and number, in alphabetical order */
void CDECL id3tag_genre_list(
        void (*handler)(int, const char *, void *),
        void*  cookie);

/* Set the tag fields before lame_init_params(). A program that writes the
 * tags itself (lame_set_write_id3tag_automatic(gfp, 0)) can also change them
 * after lame_init_params(): lame_get_id3v2_tag() and lame_get_id3v1_tag()
 * return the tag with the fields as they are at that call. */
void CDECL id3tag_init     (lame_t gfp);

/* always add a version 2 tag */
void CDECL id3tag_add_v2   (lame_t gfp);

/* always add a version 2.4 tag with UTF-8 encoding */
void CDECL id3tag_add_v2_4_UTF8 (lame_t gfp);

/* add only a version 2.4 tag with UTF-8 encoding */
void CDECL id3tag_v2_4_UTF8_only (lame_t gfp);

/* add only a version 1 tag */
void CDECL id3tag_v1_only  (lame_t gfp);

/* add only a version 2 tag */
void CDECL id3tag_v2_only  (lame_t gfp);

/* pad version 1 tag with spaces instead of nulls */
void CDECL id3tag_space_v1 (lame_t gfp);

/* pad version 2 tag with extra 128 bytes */
void CDECL id3tag_pad_v2   (lame_t gfp);

/* largest padding id3tag_set_pad() writes, in bytes */
#define LAME_MAXID3V2PADDING 128000

/* pad version 2 tag with extra n bytes; a larger n than
   LAME_MAXID3V2PADDING is reduced to it */
void CDECL id3tag_set_pad  (lame_t gfp, size_t n);

void CDECL id3tag_set_title(lame_t gfp, const char* title);
void CDECL id3tag_set_artist(lame_t gfp, const char* artist);
void CDECL id3tag_set_album(lame_t gfp, const char* album);
void CDECL id3tag_set_year(lame_t gfp, const char* year);
void CDECL id3tag_set_comment(lame_t gfp, const char* comment);
            
/* returns -1 if the track number is out of the ID3v1 range.  The ID3v1
   tag then has no track number */
int CDECL id3tag_set_track(lame_t gfp, const char* track);

/* returns -1 if the genre number is out of range, 0 otherwise.
  A name that is not an ID3v1 genre name is not an error.  The ID3v1 tag
  then gets 'Other', and the ID3v2 tag gets the name as given */
int CDECL id3tag_set_genre(lame_t gfp, const char* genre);

/* returns non-zero if the field name is not valid */
int CDECL id3tag_set_fieldvalue(lame_t gfp, const char* fieldvalue);

/* returns non-zero if the image type is not valid */
int CDECL id3tag_set_albumart(lame_t gfp, const char* image, size_t size);

/* lame_get_id3v1_tag copies the ID3v1 tag into buffer.
 * It returns the number of bytes copied, or the number of bytes required
 * if 'size' is too small.  A return value larger than 'size' means failure.
 * NOTE:
 * This function does nothing if the user or LAME turned the ID3v1 tag off.
 */
size_t CDECL lame_get_id3v1_tag(lame_t gfp, unsigned char* buffer, size_t size);

/* lame_get_id3v2_tag copies the ID3v2 tag into buffer.
 * It returns the number of bytes copied, or the number of bytes required
 * if 'size' is too small.  A return value larger than 'size' means failure.
 * NOTE:
 * This function does nothing if the user or LAME turned the ID3v2 tag off.
 * Call it after lame_init_params().
 */
size_t CDECL lame_get_id3v2_tag(lame_t gfp, unsigned char* buffer, size_t size);

/* By default, lame_init_params() writes the ID3v2 tag into the MP3 stream.
 * To write the tag yourself, call lame_set_write_id3tag_automatic(gfp, 0)
 * before lame_init_params().  After lame_init_params(), get the tag with
 * lame_get_id3v2_tag() and write it to your file.
 */
void CDECL lame_set_write_id3tag_automatic(lame_global_flags * gfp, int);
int CDECL lame_get_write_id3tag_automatic(lame_global_flags const* gfp);

/* experimental */
int CDECL id3tag_set_textinfo_latin1(lame_t gfp, char const *id, char const *text);

/* experimental */
int CDECL id3tag_set_comment_latin1(lame_t gfp, char const *lang, char const *desc, char const *text);

#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
#else
/* experimental */
int CDECL id3tag_set_textinfo_ucs2(lame_t gfp, char const *id, unsigned short const *text);

/* experimental */
int CDECL id3tag_set_comment_ucs2(lame_t gfp, char const *lang,
                                  unsigned short const *desc, unsigned short const *text);

/* experimental */
int CDECL id3tag_set_fieldvalue_ucs2(lame_t gfp, const unsigned short *fieldvalue);
#endif

/* experimental */
int CDECL id3tag_set_fieldvalue_utf16(lame_t gfp, const unsigned short *fieldvalue);

/* experimental */
int CDECL id3tag_set_fieldvalue_utf8(lame_t gfp, const char *fieldvalue);

/* experimental */
int CDECL id3tag_set_textinfo_utf16(lame_t gfp, char const *id, unsigned short const *text);

/* experimental */
int CDECL id3tag_set_comment_utf16(lame_t gfp, char const *lang, unsigned short const *desc, unsigned short const *text);

/* experimental */
int CDECL id3tag_set_textinfo_utf8(lame_t gfp, char const *id, char const *text);

/* experimental */
int CDECL id3tag_set_comment_utf8(lame_t gfp, char const *lang, char const *desc, char const *text);


/***********************************************************************
*
*  list of valid bitrates [kbps] & sample frequencies [Hz].
*  first index: 0: MPEG-2   values  (sample frequencies 16...24 kHz)
*               1: MPEG-1   values  (sample frequencies 32...48 kHz)
*               2: MPEG-2.5 values  (sample frequencies  8...12 kHz)
***********************************************************************/

extern const int     bitrate_table    [3][16];
extern const int     samplerate_table [3][ 4];

/* access functions for the tables, because a DLL does not export global
   variables */
int CDECL lame_get_bitrate(int mpeg_version, int table_index);
int CDECL lame_get_samplerate(int mpeg_version, int table_index);


/* maximum size of albumart image (128KB), which affects LAME_MAXMP3BUFFER
   as well since lame_encode_buffer() also returns ID3v2 tag data */
#define LAME_MAXALBUMART    (128 * 1024)

/* maximum size of mp3buffer needed if you encode at most 1152 samples for
   each call to lame_encode_buffer.  see lame_encode_buffer()
   (LAME_MAXMP3BUFFER is obsolete)  */
#define LAME_MAXMP3BUFFER   (16384 + LAME_MAXALBUMART)


/**
 *  \ingroup api
 *  Status values returned by the encoding and decoding calls. A negative
 *  value is an error. The `FRONTEND_` codes come from the command-line
 *  tools, not from the library.
 */
typedef enum {
    LAME_OKAY             =   0,  /**< the call succeeded */
    LAME_NOERROR          =   0,  /**< another name for \c LAME_OKAY */
    LAME_GENERICERROR     =  -1,  /**< the call failed, with no more specific
                                       code to report */
    LAME_NOMEM            = -10,  /**< an allocation failed */
    LAME_BADBITRATE       = -11,  /**< the requested bitrate is not supported */
    LAME_BADSAMPFREQ      = -12,  /**< the requested sample rate is not
                                       supported */
    LAME_INTERNALERROR    = -13,  /**< an internal error occurred in the
                                       library */
    /** The input data cannot be encoded, for example a PCM sample that is not
        a finite number. */
    LAME_BADINPUTDATA     = -14,

    FRONTEND_READERROR    = -80,  /**< the input could not be read */
    FRONTEND_WRITEERROR   = -81,  /**< the output could not be written */
    FRONTEND_FILETOOLARGE = -82   /**< the input is too large to handle */

} lame_errorcodes_t;

#if defined(__cplusplus)
}
#endif
#endif /* LAME_LAME_H */

