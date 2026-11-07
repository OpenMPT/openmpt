/*
 *	lame utility library source file
 *
 *	Copyright (c) 1999 Albert L Faber
 *	Copyright (c) 2000-2005 Alexander Leidinger
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

#include <float.h>
#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "tables.h"

#define PRECOMPUTE
#if defined(__FreeBSD__) && !defined(__alpha__)
# include <machine/floatingpoint.h>
#endif
#if defined( _WIN32 )
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

/* For the debug build's floating point exception trapping below. */
#if defined( HAVE_FENV_H ) && defined( HAVE_FEENABLEEXCEPT )
# include <fenv.h>
#endif

/* Headers for the ARM capability probe in has_NEON() below.  configure says
 * which of these exist; this says where they are needed.  HWCAP_NEON lives in
 * a different header on each system that has it, and on the ones that do not
 * the probe compiles to "don't know, assume not".
 */
#if defined( HAVE_SYS_AUXV_H )
# include <sys/auxv.h>
#endif
#if defined( HAVE_MACHINE_ELF_H )
# include <machine/elf.h>
#endif
#if defined( HAVE_ASM_HWCAP_H )
# include <asm/hwcap.h>
#endif
#if defined( HAVE_SYSCTLBYNAME )
# include <sys/types.h>
# include <sys/sysctl.h>
#endif


/***********************************************************************
*
*  Global Function Definitions
*
***********************************************************************/
/*empty and close mallocs in gfc */

void
free_id3tag(lame_internal_flags * const gfc)
{
    gfc->tag_spec.language[0] = 0;
    if (gfc->tag_spec.title != 0) {
        free(gfc->tag_spec.title);
        gfc->tag_spec.title = 0;
    }
    if (gfc->tag_spec.artist != 0) {
        free(gfc->tag_spec.artist);
        gfc->tag_spec.artist = 0;
    }
    if (gfc->tag_spec.album != 0) {
        free(gfc->tag_spec.album);
        gfc->tag_spec.album = 0;
    }
    if (gfc->tag_spec.comment != 0) {
        free(gfc->tag_spec.comment);
        gfc->tag_spec.comment = 0;
    }

    if (gfc->tag_spec.albumart != 0) {
        free(gfc->tag_spec.albumart);
        gfc->tag_spec.albumart = 0;
        gfc->tag_spec.albumart_size = 0;
        gfc->tag_spec.albumart_mimetype = MIMETYPE_NONE;
    }
    if (gfc->tag_spec.v2_head != 0) {
        FrameDataNode *node = gfc->tag_spec.v2_head;
        do {
            void   *p = node->dsc.ptr.b;
            void   *q = node->txt.ptr.b;
            void   *r = node;
            node = node->nxt;
            free(p);
            free(q);
            free(r);
        } while (node != 0);
        gfc->tag_spec.v2_head = 0;
        gfc->tag_spec.v2_tail = 0;
    }
}


static void
free_global_data(lame_internal_flags * gfc)
{
    if (gfc && gfc->cd_psy) {
        if (gfc->cd_psy->l.s3) {
            /* XXX allocated in psymodel_init() */
            free(gfc->cd_psy->l.s3);
        }
        if (gfc->cd_psy->s.s3) {
            /* XXX allocated in psymodel_init() */
            free(gfc->cd_psy->s.s3);
        }
        free(gfc->cd_psy);
        gfc->cd_psy = 0;
    }
}


/*! Release what a failed \c lame_init_params() built. */
/*!
  \internal
  The next \c lame_init_params() then builds the psychoacoustic tables, the
  quantizer tables, the bitstream buffer and the decoder again, for its own
  settings. The ID3 tag data and the buffers of \c lame_init() stay.

  \param gfc the internal flags of the encoder instance.
*/
void
free_init_state(lame_internal_flags * const gfc)
{
    free_global_data(gfc);
    gfc->iteration_init_init = 0;
    if (gfc->bs.buf != NULL) {
        free(gfc->bs.buf);
        gfc->bs.buf = NULL;
    }
#ifdef HAVE_MPG123
    if (gfc->hip) {
        hip_decode_exit(gfc->hip);
        gfc->hip = 0;
    }
#endif
}


void
freegfc(lame_internal_flags * const gfc)
{                       /* bit stream structure */
    int     i;

    if (gfc == 0) return;

    for (i = 0; i <= 2 * BPC; i++)
        if (gfc->sv_enc.blackfilt[i] != NULL) {
            free(gfc->sv_enc.blackfilt[i]);
            gfc->sv_enc.blackfilt[i] = NULL;
        }
    if (gfc->sv_enc.inbuf_old[0]) {
        free(gfc->sv_enc.inbuf_old[0]);
        gfc->sv_enc.inbuf_old[0] = NULL;
    }
    if (gfc->sv_enc.inbuf_old[1]) {
        free(gfc->sv_enc.inbuf_old[1]);
        gfc->sv_enc.inbuf_old[1] = NULL;
    }

    if (gfc->bs.buf != NULL) {
        free(gfc->bs.buf);
        gfc->bs.buf = NULL;
    }

    if (gfc->VBR_seek_table.bag) {
        free(gfc->VBR_seek_table.bag);
        gfc->VBR_seek_table.bag = NULL;
        gfc->VBR_seek_table.size = 0;
    }
    if (gfc->ATH) {
        free(gfc->ATH);
    }
    if (gfc->sv_rpg.rgdata) {
        free(gfc->sv_rpg.rgdata);
    }
    if (gfc->sv_enc.in_buffer_0) {
        free(gfc->sv_enc.in_buffer_0);
    }
    if (gfc->sv_enc.in_buffer_1) {
        free(gfc->sv_enc.in_buffer_1);
    }
    free_id3tag(gfc);

#ifdef HAVE_MPG123
    if (gfc->hip) {
        hip_decode_exit(gfc->hip);
        gfc->hip = 0;
    }
#endif

    free_global_data(gfc);

    free(gfc);
}


/*those ATH formulas are returning
their minimum value for input = -1*/

/**
 * \brief The absolute threshold of hearing, in dB, at frequency \a f.
 *
 * Terhardt's threshold formula, as given by Painter and Spanias, with two
 * changes based on measured threshold values: an added term that raises the
 * curve around 8-9 kHz, and a high-frequency tail whose steepness \a value
 * selects. A comment in the source says that the unchanged formula was not
 * accurate at high frequencies, and that the correction costs a lot of
 * bitrate in VBR.
 *
 * With \a value, one formula covers the whole quality range. The tail goes
 * from close to the published curve at the lowest quality settings towards
 * the measured curve at the highest.
 *
 * \param f      frequency in Hz. A value below -0.3 returns the minimum of the
 *               curve instead of the value at a frequency.
 * \param value  selects the high-frequency tail.
 * \param f_min  frequency range of the curve. \a f is first limited to this
 * \param f_max  range, so the curve is flat outside it.
 * \return the threshold in dB.
 */
static  FLOAT
ATHformula_GB(FLOAT f, FLOAT value, FLOAT f_min, FLOAT f_max)
{
    /* from Painter & Spanias
       modified by Gabriel Bouvigne to better fit the reality
       ath =    3.640 * pow(f,-0.8)
       - 6.800 * exp(-0.6*pow(f-3.4,2.0))
       + 6.000 * exp(-0.15*pow(f-8.7,2.0))
       + 0.6* 0.001 * pow(f,4.0);


       In the past LAME was using the Painter &Spanias formula.
       But we had some recurrent problems with HF content.
       We measured real ATH values, and found the older formula
       to be inacurate in the higher part. So we made this new
       formula and this solved most of HF problematic testcases.
       The tradeoff is that in VBR mode it increases a lot the
       bitrate. */


/*this curve can be udjusted according to the VBR scale:
it adjusts from something close to Painter & Spanias
on V9 up to Bouvigne's formula for V0. This way the VBR
bitrate is more balanced according to the -V value.*/

    FLOAT   ath;

    /* the following Hack allows to ask for the lowest value */
    if (f < -.3)
        f = 3410;

    f /= 1000;          /* convert to khz */
    f = Max(f_min, f);
    f = Min(f_max, f);

    ath = 3.640 * pow(f, -0.8)
        - 6.800 * exp(-0.6 * pow(f - 3.4, 2.0))
        + 6.000 * exp(-0.15 * pow(f - 8.7, 2.0))
        + (0.6 + 0.04 * value) * 0.001 * pow(f, 4.0);
    return ath;
}



/**
 * \internal
 * \brief The absolute threshold of hearing for the configured curve.
 *
 * Calls ATHformula_GB() for the selected curve type. The types differ in the
 * high-frequency tail they use, in whether the caller can choose it, and in
 * the frequency range before the curve is flat. One of them also shifts the
 * whole curve.
 *
 * The threshold that the quantizer uses is not only this value. A configured
 * offset is added, and adjust_ATH() can lower the result in quiet passages.
 */
FLOAT
ATHformula(SessionConfig_t const *cfg, FLOAT freq)
{
    FLOAT   ath;
    switch (cfg->ATHtype) {
    case 0:
        ath = ATHformula_GB(freq, 9, 0.1f, 24.0f);
        break;
    case 1:
        ath = ATHformula_GB(freq, -1, 0.1f, 24.0f); /*over sensitive, should probably be removed */
        break;
    case 2:
        ath = ATHformula_GB(freq, 0, 0.1f, 24.0f);
        break;
    case 3:
        ath = ATHformula_GB(freq, 1, 0.1f, 24.0f) + 6; /*modification of GB formula by Roel */
        break;
    case 4:
        ath = ATHformula_GB(freq, cfg->ATHcurve, 0.1f, 24.0f);
        break;
    case 5:
        ath = ATHformula_GB(freq, cfg->ATHcurve, 3.41f, 16.1f);
        break;
    default:
        ath = ATHformula_GB(freq, 0, 0.1f, 24.0f);
        break;
    }
    return ath;
}

/* see for example "Zwicker: Psychoakustik, 1982; ISBN 3-540-11401-7 */
/**
 * \internal
 * \brief Convert a frequency in Hz to the Bark scale.
 *
 * On this scale, masking has about the same shape at every frequency, so one
 * spreading function covers all frequencies. This is Zwicker's analytic
 * approximation.
 *
 * \see s3_func(), init_numline()
 */
FLOAT
freq2bark(FLOAT freq)
{
    /* input: freq in hz  output: barks */
    if (freq < 0)
        freq = 0;
    freq = freq * 0.001;
    return 13.0 * atan(.76 * freq) + 3.5 * atan(freq * freq / (7.5 * 7.5));
}

#if 0
extern FLOAT freq2cbw(FLOAT freq);

/* see for example "Zwicker: Psychoakustik, 1982; ISBN 3-540-11401-7 */
FLOAT
freq2cbw(FLOAT freq)
{
    /* input: freq in hz  output: critical band width */
    freq = freq * 0.001;
    return 25 + 75 * pow(1 + 1.4 * (freq * freq), 0.69);
}

#endif




#define ABS(A) (((A)>0) ? (A) : -(A))

int
FindNearestBitrate(int bRate, /* legal rates from 8 to 320 */
                   int version, int samplerate)
{                       /* MPEG-1 or MPEG-2 LSF */
    int     bitrate;
    int     i;

    if (samplerate < 16000)
        version = 2;

    bitrate = bitrate_table[version][1];

    /* the distances in double, which holds every difference of two ints
       exactly: bRate is whatever the caller set */
    for (i = 2; i <= 14; i++) {
        if (bitrate_table[version][i] > 0) {
            if (ABS((double) bitrate_table[version][i] - bRate) < ABS((double) bitrate - bRate))
                bitrate = bitrate_table[version][i];
        }
    }
    return bitrate;
}





/* Used to find table index when
 * we need bitrate-based values
 * determined using tables
 *
 * bitrate in kbps
 *
 * Gabriel Bouvigne 2002-11-03
 */
int
nearestBitrateFullIndex(int bitrate)
{
    int     lower_range = 0, lower_range_kbps = 0, upper_range = 0, upper_range_kbps = 0;


    int     b;


    /* We assume specified bitrate will be 320kbps */
    upper_range_kbps = full_bitrate_table[16];
    upper_range = 16;
    lower_range_kbps = full_bitrate_table[16];
    lower_range = 16;

    /* Determine which significant bitrates the value specified falls between,
     * if loop ends without breaking then we were correct above that the value was 320
     */
    for (b = 0; b < 16; b++) {
        if ((Max(bitrate, full_bitrate_table[b + 1])) != bitrate) {
            upper_range_kbps = full_bitrate_table[b + 1];
            upper_range = b + 1;
            lower_range_kbps = full_bitrate_table[b];
            lower_range = (b);
            break;      /* We found upper range */
        }
    }

    /* Determine which range the value specified is closer to; the distances in
       double, which holds every difference of two ints exactly */
    if (((double) upper_range_kbps - bitrate) > ((double) bitrate - lower_range_kbps)) {
        return lower_range;
    }
    return upper_range;
}





/** The sample rates of MP3, in Hz, from the lowest to the highest. */
static const int mp3_samplerates[] =
    { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 };

/* map frequency to a valid MP3 sample frequency
 *
 * Robert Hegemann 2000-07-01
 */
int
map2MP3Frequency(int freq)
{
    size_t  i;

    for (i = 0; i + 1 < dimension_of(mp3_samplerates); i++) {
        if (freq <= mp3_samplerates[i])
            return mp3_samplerates[i];
    }
    return mp3_samplerates[dimension_of(mp3_samplerates) - 1];
}

/**
 * \internal
 * \brief Returns the highest MP3 sample rate that is not above \a freq.
 * \param freq a sample rate in Hz.
 * \return the MP3 sample rate in Hz. 0 when \a freq is below the lowest one.
 */
int
floorMP3Frequency(int freq)
{
    size_t  i = dimension_of(mp3_samplerates);

    while (i > 0) {
        --i;
        if (mp3_samplerates[i] <= freq)
            return mp3_samplerates[i];
    }
    return 0;
}

int
BitrateIndex(int bRate,      /* legal rates from 32 to 448 kbps */
             int version,    /* MPEG-1 or MPEG-2/2.5 LSF */
             int samplerate)
{                       /* convert bitrate in kbps to index */
    int     i;
    if (samplerate < 16000)
        version = 2;
    for (i = 0; i <= 14; i++) {
        if (bitrate_table[version][i] > 0) {
            if (bitrate_table[version][i] == bRate) {
                return i;
            }
        }
    }
    return -1;
}

/* convert samp freq in Hz to index */

int
SmpFrqIndex(int sample_freq, int *const version)
{
    int     row, i;

    /* Rows 0, 1 and 2 of the table are MPEG-2, MPEG-1 and MPEG-2.5. The
       version bit is 0 for MPEG-2.5 too; the sample rate tells it apart. */
    for (row = 0; row < 3; row++) {
        for (i = 0; i < 3; i++) {
            if (samplerate_table[row][i] == sample_freq) {
                *version = (row == 1) ? 1 : 0;
                return i;
            }
        }
    }
    *version = 0;
    return -1;
}


/*****************************************************************************
*
*  End of bit_stream.c package
*
*****************************************************************************/










/* resampling via FIR filter, blackman window */
inline static FLOAT
blackman(FLOAT x, FLOAT fcn, int l)
{
    /* This algorithm from:
       SIGNAL PROCESSING ALGORITHMS IN FORTRAN AND C
       S.D. Stearns and R.A. David, Prentice-Hall, 1992
     */
    FLOAT   bkwn, x2;
    FLOAT const wcn = (PI * fcn);

    x /= l;
    if (x < 0)
        x = 0;
    if (x > 1)
        x = 1;
    x2 = x - .5;

    bkwn = 0.42 - 0.5 * cos(2 * x * PI) + 0.08 * cos(4 * x * PI);
    if (fabs(x2) < 1e-9)
        return wcn / PI;
    else
        return (bkwn * sin(l * wcn * x2) / (PI * l * x2));


}




/* gcd - greatest common divisor */
/* Joint work of Euclid and M. Hendry */

static int
gcd(int i, int j)
{
    /*    assert ( i > 0  &&  j > 0 ); */
    return j ? gcd(j, i % j) : i;
}



static int
fill_buffer_resample(lame_internal_flags * gfc,
                     sample_t * outbuf,
                     int desired_len, sample_t const *inbuf, int len, int *num_used, int ch)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncStateVar_t *esv = &gfc->sv_enc;
    double  resample_ratio = (double)cfg->samplerate_in / (double)cfg->samplerate_out;
    int     BLACKSIZE;
    FLOAT   offset;
    int     i, j = 0, k;
    int     filter_l;
    int     alloc_failed;
    FLOAT   fcn, intratio;
    FLOAT  *inbuf_old;
    int     bpc;             /* number of convolution functions to pre-compute */
    bpc = cfg->samplerate_out / gcd(cfg->samplerate_out, cfg->samplerate_in);
    if (bpc > BPC)
        bpc = BPC;

    intratio = (fabs(resample_ratio - floor(.5 + resample_ratio)) < FLT_EPSILON);
    fcn = 1.00 / resample_ratio;
    if (fcn > 1.00)
        fcn = 1.00;
    filter_l = RESAMPLE_FILTER_LENGTH; /* must be odd */
    filter_l += intratio; /* unless resample_ratio=int, it must be even */


    BLACKSIZE = filter_l + 1; /* size of data needed for FIR */

    if (gfc->fill_buffer_resample_init == 0) {
        esv->inbuf_old[0] = lame_calloc(sample_t, BLACKSIZE);
        esv->inbuf_old[1] = lame_calloc(sample_t, BLACKSIZE);
        for (i = 0; i <= 2 * bpc; ++i)
            esv->blackfilt[i] = lame_calloc(sample_t, BLACKSIZE);

        alloc_failed = (esv->inbuf_old[0] == NULL || esv->inbuf_old[1] == NULL);
        for (i = 0; i <= 2 * bpc; ++i) {
            if (esv->blackfilt[i] == NULL) {
                alloc_failed = 1;
            }
        }
        if (alloc_failed) {
            if (esv->inbuf_old[0]) {
                free(esv->inbuf_old[0]);
                esv->inbuf_old[0] = 0;
            }
            if (esv->inbuf_old[1]) {
                free(esv->inbuf_old[1]);
                esv->inbuf_old[1] = 0;
            }
            for (i = 0; i <= 2 * bpc; ++i) {
                if (esv->blackfilt[i]) {
                    free(esv->blackfilt[i]);
                    esv->blackfilt[i] = 0;
                }
            }
            ERRORF(gfc, "Error: can't allocate resample buffers\n");
            return -1;
        }

        esv->itime[0] = 0;
        esv->itime[1] = 0;

        /* precompute blackman filter coefficients */
        for (j = 0; j <= 2 * bpc; j++) {
            FLOAT   sum = 0.;
            offset = (j - bpc) / (2. * bpc);
            for (i = 0; i <= filter_l; i++)
                sum += esv->blackfilt[j][i] = blackman(i - offset, fcn, filter_l);
            for (i = 0; i <= filter_l; i++)
                esv->blackfilt[j][i] /= sum;
        }
        gfc->fill_buffer_resample_init = 1;
    }

    inbuf_old = esv->inbuf_old[ch];

    /* time of j'th element in inbuf = itime + j/ifreq; */
    /* time of k'th element in outbuf   =  j/ofreq */
    for (k = 0; k < desired_len; k++) {
        double  time0 = k * resample_ratio; /* time of k'th output sample */
        FLOAT   xvalue;
        int     joff;

        j = floor(time0 - esv->itime[ch]);

        /* check if we need more input data */
        if ((filter_l + j - filter_l / 2) >= len)
            break;

        /* blackman filter.  by default, window centered at j+.5(filter_l%2) */
        /* but we want a window centered at time0.   */
        offset = (time0 - esv->itime[ch] - (j + .5 * (filter_l % 2)));
        /* offset is bounded to [-0.5, +0.5) by construction, so joff below
           always lands within blackfilt[0 .. 2*bpc] and never over-indexes
           the table: for a non-integer resample ratio filter_l is odd, making
           offset the fractional part of (time0 - itime) minus 0.5; for a
           (near-)integer ratio the intratio branch above is only taken when
           the ratio is an *exact* integer (samplerate_in an exact multiple of
           samplerate_out), where offset is identically 0.  Sample rates are
           always integers, so no runtime bounds guard is needed here - the
           assert is a development-time sanity check, compiled out under NDEBUG. */
        assert(fabs(offset) <= .501);

        /* find the closest precomputed window for this offset: */
        joff = floor((offset * 2 * bpc) + bpc + .5);

        xvalue = 0.;
        for (i = 0; i <= filter_l; ++i) {
            int const j2 = i + j - filter_l / 2;
            sample_t y;
            assert(j2 < len);
            assert(j2 + BLACKSIZE >= 0);
            y = (j2 < 0) ? inbuf_old[BLACKSIZE + j2] : inbuf[j2];
#ifdef PRECOMPUTE
            xvalue += y * esv->blackfilt[joff][i];
#else
            xvalue += y * blackman(i - offset, fcn, filter_l); /* very slow! */
#endif
        }
        outbuf[k] = xvalue;
    }


    /* k = number of samples added to outbuf */
    /* last k sample used data from [j-filter_l/2,j+filter_l-filter_l/2]  */

    /* how many samples of input data were used:  */
    *num_used = Min(len, filter_l + j - filter_l / 2);

    /* adjust our input time counter.  Incriment by the number of samples used,
     * then normalize so that next output sample is at time 0, next
     * input buffer is at time itime[ch] */
    esv->itime[ch] += *num_used - k * resample_ratio;

    /* save the last BLACKSIZE samples into the inbuf_old buffer */
    if (*num_used >= BLACKSIZE) {
        for (i = 0; i < BLACKSIZE; i++)
            inbuf_old[i] = inbuf[*num_used + i - BLACKSIZE];
    }
    else {
        /* shift in *num_used samples into inbuf_old  */
        int const n_shift = BLACKSIZE - *num_used; /* number of samples to shift */

        /* shift n_shift samples by *num_used, to make room for the
         * num_used new samples */
        for (i = 0; i < n_shift; ++i)
            inbuf_old[i] = inbuf_old[i + *num_used];

        /* shift in the *num_used samples */
        for (j = 0; i < BLACKSIZE; ++i, ++j)
            inbuf_old[i] = inbuf[j];

        assert(j == *num_used);
    }
    return k;           /* return the number samples created at the new samplerate */
}

int
isResamplingNecessary(SessionConfig_t const* cfg)
{
    return cfg->samplerate_in != cfg->samplerate_out ? 1 : 0;
}

/* copy in new samples from in_buffer into mfbuf, with resampling
   if necessary.  n_in = number of samples from the input buffer that
   were used.  n_out = number of samples copied into mfbuf  */

int
fill_buffer(lame_internal_flags * gfc,
            sample_t * const mfbuf[2], sample_t const * const in_buffer[2], int nsamples, int *n_in, int *n_out)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    int     mf_size = gfc->sv_enc.mf_size;
    int     framesize = SAMPLES_PER_GRANULE * cfg->mode_gr;
    int     nout, ch = 0;
    int     nch = cfg->channels_out;

    /* copy in new samples into mfbuf, with resampling if necessary */
    if (isResamplingNecessary(cfg)) {
        do {
            nout =
                fill_buffer_resample(gfc, &mfbuf[ch][mf_size],
                                     framesize, in_buffer[ch], nsamples, n_in, ch);
            if (nout < 0) {
                return -1;
            }
        } while (++ch < nch);
        *n_out = nout;
    }
    else {
        nout = Min(framesize, nsamples);
        do {
            memcpy(&mfbuf[ch][mf_size], &in_buffer[ch][0], nout * sizeof(mfbuf[0][0]));
        } while (++ch < nch);
        *n_out = nout;
        *n_in = nout;
    }
    return 0;
}







/***********************************************************************
*
*  Message Output
*
***********************************************************************/

void
lame_report_def(const char *format, va_list args)
{
    (void) vfprintf(stderr, format, args);
    fflush(stderr); /* an debug function should flush immediately */
}

void 
lame_report_fnc(lame_report_function print_f, const char *format, ...)
{
    if (print_f) {
        va_list args;
        va_start(args, format);
        print_f(format, args);
        va_end(args);
    }
}


void
lame_debugf(const lame_internal_flags* gfc, const char *format, ...)
{
    if (gfc && gfc->report_dbg) {
        va_list args;
        va_start(args, format);
        gfc->report_dbg(format, args);
        va_end(args);
    }
}


void
lame_msgf(const lame_internal_flags* gfc, const char *format, ...)
{
    if (gfc && gfc->report_msg) {
        va_list args;
        va_start(args, format);
        gfc->report_msg(format, args);
        va_end(args);
    }
}


void
lame_errorf(const lame_internal_flags* gfc, const char *format, ...)
{
    if (gfc && gfc->report_err) {
        va_list args;
        va_start(args, format);
        gfc->report_err(format, args);
        va_end(args);
    }
}



/***********************************************************************
 *
 *      routines to detect CPU specific features like SSE2, AVX2
 *
 *  donated by Frank Klemm
 *  added Robert Hegemann 2000-10-10
 *
 ***********************************************************************/

/* Two separate questions, and conflating them is how the vector routines
 * ended up compiled but unreachable:
 *
 *   - may the compiler emit these instructions here unconditionally?  That
 *     is a property of the target baseline, and every toolchain publishes it
 *     under a name of its own.
 *   - does the CPU running this binary have them?  That is a runtime
 *     question, and only worth asking when the answer above is no.
 */
#if defined( __SSE2__ )                                       /* GCC, Clang */ \
 || defined( _M_X64 ) || defined( _M_AMD64 )                  /* MSVC x64   */ \
 || (defined( _M_IX86_FP ) && _M_IX86_FP >= 2)                /* MSVC /arch */
#define LAME_BASELINE_SSE2 1
#endif
#if defined( __AVX2__ )
#define LAME_BASELINE_AVX2 1
#endif
/* The tier needs four subsets, so the baseline has to carry all four before it
 * can answer yes: the kernels narrow to sixteen bits (BW), use the packed
 * integer forms (DQ), and address the shorter widths on the same encodings
 * (VL). */
#if defined( __AVX512F__ ) && defined( __AVX512VL__ ) \
 && defined( __AVX512BW__ ) && defined( __AVX512DQ__ )
#define LAME_BASELINE_AVX512 1
#endif

/* The same question on ARM.  Advanced SIMD is architectural on AArch64, so
 * every 64-bit toolchain answers it yes and the run-time probe below is never
 * consulted there.  It is optional on ARMv7 - Cortex-A9 shipped in
 * configurations without it - so on 32-bit ARM the probe is what decides, and
 * a build that assumed otherwise would fault on the first vector instruction
 * rather than fall back.
 *
 * MSVC defines neither __ARM_NEON nor __ARM_NEON__; on _M_ARM64 the
 * instructions are unconditionally available, and on _M_ARM they are implied
 * by the ARMv7 floating-point ABI it targets.
 */
#if defined( __ARM_NEON ) || defined( __ARM_NEON__ ) \
 || defined( _M_ARM64 ) || defined( _M_ARM64EC )
#define LAME_BASELINE_NEON 1
#endif

/* LAME_TARGET_X86 and the two LAME_CPU_SUPPORTS macros come from util.h,
   where the vector ladder and the unit tests need them too.  MSVC has neither
   builtin and answers from CPUID instead. */
#if defined( LAME_TARGET_X86 ) && defined( _MSC_VER )
# include <intrin.h>
# define LAME_CPUID_MSVC 1
#endif

#if defined( LAME_CPUID_MSVC )
/* Feature bits of the EDX result of CPUID leaf 1. */
enum {
    CPUID1_EDX_SSE2 = 26
};

/* The wide registers take three answers rather than one: the CPU has to
 * carry the instructions, the operating system has to preserve the wider
 * register state across a context switch, and only then may the state bits
 * be read at all.  A test that asks the first question alone reports yes on
 * a system where the registers are silently corrupted.
 */
enum {
    CPUID1_ECX_OSXSAVE = 27,
    CPUID7_EBX_AVX2 = 5,
    CPUID7_EBX_AVX512F = 16,
    CPUID7_EBX_AVX512DQ = 17,
    CPUID7_EBX_AVX512BW = 30,
    CPUID7_EBX_AVX512VL = 31,
    XCR0_SSE_AND_AVX_STATE = 0x6,
    /* ... and for the 512-bit registers, three more pieces of state: the
       opmask registers, the upper half of the low sixteen, and the sixteen
       the wider encoding adds. */
    XCR0_AVX512_STATE = 0xe6
};

static int
cpuid_feature_edx(int bit)
{
    int     regs[4];
    __cpuid(regs, 0);
    if (regs[0] < 1)
        return 0;
    __cpuid(regs, 1);
    return (regs[3] >> bit) & 1;
}

static int
cpuid_feature_avx2(void)
{
    int     regs[4];
    __cpuid(regs, 0);
    if (regs[0] < 7)
        return 0;
    __cpuid(regs, 1);
    if (((regs[2] >> CPUID1_ECX_OSXSAVE) & 1) == 0)
        return 0;
    if ((_xgetbv(0) & XCR0_SSE_AND_AVX_STATE) != XCR0_SSE_AND_AVX_STATE)
        return 0;
    __cpuidex(regs, 7, 0);
    return (regs[1] >> CPUID7_EBX_AVX2) & 1;
}

static int
cpuid_feature_avx512(void)
{
    int     regs[4];
    int     ebx;
    __cpuid(regs, 0);
    if (regs[0] < 7)
        return 0;
    __cpuid(regs, 1);
    if (((regs[2] >> CPUID1_ECX_OSXSAVE) & 1) == 0)
        return 0;
    if ((_xgetbv(0) & XCR0_AVX512_STATE) != XCR0_AVX512_STATE)
        return 0;
    __cpuidex(regs, 7, 0);
    ebx = regs[1];
    return ((ebx >> CPUID7_EBX_AVX512F) & 1)
        && ((ebx >> CPUID7_EBX_AVX512DQ) & 1)
        && ((ebx >> CPUID7_EBX_AVX512BW) & 1)
        && ((ebx >> CPUID7_EBX_AVX512VL) & 1);
}
#endif

/* __builtin_cpu_supports() is documented to return nonzero, not one, and GCC
 * really does return a bitmask (sse2 -> 16).  These results feed single-bit
 * fields, so every one of them is normalised here rather than at the caller.
 */
int
has_SSE2(void)
{
#if defined( LAME_BASELINE_SSE2 )
    return 1;
#elif defined( LAME_CPU_SUPPORTS )
    return __builtin_cpu_supports("sse2") != 0;
#elif defined( LAME_CPUID_MSVC )
    return cpuid_feature_edx(CPUID1_EDX_SSE2);
#else
    return 0;           /* don't know, assume not */
#endif
}

int
has_AVX2(void)
{
#if defined( LAME_BASELINE_AVX2 )
    return 1;
#elif defined( LAME_CPU_SUPPORTS )
    return __builtin_cpu_supports("avx2") != 0;
#elif defined( LAME_CPUID_MSVC )
    return cpuid_feature_avx2();
#else
    return 0;           /* don't know, assume not */
#endif
}

int
has_AVX512(void)
{
#if !defined( HAVE_AVX512_INTRINSICS )
    /* The dispatch table is the only reader of this answer and is compiled
       under the same condition, so where the routines do not exist there is
       nothing to decide - and a build with no use for the query should not
       be making it. */
    return 0;
#elif defined( LAME_BASELINE_AVX512 )
    return 1;
#elif defined( LAME_CPU_SUPPORTS_AVX512 )
    /* Asked as four questions for the same reason the baseline test is four:
       the kernels use all four subsets, and a CPU carrying only the
       foundation would fault on the rest. */
    return __builtin_cpu_supports("avx512f") != 0
        && __builtin_cpu_supports("avx512vl") != 0
        && __builtin_cpu_supports("avx512bw") != 0
        && __builtin_cpu_supports("avx512dq") != 0;
#elif defined( LAME_CPUID_MSVC )
    return cpuid_feature_avx512();
#else
    return 0;           /* don't know, assume not */
#endif
}

/* One question, four ways of asking it.  x86 has __builtin_cpu_supports() and
 * CPUID and nothing else to know; ARM has no unprivileged instruction that
 * reports the feature registers, so the answer has to come from the operating
 * system, and every one of them spells it differently.  The variation stays
 * here: the rest of the library asks has_NEON() and nothing else.
 *
 * Which of these can be compiled is decided by configure, not guessed from
 * platform macros - the header and the function are separate questions and a
 * system can have one without the other.
 */
int
has_NEON(void)
{
#if defined( LAME_BASELINE_NEON )
    return 1;
#elif !defined( LAME_TARGET_ARM )
    return 0;           /* not an ARM target; the question does not arise */
#elif defined( HAVE_ELF_AUX_INFO )                              /* FreeBSD */
    {
        unsigned long cap = 0;

        if (elf_aux_info(AT_HWCAP, &cap, sizeof cap) != 0)
            return 0;
# if defined( HWCAP_NEON )
        return (cap & HWCAP_NEON) != 0;
# else
        return 0;
# endif
    }
#elif defined( HAVE_GETAUXVAL )                          /* Linux, Android */
    {
        unsigned long const cap = getauxval(AT_HWCAP);

# if defined( HWCAP_NEON )
        return (cap & HWCAP_NEON) != 0;
# else
        return 0;
# endif
    }
#elif defined( HAVE_SYSCTLBYNAME )                           /* macOS, iOS */
    {
        int     v = 0;
        size_t  len = sizeof v;

        if (sysctlbyname("hw.optional.neon", &v, &len, NULL, 0) != 0)
            return 0;
        return v != 0;
    }
#elif defined( _WIN32 )                                  /* Windows on ARM */
    return IsProcessorFeaturePresent(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE) != 0;
#else
    return 0;           /* don't know, assume not */
#endif
}

/** @internal
 * The vector routine sets of this build.  All code reads this table to find
 * out how many sets the build has, their names, whether a name is valid,
 * whether this CPU can run a set, and which set an encode uses.  Because
 * there is only one table, the sets that the library reports always match the
 * sets that the encoder can use.
 *
 * The order is ascending capability, and the public API promises this order:
 * index count-1 is the widest set of this build, so "the best available" is
 * found from the end.  The names are the identifiers that the API takes:
 * lowercase, and the real instruction set, not a family, so SSE2 and not SSE.
 * For display, the name is converted to uppercase; there is no second string.
 *
 * On x86 the tiers are scalar, SSE2, AVX2 and AVX-512, with no SSE4.1/SSE4.2
 * tier, and the top tier has only the quantization loop of count_bits().  On
 * ARM the tiers are scalar and NEON, and NEON has one routine, because the
 * compiler already vectorizes the rest as well or better.  See
 * @ref vector_dispatch for the reasons.
 */
static const struct {
    const char *name;
    vector_impl_t impl;
    int     (*available)(void);
} vector_impl_table[] = {
#if defined( HAVE_SSE2_INTRINSICS )
    { "sse2", VECTOR_IMPL_SSE2, has_SSE2 },
#endif
#if defined( HAVE_AVX2_INTRINSICS )
    { "avx2", VECTOR_IMPL_AVX2, has_AVX2 },
#endif
#if defined( HAVE_AVX512_INTRINSICS )
    { "avx512", VECTOR_IMPL_AVX512, has_AVX512 },
#endif
#if defined( HAVE_NEON_INTRINSICS )
    { "neon", VECTOR_IMPL_NEON, has_NEON },
#endif
    { 0, VECTOR_IMPL_NONE, 0 }  /* C89 forbids an empty initialiser, and a
                                   build with no vector routines at all needs
                                   this array to exist anyway. */
};

/** @internal
 * Every name LAME knows, on any architecture and in any configuration.
 *
 * The table above lists the sets this build has.  This list has every set
 * name LAME uses, so that an error can give the reason.  A name in this list
 * but not in the table is not compiled into this build (rebuild, or use
 * another binary).  A name in neither list is wrong, for example a typo.
 *
 * These are plain strings, because the list must also name sets that this
 * architecture does not have, and their vector_impl_t values do not exist
 * here.
 *
 * The two lists can differ.  Every name in the table must also be in this
 * list.  test_vector_ladder.c checks this for the sets that a build compiles.
 */
static const char *const vector_impl_known_names[] = {
    "sse2", "avx2", "avx512", "neon", 0
};

/* Exact match within the bound every name is required to fit: both strings
   are terminated inside it, so the comparison stops at the first difference
   or at a terminator and neither is read past its own end. */
static int
name_equals(const char *name, const char *known)
{
    return strncmp(name, known, VECTOR_IMPL_NAME_MAX) == 0;
}

/* Whether LAME knows the name at all, whatever this build compiled. */
int
vector_impl_known(const char *name)
{
    int     i;

    if (name == 0)
        return 0;
    for (i = 0; vector_impl_known_names[i] != 0; ++i) {
        if (name_equals(name, vector_impl_known_names[i]))
            return 1;
    }
    return 0;
}

/* The sentinel is not a set, so it is not counted. */
int
vector_impl_count(void)
{
    return (int) (sizeof vector_impl_table / sizeof vector_impl_table[0]) - 1;
}

const char *
vector_impl_name_at(int index)
{
    if (index < 0 || index >= vector_impl_count())
        return 0;
    return vector_impl_table[index].name;
}

/* 1 and *impl set when the name is one this build carries, 0 otherwise. */
int
vector_impl_from_name(const char *name, vector_impl_t * impl)
{
    int     i;
    int const n = vector_impl_count();

    if (name == 0)
        return 0;
    for (i = 0; i < n; ++i) {
        if (name_equals(name, vector_impl_table[i].name)) {
            *impl = vector_impl_table[i].impl;
            return 1;
        }
    }
    return 0;
}

/* Whether the processor running this can execute that set.  VECTOR_IMPL_NONE
   is the scalar code, which every machine can run. */
int
vector_impl_supported(vector_impl_t impl)
{
    int     i;
    int const n = vector_impl_count();

    if (impl == VECTOR_IMPL_NONE)
        return 1;
    for (i = 0; i < n; ++i) {
        if (vector_impl_table[i].impl == impl)
            return vector_impl_table[i].available() != 0;
    }
    return 0;
}

/** @internal
 * Decide, once, which set this instance runs.
 *
 * @p request is VECTOR_IMPL_AUTO, or a set that was checked when it was
 * selected.  With AUTO the result is the widest set of the machine that the
 * deprecated asm_optimizations mask still allows.  The output of the encoder
 * must not depend on which of the two APIs a caller used.
 */
void
vector_impl_init(lame_internal_flags * gfc, int request)
{
    int     i;

    if (request != VECTOR_IMPL_AUTO) {
        gfc->vector_impl = (vector_impl_t) request;
        return;
    }
    gfc->vector_impl = VECTOR_IMPL_NONE;
    /* Walk down from the widest: the first the machine offers wins.  The
       CPU_features bits are already masked by the legacy enable flags. */
    for (i = vector_impl_count() - 1; i >= 0; --i) {
        switch (vector_impl_table[i].impl) {
#if defined( HAVE_AVX512_INTRINSICS )
        case VECTOR_IMPL_AVX512:
            if (gfc->CPU_features.AVX512) {
                gfc->vector_impl = VECTOR_IMPL_AVX512;
                return;
            }
            break;
#endif
#if defined( HAVE_AVX2_INTRINSICS )
        case VECTOR_IMPL_AVX2:
            if (gfc->CPU_features.AVX2) {
                gfc->vector_impl = VECTOR_IMPL_AVX2;
                return;
            }
            break;
#endif
#if defined( HAVE_SSE2_INTRINSICS )
        case VECTOR_IMPL_SSE2:
            if (gfc->CPU_features.SSE2) {
                gfc->vector_impl = VECTOR_IMPL_SSE2;
                return;
            }
            break;
#endif
#if defined( HAVE_NEON_INTRINSICS )
        case VECTOR_IMPL_NEON:
            if (gfc->CPU_features.NEON) {
                gfc->vector_impl = VECTOR_IMPL_NEON;
                return;
            }
            break;
#endif
        default:
            break;
        }
    }
}

vector_impl_t
vector_implementation(lame_internal_flags const *gfc)
{
    return gfc->vector_impl;
}

const char *
vector_impl_name(vector_impl_t impl)
{
    int     i;
    int const n = vector_impl_count();

    for (i = 0; i < n; ++i) {
        if (vector_impl_table[i].impl == impl)
            return vector_impl_table[i].name;
    }
    return "none";
}

/* Upper-case @p name into @p buf, returning how many characters were written
   - never more than size-1, and the result is always terminated.  Callers
   that go on to assemble a longer string take the length from here rather
   than measuring the buffer afterwards. */
static size_t
display_name(const char *name, char *buf, size_t size)
{
    size_t  i;

    for (i = 0; i + 1 < size && name[i] != '\0'; ++i)
        buf[i] = (char) toupper((unsigned char) name[i]);
    buf[i] = '\0';
    return i;
}

/** @internal
 * The display form of a set: the identifier in uppercase.
 *
 * There is no second string per set.  One name is stored, in the form that
 * the API takes, and the display form comes from it.  So the two cannot
 * differ, and a new set needs one string.
 */
const char *
vector_impl_display(const char *name, char *buf, size_t size)
{
    if (name == 0 || buf == 0 || size == 0)
        return "";
    (void) display_name(name, buf, size);
    return buf;
}

/** @internal
 * The sets of this build that the processor can also run, in display form,
 * in table order, separated by commas.
 *
 * Both conditions are needed.  A set that the build does not have cannot run
 * on any processor.  A set that the processor does not have cannot run in any
 * build.  The rest is exactly what lame_set_vector_routines() accepts here.
 *
 * The result is empty when there are none, which is a normal result.  Each
 * entry is added whole or not at all.
 */
const char *
vector_impl_available_list(char *buf, size_t size)
{
    int     i;
    int const n = vector_impl_count();
    size_t  used = 0;   /* characters written, terminator excluded */

    if (buf == 0 || size == 0)
        return "";
    buf[0] = '\0';
    for (i = 0; i < n; ++i) {
        char    disp[VECTOR_IMPL_NAME_MAX];
        size_t  sep, len;

        if (!vector_impl_supported(vector_impl_table[i].impl))
            continue;
        sep = (used > 0) ? 2 : 0;
        len = display_name(vector_impl_table[i].name, disp, sizeof disp);
        /* Whole entries only.  A truncated instruction-set name would read as
           a different one, so a name that does not fit ends the list. */
        if (used + sep + len + 1 > size)
            break;
        if (sep > 0) {
            strncat(buf, ", ", size - used - 1);
            used += sep;
        }
        strncat(buf, disp, size - used - 1);
        used += len;
    }
    return buf;
}

void
disable_FPE(void)
{
/* extremly system dependent stuff, move to a lib to make the code readable */
/*==========================================================================*/



    /*
     *  Disable floating point exceptions
     */




#if defined(__FreeBSD__) && defined(FP_X_INV) && defined(FP_X_DZ)
    {
        /* seet floating point mask to the Linux default */
        fp_except_t mask;
        mask = fpgetmask();
        /* if bit is set, we get SIGFPE on that error! */
        fpsetmask(mask & ~(FP_X_INV | FP_X_DZ));
        /*  DEBUGF("FreeBSD mask is 0x%x\n",mask); */
    }
#endif

#if defined(__riscos__) && !defined(ABORTFP)
    /* Disable FPE's under RISC OS */
    /* if bit is set, we disable trapping that error! */
    /*   _FPE_IVO : invalid operation */
    /*   _FPE_DVZ : divide by zero */
    /*   _FPE_OFL : overflow */
    /*   _FPE_UFL : underflow */
    /*   _FPE_INX : inexact */
    DisableFPETraps(_FPE_IVO | _FPE_DVZ | _FPE_OFL);
#endif

    /*
     *  Debugging stuff
     *  The default is to ignore FPE's, unless compiled with -DABORTFP
     *  so add code below to ENABLE FPE's.
     */

#if defined(ABORTFP)
#if defined(_MSC_VER)
    {
#include <float.h>
        unsigned int mask;
        mask = _controlfp(0, 0);
        mask &= ~(_EM_OVERFLOW | _EM_UNDERFLOW | _EM_ZERODIVIDE | _EM_INVALID);
        mask = _controlfp(mask, _MCW_EM);
    }
#elif defined(__CYGWIN__)
#  define LAME_X87_GETCW(cw) __asm__ ("fnstcw %0" : "=m" (*&cw))
#  define LAME_X87_SETCW(cw) __asm__ ("fldcw %0" : : "m" (*&cw))

/* x87 control word: the mask bits of three exceptions */
#  define LAME_X87_MASK_OVERFLOW    0x00000008
#  define LAME_X87_MASK_ZERODIVIDE  0x00000004
#  define LAME_X87_MASK_INVALID     0x00000001
    {
        unsigned int mask;
        LAME_X87_GETCW(mask);
        /* Set the FPU control word to abort on most FPEs */
        mask &= ~(LAME_X87_MASK_OVERFLOW | LAME_X87_MASK_ZERODIVIDE | LAME_X87_MASK_INVALID);
        LAME_X87_SETCW(mask);
    }
# elif defined(HAVE_FENV_H) && defined(HAVE_FEENABLEEXCEPT)
    {
        /*
         * feenableexcept() reaches the systems the x87 path below cannot -
         * the BSDs, and Linux on anything without an x87. Whether anything
         * then traps is the hardware's business: an AArch64 implementation
         * need not offer trapping exceptions, and there the call succeeds
         * and execution continues past the division.
         */
        (void) feenableexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW);
    }
# elif defined(HAVE_FPU_CONTROL_H)
    {

#  include <fpu_control.h>
/*
 * <fpu_control.h> exists wherever the C library is glibc - Android and musl
 * say __linux__ and do not have it. The mask names below are the x87 ones,
 * and glibc ships the header without them on architectures that have no x87,
 * so the body is compiled only where they turn up.
 */
#  if defined(_FPU_MASK_IM) && defined(_FPU_MASK_ZM) && defined(_FPU_MASK_OM)
#   ifdef _FPU_GETCW
#    define LAME_X87_GETCW(cw) _FPU_GETCW(cw)
#   else
#    define LAME_X87_GETCW(cw) __asm__ ("fnstcw %0" : "=m" (*&cw))
#   endif
#   ifdef _FPU_SETCW
#    define LAME_X87_SETCW(cw) _FPU_SETCW(cw)
#   else
#    define LAME_X87_SETCW(cw) __asm__ ("fldcw %0" : : "m" (*&cw))
#   endif

        /*
         * Set the mask to abort on most FPE's
         * if bit is set, we _mask_ SIGFPE on that error!
         *  mask &= ~( _FPU_MASK_IM | _FPU_MASK_ZM | _FPU_MASK_OM | _FPU_MASK_UM );
         */

        unsigned int mask;
        LAME_X87_GETCW(mask);
        mask &= ~(_FPU_MASK_IM | _FPU_MASK_ZM | _FPU_MASK_OM);
        LAME_X87_SETCW(mask);
#  endif
    }
#endif
#endif /* ABORTFP */
}





#ifdef USE_FAST_LOG
/***********************************************************************
 *
 * Fast Log Approximation for log2, used to approximate every other log
 * (log10 and log)
 * maximum absolute error for log10 is around 10-6
 * maximum *relative* error can be high when x is almost 1 because error/log10(x) tends toward x/e
 *
 * use it if typical RESULT values are > 1e-5 (for example if x>1.00001 or x<0.99999)
 * or if the relative precision in the domain around 1 is not important (result in 1 is exact and 0)
 *
 ***********************************************************************/


#define LOG2_SIZE       (512)
#define LOG2_SIZE_L2    (9)

static ieee754_float32_t log_table[LOG2_SIZE + 1];



void
init_log_table(void)
{
    int     j;
    static int init = 0;

    /* Range for log2(x) over [1,2[ is [0,1[ */
    assert((1 << LOG2_SIZE_L2) == LOG2_SIZE);

    if (!init) {
        for (j = 0; j < LOG2_SIZE + 1; j++)
            log_table[j] = log(1.0f + j / (ieee754_float32_t) LOG2_SIZE) / log(2.0f);
    }
    init = 1;
}



ieee754_float32_t
fast_log2(ieee754_float32_t x)
{
    ieee754_float32_t log2val, partial;
    union {
        ieee754_float32_t f;
        int     i;
    } fi;
    int     mantisse;
    fi.f = x;
    mantisse = fi.i & 0x7fffff;
    log2val = ((fi.i >> 23) & 0xFF) - 0x7f;
    partial = (mantisse & ((1 << (23 - LOG2_SIZE_L2)) - 1));
    partial *= 1.0f / ((1 << (23 - LOG2_SIZE_L2)));


    mantisse >>= (23 - LOG2_SIZE_L2);

    /* log2val += log_table[mantisse];  without interpolation the results are not good */
    log2val += log_table[mantisse] * (1.0f - partial) + log_table[mantisse + 1] * partial;

    return log2val;
}

#else /* Don't use FAST_LOG */


void
init_log_table(void)
{
}


#endif

/* end of util.c */
