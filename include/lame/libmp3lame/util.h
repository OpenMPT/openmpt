/*
 *      lame utility library include file
 *
 *      Copyright (c) 1999 Albert L Faber
 *      Copyright (c) 2008 Robert Hegemann
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

#ifndef LAME_UTIL_H
#define LAME_UTIL_H

#include "l3side.h"
#include "id3tag.h"
#include "lame_global_flags.h"

#ifdef __cplusplus
extern  "C" {
#endif

/***********************************************************************
*
*  Global Definitions
*
***********************************************************************/

#ifndef FALSE
#define         FALSE                   0
#endif

#ifndef TRUE
#define         TRUE                    (!FALSE)
#endif

#ifdef UINT_MAX
# define         MAX_U_32_NUM            UINT_MAX
#else
# define         MAX_U_32_NUM            0xFFFFFFFF
#endif

#ifndef PI
# ifdef M_PI
#  define       PI                      M_PI
# else
#  define       PI                      3.14159265358979323846
# endif
#endif


#ifdef M_LN2
# define        LOG2                    M_LN2
#else
# define        LOG2                    0.69314718055994530942
#endif

#ifdef M_LN10
# define        LOG10                   M_LN10
#else
# define        LOG10                   2.30258509299404568402
#endif


#ifdef M_SQRT2
# define        SQRT2                   M_SQRT2
#else
# define        SQRT2                   1.41421356237309504880
#endif


#define         CRC16_POLYNOMIAL        0x8005

#define MAX_BITS_PER_CHANNEL 4095
#define MAX_BITS_PER_GRANULE 7680

/* the highest free format bitrate the encoder uses, in kbit/s */
#define MAX_FREE_FORMAT_KBPS 640

/* the loudest input the encoder takes, in multiples of full scale, and the
   largest scale factor the scale setters accept: 2^12 (+72 dB) */
#define MAX_INPUT_SCALE 4096.0f

/* "bit_stream.h" Definitions */
#define         BUFFER_SIZE     LAME_MAXMP3BUFFER

#define         Min(A, B)       ((A) < (B) ? (A) : (B))
#define         Max(A, B)       ((A) > (B) ? (A) : (B))

/* log/log10 approximations */
#ifdef USE_FAST_LOG
#define         FAST_LOG10(x)       (fast_log2(x)*(LOG2/LOG10))
#define         FAST_LOG10_X(x,y)   (fast_log2(x)*(LOG2/LOG10*(y)))
#else
#define         FAST_LOG10(x)       log10(x)
#define         FAST_LOG10_X(x,y)   (log10(x)*(y))
#endif


    struct replaygain_data;
#ifndef replaygain_data_defined
#define replaygain_data_defined
    typedef struct replaygain_data replaygain_t;
#endif
    struct plotting_data;
#ifndef plotting_data_defined
#define plotting_data_defined
    typedef struct plotting_data plotting_data;
#endif

/***********************************************************************
*
*  Global Type Definitions
*
***********************************************************************/

    /* "bit_stream.h" Type Definitions */

    typedef struct bit_stream_struc {
        unsigned char *buf;  /* bit stream buffer */
        int     buf_size;    /* size of buffer (in number of bytes) */
        int     totbit;      /* bit counter of bit stream */
        int     buf_byte_idx; /* pointer to top byte in buffer */
        int     buf_bit_idx; /* pointer to top bit of top byte in buffer */

        /* format of file in rd mode (BINARY/ASCII) */
    } Bit_stream_struc;



    typedef struct {
        int    *bag;         /* pointer to our bag */
        uint64_t nBytesWritten;      /* the bytes of the stream so far, 64 bits everywhere */
        int     sum;         /* what we have seen so far */
        int     seen;        /* how many frames we have seen in this chunk */
        int     want;        /* how many frames we want to collect into one chunk */
        int     pos;         /* actual position in our bag */
        int     size;        /* size of our bag */
        unsigned int nVbrNumFrames;
        /* VBR tag data */
        unsigned int TotalFrameSize;
    } VBR_seek_info_t;


    /**
     *  \internal
     *  \brief Settings and tables for the absolute threshold of hearing (ATH).
     *         New ATH data belongs in this struct.
     */
    typedef struct {
        int     use_adjust;  /* method for the auto adjustment  */
        FLOAT   aa_sensitivity_p; /* factor for tuning the (sample power)
                                     point below which adaptive threshold
                                     of hearing adjustment occurs */
        FLOAT   adjust_factor; /* lowering based on peak volume, 1 = no lowering */
        FLOAT   adjust_limit; /* limit for dynamic ATH adjust */
        FLOAT   decay;       /* determined to lower x dB each second */
        FLOAT   floor;       /* lowest ATH value */
        FLOAT   l[SBMAX_l];  /* ATH for sfbs in long blocks */
        FLOAT   s[SBMAX_s];  /* ATH for sfbs in short blocks */
        FLOAT   psfb21[PSFB21]; /* ATH for partitionned sfb21 in long blocks */
        FLOAT   psfb12[PSFB12]; /* ATH for partitionned sfb12 in short blocks */
        FLOAT   cb_l[CBANDS]; /* ATH for long block convolution bands */
        FLOAT   cb_s[CBANDS]; /* ATH for short block convolution bands */
        FLOAT   eql_w[BLKSIZE / 2]; /* equal loudness weights (based on ATH) */
    } ATH_t;

    /**
     *  \internal
     *  \brief Constant tables of the psychoacoustic model for one block type.
     */

    typedef struct {
        FLOAT   masking_lower[CBANDS];
        FLOAT   minval[CBANDS];
        FLOAT   rnumlines[CBANDS];
        FLOAT   mld_cb[CBANDS];
        FLOAT   mld[Max(SBMAX_l,SBMAX_s)];
        FLOAT   bo_weight[Max(SBMAX_l,SBMAX_s)]; /* band weight long scalefactor bands, at transition */
        FLOAT   attack_threshold; /* short block tuning */
        int     s3ind[CBANDS][2];
        int     numlines[CBANDS];
        int     bm[Max(SBMAX_l,SBMAX_s)];
        int     bo[Max(SBMAX_l,SBMAX_s)];
        int     npart;
        int     n_sb; /* SBMAX_l or SBMAX_s */
        FLOAT  *s3;
    } PsyConst_CB2SB_t;


    /**
     *  \internal
     *  \brief Constant tables of the psychoacoustic model, for long and short
     *         blocks.
     */
    typedef struct {
        FLOAT window[BLKSIZE], window_s[BLKSIZE_s / 2];
        PsyConst_CB2SB_t l;
        PsyConst_CB2SB_t s;
        PsyConst_CB2SB_t l_to_s;
        FLOAT   attack_threshold[4];
        FLOAT   decay;
        int     force_short_block_calc;
    } PsyConst_t;


    typedef struct {

        FLOAT   nb_l1[4][CBANDS], nb_l2[4][CBANDS];
        FLOAT   nb_s1[4][CBANDS], nb_s2[4][CBANDS];

        III_psy_xmin thm[4];
        III_psy_xmin en[4];

        /* loudness calculation (for adaptive threshold of hearing) */
        FLOAT   loudness_sq_save[2]; /* account for granule delay of L3psycho_anal */

        FLOAT   tot_ener[4];

        FLOAT   last_en_subshort[4][9];
        int     last_attacks[4];

        int     blocktype_old[2];
    } PsyStateVar_t;


    typedef struct {
        /* loudness calculation (for adaptive threshold of hearing) */
        FLOAT   loudness_sq[2][2]; /* loudness^2 approx. per granule and channel */
    } PsyResult_t;


    /* variables used by encoder.c */
    typedef struct {
        /* variables for newmdct.c */
        FLOAT   sb_sample[2][2][18][SBLIMIT];
        FLOAT   amp_filter[32];

        /* variables used by util.c */
        /* BPC = maximum number of filter convolution windows to precompute */
#define BPC 320
        /* the resampler's FIR filter length: odd, one longer for an integer ratio */
#define RESAMPLE_FILTER_LENGTH 31
        /* the most input the resampler keeps back from one call for the next */
#define RESAMPLE_HELD_INPUT (RESAMPLE_FILTER_LENGTH + 2)
        /* the largest output/input rate ratio lame_init_params() accepts */
#define MAX_UPSAMPLING_RATIO 128
        double  itime[2]; /* float precision seems to be not enough */
        sample_t *inbuf_old[2];
        sample_t *blackfilt[2 * BPC + 1];

        FLOAT   pefirbuf[19];
        
        /* used for padding */
        int     frac_SpF;
        int     slot_lag;

        /* variables for bitstream.c */
        /* A frame's header is produced when the frame is encoded and written
         * to the stream when the stream reaches its position, so the number of
         * headers waiting here is the reservoir divided by what a frame has
         * room for beyond its own side info.
         *
         * mpeg1: buffer=511 bytes  smallest frame: 96-38(sideinfo)=58
         * max number of frames in reservoir:  8
         * mpeg2: buffer=255 bytes.  smallest frame: 24-23bytes=1
         * Encoding silence at 8kbs/24khz therefore leaves 255 headers waiting,
         * and the frame being encoded needs a slot of its own beside them:
         * encodeSideInfo2 takes the next one before this frame's data has
         * written any of the waiting headers out. So the demand is 256, and a
         * ring of exactly 256 overwrites the oldest waiting header instead of
         * holding it - which loses every header from that one on until the
         * stream catches up, some 255 frames later.
         *
         * also, max_header_buf has to be a power of two */
#define MAX_HEADER_BUF 512
#define MAX_HEADER_LEN 40    /* max size of header is 38 */
        struct {
            int     write_timing;
            int     ptr;
            unsigned char buf[MAX_HEADER_LEN];
        } header[MAX_HEADER_BUF];

        int     h_ptr;
        int     w_ptr;
        int     ancillary_flag;

        /* variables for reservoir.c */
        int     ResvSize;    /* in bits */
        int     ResvMax;     /* in bits */

        sample_t *in_buffer_0;
        sample_t *in_buffer_1;
        int     in_buffer_nsamples;

#ifndef  MFSIZE
# define MFSIZE  ( 3*1152 + ENCDELAY - MDCTDELAY )
#endif
/* The frames of 1152 samples, the longest frame, that mfbuf holds beyond
   MFSIZE: the samples in use move down to its start only when the next frame
   would not fit behind them. */
#define MFBUF_SLIDE_FRAMES 4
#define MFBUF_SIZE  ( MFSIZE + MFBUF_SLIDE_FRAMES*1152 )
        sample_t mfbuf[2][MFBUF_SIZE];

        int     mf_samples_to_encode;
        int     mf_size;        /* the samples in use, from mfbuf[ch][mf_start] on */
        int     mf_start;       /* where in mfbuf the current frame starts */

        /* set when the quantization could not fit a frame into its bits, or the
           bits written disagree with the bit reservoir; every later encode call
           then fails with LAME_INTERNALERROR */
        int     internal_error;

    } EncStateVar_t;


    typedef struct {
        /* simple statistics */
        int     bitrate_channelmode_hist[16][4 + 1];
        int     bitrate_blocktype_hist[16][4 + 1 + 1]; /*norm/start/short/stop/mixed(short)/sum */

        int     bitrate_index;
        int     frame_number; /* number of frames encoded             */
        int     padding;     /* padding for the current frame? */
        int     mode_ext;
        int     encoder_delay;
        int     encoder_padding; /* number of samples of padding appended to input */
    } EncResult_t;


    /** \internal
       \brief The bits of the substep shaping setting.

       The three low bits are the caller's, as lame_set_substep() takes them;
       the encoder keeps one bit of its own in the same field. */
    enum substep_shaping_bits {
        SUBSTEP_TRUNCATE = 1,     /**< drop the small spectral lines after the loop */
        SUBSTEP_HALF_STEP = 2,    /**< pseudo half steps inside the loop */
        SUBSTEP_SHORT_BLOCKS = 4, /**< truncate short blocks too */
        SUBSTEP_USER_BITS = 0x7f, /**< every bit but the encoder's own */
        SUBSTEP_RESV_FULL = 0x80  /**< encoder's own: the reservoir is over 90 % full */
    };


    /** \internal
       \brief Which distorted bands the quantizer loop amplifies in one step. */
    enum noise_shaping_amp_mode {
        NS_AMP_ISO = 0,     /**< every band whose noise is not masked */
        NS_AMP_HALF_DB = 1, /**< the bands within half the largest distortion in dB */
        NS_AMP_ONE_BAND = 2 /**< only the most distorted band */
    };


    /* variables used by quantize.c */
    typedef struct {
        /* variables for nspsytune */
        FLOAT   longfact[SBMAX_l];
        FLOAT   shortfact[SBMAX_s];
        FLOAT   masking_lower;
        FLOAT   mask_adjust; /* the dbQ stuff */
        FLOAT   mask_adjust_short; /* the dbQ stuff */
        int     OldValue[2];
        int     CurrentStep[2];
        int     pseudohalf[SFBMAX];
        int     sfb21_extra; /* will be set in lame_init_params */
        int     substep_shaping; /* bits of enum substep_shaping_bits, see lame_set_substep() */


        char    bv_scf[SAMPLES_PER_GRANULE];
    } QntStateVar_t;


    typedef struct {
        replaygain_t *rgdata;
        /* ReplayGain */
    } RpgStateVar_t;


    typedef struct {
        FLOAT   noclipScale; /* user-specified scale factor required for preventing clipping */
        sample_t PeakSample;
        int     RadioGain;
        int     noclipGainChange; /* gain change required for preventing clipping */
    } RpgResult_t;


    typedef struct {
        int     version;     /* 0=MPEG-2/2.5  1=MPEG-1               */
        int     samplerate_index;
        int     sideinfo_len;

        int     noise_shaping; /* 0 = none
                                  1 = ISO AAC model
                                  2 = allow scalefac_select=1
                                */

        int     subblock_gain; /*  0 = no, 1 = yes */
        int     use_best_huffman; /* 0 = no.  1=outside loop  2=inside loop(slow) */
        int     noise_shaping_amp; /* enum noise_shaping_amp_mode */

        int     noise_shaping_stop; /* 0 = stop at over=0, all scalefacs amplified or
                                       a scalefac has reached max value
                                       1 = stop when all scalefacs amplified or
                                       a scalefac has reached max value
                                       2 = stop when all scalefacs amplified
                                     */


        int     full_outer_loop; /* 0 = stop early after 0 distortion found. 1 = full search */

        int     lowpassfreq;
        int     highpassfreq;
        int     samplerate_in; /* input_samp_rate in Hz. default=44.1 kHz     */
        int     samplerate_out; /* output_samp_rate. */
        int     channels_in; /* number of channels in the input data stream (PCM or decoded PCM) */
        int     channels_out; /* number of channels in the output data stream (not used for decoding) */
        int     mode_gr;     /* granules per frame */
        int     force_ms;    /* force M/S mode.  requires mode=1            */

        int     quant_comp;
        int     quant_comp_short;

        int     use_temporal_masking_effect;
        int     use_safe_joint_stereo;

        int     preset;

        vbr_mode vbr;
        int     vbr_avg_bitrate_kbps;
        int     vbr_min_bitrate_index; /* min bitrate index */
        int     vbr_max_bitrate_index; /* max bitrate index */
        int     avg_bitrate;
        int     enforce_min_bitrate; /* strictly enforce VBR_min_bitrate normaly, it will be violated for analog silence */

        int     findReplayGain; /* find the RG value? default=0       */
        int     findPeakSample;
        int     decode_on_the_fly; /* decode on the fly? default=0                */
        int     analysis;
        int     disable_reservoir;
        int     buffer_constraint;  /* enforce ISO spec as much as possible   */
        int     free_format;
        int     write_lame_tag; /* add Xing VBR tag?                           */

        int     error_protection; /* use 2 bytes per frame for a CRC checksum. default=0 */
        int     copyright;   /* mark as copyright. default=0           */
        int     original;    /* mark as original. default=1            */
        int     extension;   /* the MP3 'private extension' bit. Meaningless */
        int     emphasis;    /* Input PCM is emphased PCM (for
                                instance from one of the rarely
                                emphased CDs), it is STRONGLY not
                                recommended to use this, because
                                psycho does not take it into account,
                                and last but not least many decoders
                                don't care about these bits          */


        MPEG_mode mode;
        short_block_t short_blocks;

        float   interChRatio;
        float   msfix;       /* Naoki's adjustment of Mid/Side maskings */
        float   ATH_offset_db;/* add to ATH this many db            */
        float   ATH_offset_factor;/* change ATH by this factor, derived from ATH_offset_db */
        float   ATHcurve;    /* change ATH formula 4 shape           */
        int     ATHtype;
        int     ATHonly;     /* only use ATH                         */
        int     ATHshort;    /* only use ATH for short blocks        */
        int     noATH;       /* disable ATH                          */
        
        float   ATHfixpoint;

        float   adjust_alto_db;
        float   adjust_bass_db;
        float   adjust_treble_db;
        float   adjust_sfb21_db;

        float   compression_ratio; /* sizeof(wav file)/sizeof(mp3 file)          */

        /* lowpass and highpass filter control */
        FLOAT   lowpass1, lowpass2; /* normalized frequency bounds of passband */
        FLOAT   highpass1, highpass2; /* normalized frequency bounds of passband */

        /* scale input by this amount before encoding at least not used for MP3 decoding */
        FLOAT   pcm_transform[2][2];

        FLOAT   minval;
    } SessionConfig_t;


/* Whether the target is x86 at all.  It decides which CPU tests can be
   asked and which instruction sets the vector ladder below can name. */
#if defined( __i386__ ) || defined( __x86_64__ ) \
 || defined( _M_IX86 ) || defined( _M_X64 ) || defined( _M_AMD64 )
#define LAME_TARGET_X86 1
#endif

/* And whether it is ARM, 32- or 64-bit.  Both are named because the two
   answer the capability question differently: Advanced SIMD is architectural
   on AArch64 and optional on ARMv7, so the run-time test is not ceremony
   there. */
#if defined( __aarch64__ ) || defined( _M_ARM64 ) || defined( _M_ARM64EC ) \
 || defined( __arm__ ) || defined( __arm ) || defined( _M_ARM )
#define LAME_TARGET_ARM 1
#endif

/* Whether the compiler offers __builtin_cpu_supports() at all.  It is the
   question the unit tests ask as well, so it is answered here rather than
   beside any one of its callers.

   The architecture test is part of the question and not a shortcut for it:
   compilers offer the builtin on AArch64 too, where it answers about a
   different set of names and an x86 one is rejected. */
#if defined( LAME_TARGET_X86 )
# if defined( __has_builtin )
#  if __has_builtin( __builtin_cpu_supports )
#   define LAME_CPU_SUPPORTS 1
#  endif
# elif defined( __GNUC__ ) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
#  define LAME_CPU_SUPPORTS 1
# endif
#endif

/* And whether it may be asked about the AVX-512 subsets, which is a separate
   question: a feature name the compiler does not know is rejected when the
   call is compiled, not answered no at run time.  GCC did not know these
   names before 6, and reports __GNUC__ 4 when it is clang, which has known
   them for as long as it has had the builtin. */
#if defined( LAME_CPU_SUPPORTS ) \
 && (defined( __clang__ ) || !defined( __GNUC__ ) || __GNUC__ >= 6)
#define LAME_CPU_SUPPORTS_AVX512 1
#endif

    /* Which set of vector routines the encoder will run.  A wider
       implementation adds a value here and a name in the table in util.c,
       rather than another flag to test at each call site.  Ordered by
       capability, so a comparison picks the best available one: a routine
       that exists only at the lower tier is still chosen on a machine that
       offers the higher one.

       The tiers are the instruction sets themselves, so they are listed per
       architecture and each architecture reports names that exist on it.
       The type is internal, and a caller written as ">= the tier my routine
       needs" reads the same whichever architecture it is compiled for. */
    typedef enum {
        VECTOR_IMPL_NONE = 0
#if defined( LAME_TARGET_X86 )
        , VECTOR_IMPL_SSE2
        , VECTOR_IMPL_AVX2
        , VECTOR_IMPL_AVX512
#endif
#if defined( LAME_TARGET_ARM )
        , VECTOR_IMPL_NEON
#endif
    } vector_impl_t;


    struct lame_internal_flags {

  /********************************************************************
   * internal variables NOT set by calling program, and should not be *
   * modified by the calling program                                  *
   ********************************************************************/

        /*
         * Some remarks to the Class_ID field:
         * The Class ID is an Identifier for a pointer to this struct.
         * It is very unlikely that a pointer to lame_global_flags has the same 32 bits
         * in it's structure (large and other special properties, for instance prime).
         *
         * To test that the structure is right and initialized, use:
         *     if ( gfc -> Class_ID == LAME_ID ) ...
         * Other remark:
         *     If you set a flag to 0 for uninit data and 1 for init data, the right test
         *     should be "if (flag == 1)" and NOT "if (flag)". Unintended modification
         *     of this element will be otherwise misinterpreted as an init.
         */
#  define  LAME_ID   0xFFF88E3B
        unsigned long class_id;

        int     lame_init_params_successful;
        int     lame_encode_frame_init;
        int     iteration_init_init;
        int     fill_buffer_resample_init;

        SessionConfig_t cfg;
        uint16_t nMusicCRC;

        uint16_t _unused;

        /* variables used by lame.c */
        Bit_stream_struc bs;
        III_side_info_t l3_side;

        scalefac_struct scalefac_band;

        PsyStateVar_t sv_psy; /* DATA FROM PSYMODEL.C */
        PsyResult_t ov_psy;
        /* Which vector routines this instance runs.  Decided once, in
           lame_init_params(), rather than recomputed at each call site: a
           selection forced for performance testing has to be one that no
           call site can bypass. */
        vector_impl_t vector_impl;
        EncStateVar_t sv_enc; /* DATA FROM ENCODER.C */
        EncResult_t ov_enc;
        QntStateVar_t sv_qnt; /* DATA FROM QUANTIZE.C */
        /* CPU features */
        /* Only what the encoder dispatches on.  A feature nothing selects on
           has no bit here: it could only ever be reported, and a report of a
           capability the library will not use is what this replaced. */
        struct {
            unsigned int SSE2:1; /* Pentium 4, K8             */
            unsigned int AVX2:1; /* Haswell, Excavator        */
            unsigned int AVX512:1; /* Skylake-SP, Ice Lake, Zen 4 */
            unsigned int NEON:1; /* ARM Advanced SIMD         */
            unsigned int _unused:28;
        } CPU_features;

        RpgStateVar_t sv_rpg;
        RpgResult_t ov_rpg;

        /* optional ID3 tags, used in id3tag.c  */
        struct id3tag_spec tag_spec;


        VBR_seek_info_t VBR_seek_table; /* used for Xing VBR header */

        ATH_t  *ATH;         /* all ATH related stuff */

        PsyConst_t *cd_psy;

        /* used by the frame analyzer */
        plotting_data *pinfo;
        hip_t hip;

        /* functions to replace with CPU feature optimized versions in takehiro.c */
        int     (*choose_table) (const int *ix, const int *const end, int *const s);
        void    (*fft_fht) (FLOAT *, int);
        void    (*init_xrpow_core) (gr_info * const cod_info, FLOAT xrpow[SAMPLES_PER_GRANULE], int upper,
                                    FLOAT * sum);

        lame_report_function report_msg;
        lame_report_function report_dbg;
        lame_report_function report_err;
    };

#ifndef lame_internal_flags_defined
#define lame_internal_flags_defined
    typedef struct lame_internal_flags lame_internal_flags;
#endif


/***********************************************************************
*
*  Global Function Prototype Declarations
*
***********************************************************************/
    void    freegfc(lame_internal_flags * const gfc);
    void    free_init_state(lame_internal_flags * const gfc);
    void    free_id3tag(lame_internal_flags * const gfc);
    extern int BitrateIndex(int, int, int);
    extern int FindNearestBitrate(int, int, int);
    extern int map2MP3Frequency(int freq);
    extern int floorMP3Frequency(int freq);
    extern int SmpFrqIndex(int, int *const);
    extern int nearestBitrateFullIndex(int bitrate);
    extern FLOAT ATHformula(SessionConfig_t const *cfg, FLOAT freq);
    extern FLOAT freq2bark(FLOAT freq);
    void    disable_FPE(void);

/**
 * \internal
 * \brief Stores \a value in 2 bytes, the most significant byte first.
 * \param bytes  where the 2 bytes go.
 * \param value  the value.
 */
static inline void
put_be16(unsigned char *bytes, uint16_t value)
{
    bytes[0] = (unsigned char) (value >> 8);
    bytes[1] = (unsigned char) value;
}

/**
 * \internal
 * \brief Stores \a value in 4 bytes, the most significant byte first.
 * \param bytes  where the 4 bytes go.
 * \param value  the value.
 */
static inline void
put_be32(unsigned char *bytes, uint32_t value)
{
    bytes[0] = (unsigned char) (value >> 24);
    bytes[1] = (unsigned char) (value >> 16);
    bytes[2] = (unsigned char) (value >> 8);
    bytes[3] = (unsigned char) value;
}

/* log/log10 approximations */
    extern void init_log_table(void);
    extern ieee754_float32_t fast_log2(ieee754_float32_t x);

    int     isResamplingNecessary(SessionConfig_t const* cfg);

    int     fill_buffer(lame_internal_flags * gfc,
                        sample_t *const mfbuf[2],
                        sample_t const *const in_buffer[2], int nsamples, int *n_in, int *n_out);

/* same as lame_decode1 (look in lame.h), but returns
   unclipped raw floating-point samples. It is declared
   here, not in lame.h, because it returns LAME's
   internal type sample_t. No more than 1152 samples
   per channel are allowed.
   The samples are in LAME's own scale, where full scale
   is 32768, and are not limited to it: a stream that
   decodes past full scale says so here, which is what
   the clipping figures are measured from. */
    int     hip_decode1_unclipped(hip_t hip, unsigned char *mp3buf,
                                   size_t len, sample_t pcm_l[], sample_t pcm_r[]);


    extern int has_SSE2(void);
    extern int has_AVX2(void);
    extern int has_AVX512(void);
    extern int has_NEON(void);

/* The request stored in lame_global_flags: a vector_impl_t value, or this,
   meaning "whatever the machine offers".  It is not a vector_impl_t member
   because it is not a tier - it is the absence of a choice.  */
#define VECTOR_IMPL_AUTO (-1)

/* Buffer size for one name, terminator included.  Instruction-set names are
   short identifiers, so this is generous; it exists so the comparisons and
   the display buffers all state the same bound instead of repeating a
   literal.  A name that did not fit would be reported truncated, which is why
   the unit test asserts every one of them is shorter than this. */
#define VECTOR_IMPL_NAME_MAX 32

    /* The table in util.c is the single source of truth for the set of
       vector routines this build carries: the public enumeration, the
       name lookup, the setter's validation and the configuration report all
       read it.  Keeping them apart is how a build comes to advertise a set
       the encoder never dispatches. */
    extern int  vector_impl_count(void);
    extern const char *vector_impl_name_at(int index);
    extern int  vector_impl_from_name(const char *name, vector_impl_t * impl);
    extern int  vector_impl_known(const char *name);
    extern int  vector_impl_supported(vector_impl_t impl);

    /* Decides gfc->vector_impl from the request and what the CPU offers.
       Called once, from lame_init_params(); every consumer of
       vector_implementation() runs after it. */
    extern void vector_impl_init(lame_internal_flags * gfc, int request);

    extern vector_impl_t vector_implementation(lame_internal_flags const *gfc);
    extern const char *vector_impl_name(vector_impl_t impl);
    extern const char *vector_impl_display(const char *name, char *buf, size_t size);
    extern const char *vector_impl_available_list(char *buf, size_t size);



/***********************************************************************
*
*  Macros about Message Printing and Exit
*
***********************************************************************/

    extern void lame_report_def(const char* format, va_list args);
    extern void lame_report_fnc(lame_report_function print_f, const char *, ...);
    extern void lame_errorf(const lame_internal_flags * gfc, const char *, ...);
    extern void lame_debugf(const lame_internal_flags * gfc, const char *, ...);
    extern void lame_msgf(const lame_internal_flags * gfc, const char *, ...);
#define DEBUGF  lame_debugf
#define ERRORF  lame_errorf
#define MSGF    lame_msgf

    int     is_lame_internal_flags_valid(const lame_internal_flags * gfp);
    

#ifdef __cplusplus
}
#endif
#endif                       /* LAME_UTIL_H */
