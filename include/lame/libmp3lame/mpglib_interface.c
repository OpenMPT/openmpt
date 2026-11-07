/* -*- mode: C; mode: fold -*- */
/*
 *      LAME MP3 encoding engine
 *
 *      Copyright (c) 1999-2000 Mark Taylor
 *      Copyright (c) 2003 Olcios
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

/* $Id$ */

/*!
  \file   mpglib_interface.c
  \brief  The decoding side of the public API.

  The group \ref api_decoding describes these functions as a whole, and the
  decoder.
*/

/* Every function below is part of that interface, so the group is opened once
   here rather than named on each of them. Text inside the \addtogroup block
   would be appended to the group's description, so keep this comment out of
   it. */
/*! \addtogroup api_decoding
    @{ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define hip_global_struct mpstr_tag

#ifdef HAVE_MPG123
/* libmpg123 */
#include <mpg123.h>
#ifndef MPG123_API_VERSION
#error "Seems like you got the wrong mpg123 header. No MPG123_API_VERSION defined."
#endif
#if (MPG123_API_VERSION < 45)
#error "Need mpg123 API >= 45."
#endif

#endif /* HAVE_MPG123 */

/* for mpstr_tag */
#include "mpglib/mpglib.h"

#include "lame.h"
#include "machine.h"
#include "encoder.h"

/* for plotting_data */
#ifndef NOANALYSIS
#include "lame-analysis.h"
#endif

#include "util.h"
#include "obsolete_api.h"


/*! Shut the old global decoder down. */
/*!
  \deprecated This function does nothing. Each caller creates its own decoder
  with \c hip_decode_init() and releases it with \c hip_decode_exit(). lame.h
  does not declare this function. The library still exports it, so that
  programs built against an older release still link.

  \return always 0.
*/
int
lame_decode_exit(void)
{
    return 0;
}


/*! Start the old global decoder. */
/*!
  \deprecated This function does nothing. Use \c hip_decode_init(), which
  returns the decoder instance that the other decoding functions need. This
  function returns 0 without creating anything. So a program that only calls
  it and then decodes gets an error from every later call.

  \return always 0.
*/
int
lame_decode_init(void)
{
    return 0;
}




/*! Decode one frame through the old global decoder, with delay and padding. */
/*!
  \deprecated This function does nothing. Use \c hip_decode1_headersB(), which
  takes a decoder instance. **It does not decode.** It returns -1 without
  reading its arguments, so a caller that ignores the result reads
  uninitialized output buffers.

  \return always -1.
*/
int
lame_decode1_headersB(LAME_UNUSED unsigned char *buffer,
                      LAME_UNUSED int len,
                      LAME_UNUSED short pcm_l[], LAME_UNUSED short pcm_r[],
                      LAME_UNUSED mp3data_struct *mp3data,
                      LAME_UNUSED int *enc_delay, LAME_UNUSED int *enc_padding)
{
    return -1;
}





/*! Decode one frame through the old global decoder, with header data. */
/*!
  \deprecated This function does nothing, as \c lame_decode1_headersB(). Use
  \c hip_decode1_headers().
  \return always -1.
*/
int
lame_decode1_headers(LAME_UNUSED unsigned char *buffer,
                     LAME_UNUSED int len, LAME_UNUSED short pcm_l[],
                     LAME_UNUSED short pcm_r[], LAME_UNUSED mp3data_struct *mp3data)
{
    return -1;
}


/*! Decode one frame through the old global decoder. */
/*!
  \deprecated This function does nothing, as \c lame_decode1_headersB(). Use
  \c hip_decode1().
  \return always -1.
*/
int
lame_decode1(LAME_UNUSED unsigned char *buffer, LAME_UNUSED int len,
             LAME_UNUSED short pcm_l[], LAME_UNUSED short pcm_r[])
{
    return -1;
}


/*! Decode through the old global decoder, with header data. */
/*!
  \deprecated This function does nothing, as \c lame_decode1_headersB(). Use
  \c hip_decode_headers().
  \return always -1.
*/
int
lame_decode_headers(LAME_UNUSED unsigned char *buffer,
                    LAME_UNUSED int len, LAME_UNUSED short pcm_l[],
                    LAME_UNUSED short pcm_r[], LAME_UNUSED mp3data_struct *mp3data)
{
    return -1;
}


/*! Decode through the old global decoder. */
/*!
  \deprecated This function does nothing, as \c lame_decode1_headersB(). Use
  \c hip_decode().
  \return always -1.
*/
int
lame_decode(LAME_UNUSED unsigned char *buffer, LAME_UNUSED int len,
            LAME_UNUSED short pcm_l[], LAME_UNUSED short pcm_r[])
{
    return -1;
}




/**
 * \internal
 * \brief Creates a decoder for hip_decode_init() and hip_decode_init_gapless().
 * \param gapless  non-zero to let libmpg123 remove the encoder delay and
 *                 padding, zero to leave them in the output.
 * \return the decoder instance, or NULL as hip_decode_init() describes.
 */
static hip_t
hip_new(LAME_UNUSED int gapless)
{
    hip_t hip = lame_calloc(hip_global_flags, 1);
    if(!hip)
        return hip;
#ifdef HAVE_MPG123
    mpg123_init();
    hip->mh = mpg123_new(NULL, NULL);
    /* Could allocate on demand only. */
    memset(&hip->mi, 0, sizeof(hip->mi));
    /* Gapless decoding is on by default in libmpg123; set it either way. The
       other entry points report the encoder delay and padding, so a caller
       without it can remove them itself. */
    mpg123_param(hip->mh, gapless ? MPG123_ADD_FLAGS : MPG123_REMOVE_FLAGS, MPG123_GAPLESS, 0.);
    /* We are going to feed buffers. */
    if(mpg123_open_feed(hip->mh) != MPG123_OK)
    {
        mpg123_delete(hip->mh);
        free(hip);
        hip = NULL;
    }
#else
    /* Nothing here can decode, so refuse to hand out a decoder at all rather
       than one that fails on every later call. */
    free(hip);
    hip = NULL;
#endif
    return hip;
}

/*! Create a decoder. */
/*!
  The first call of the decoding API. Pass the returned decoder instance to
  every other \c hip_ function, and release it with \c hip_decode_exit().

  In a library built without libmpg123, **this fails** and returns NULL. Check
  the result to find out whether decoding is available. A program that does
  not check it gets an error from every decode call.

  \since LAME 4.1. Earlier versions return a decoder instance also in a
         library that cannot decode. So a program that must also work with an
         older libmp3lame cannot rely on this check alone.

  \return the decoder instance, or NULL if decoding is not available or
          memory allocation fails. A caller cannot tell these two cases apart.
*/
hip_t hip_decode_init(void)
{
    return hip_new(0);
}

/*! Create a decoder that trims the encoder's padding itself. */
/*!
  As \c hip_decode_init(), but the decoder uses the gapless information in the
  file. It removes the silence that the encoder added at the start and at the
  end. The output then has as many samples as the input of the encoder. With
  a plain decoder, the caller must remove this silence. For that, the other
  entry points return the encoder delay and padding.

  Like \c hip_decode_init(), this **returns NULL in a library built without
  libmpg123**, because libmpg123 does the trimming.

  \return the decoder instance, or NULL if gapless decoding is not available
          or memory allocation fails. A caller cannot tell these two cases
          apart.
*/
hip_t hip_decode_init_gapless(void)
{
    return hip_new(1);
}



/*! Destroy a decoder. */
/*!
  Frees everything the decoder instance owns. NULL is accepted and ignored, so
  a failed \c hip_decode_init() needs no special case.

  \param hip the decoder instance, or NULL.
  \return always 0. There is no failure to report.
*/
int hip_decode_exit(hip_t hip)
{
    if(hip) {
#ifdef HAVE_MPG123
        mpg123_delete(hip->mh); /* Closes implicitly. */
        /* No mpg123_exit(), will be deprecated anyway. */
#endif
        free(hip);
    }
    return 0;
}

#ifdef HAVE_MPG123
/* One decoding routine to cover all API cases. Any output pointer except pcm_l
   and pcm_r, which are always required to be able to store full MPEG frame
   (1152 samples), can be NULL if you are not really interested in it.
   This always works on one whole MPEG frame, even if sample count can be
   smaller after gapless handling. TODO: Optionally turn on gapless decoding?
   If not, the decoder delay also needs to be communicated.
   Or do we just assume 529 samples? */

/**
 * @internal
 * @brief The factor between the normalized full scale of libmpg123, which is
 *        1, and the sample_t scale of LAME, whose full scale is 32768.
 */
#define SAMPLE_T_FULL_SCALE 32768.0

/*! Return the encoder delay and padding from the LAME tag of the stream. */
/*!
  \internal
  Called on every return of \c hip123_decode1() that is not an error.

  If mpg123_getstate() fails, it does not set val. So its return value is
  checked. Without this check, the caller would get an uninitialized value and
  trim that many samples. On failure, this function writes -1. Callers read -1
  as "not available". It also writes -1 for a stream that has no delay and
  padding information.

  \param hip          the decoder, not NULL.
  \param enc_delay    receives the delay, or NULL.
  \param enc_padding  receives the padding, or NULL.
*/
static void
report_delay_padding(hip_t hip, int *enc_delay, int *enc_padding)
{
    if(enc_delay) {
        long val;
        if(MPG123_OK != mpg123_getstate(hip->mh, MPG123_ENC_DELAY, &val, NULL))
            val = -1;
        *enc_delay = val > INT_MAX ? -1 : val;
    }
    if(enc_padding) {
        long val;
        if(MPG123_OK != mpg123_getstate(hip->mh, MPG123_ENC_PADDING, &val, NULL))
            val = -1;
        *enc_padding = val > INT_MAX ? -1 : val;
    }
}

/**
 * @internal
 * @brief The sample format hip123_decode1() writes.
 */
enum hip_output {
    HIP_OUT_S16,     /**< 16-bit samples, as short */
    HIP_OUT_SAMPLE_T /**< sample_t on LAME's scale, not clipped */
};

static int hip123_decode1( hip_t hip, unsigned char *buffer, size_t len,
    void *pcm_l, void *pcm_r,
    int *enc_delay, int *enc_padding,
    mp3data_struct *mp3data,
    enum hip_output output)
{
    int ret;
    unsigned char *mpg123buf;
    size_t mpg123fill;
    long rate;
    int channels;
    int encoding;
    int change_format;
    int samples = 0;
    int want_enc = output == HIP_OUT_SAMPLE_T ? MPG123_ENC_FLOAT_32 : MPG123_ENC_SIGNED_16;

    /* Every decoding entry point arrives here, and hip_decode_init() hands back
       NULL where there is no decoder to give - so a caller who did not check
       that result reaches this with nothing. The answer they are promised is an
       error from the decode; without this it is a dereference of hip->mh below.
       hip_decode1_headersB() asks the same question at its own level, which is
       where it has to be asked in a build with no libmpg123, since none of this
       is compiled then. */
    if(!hip)
        return -1;

    if(MPG123_OK != mpg123_feed(hip->mh, buffer, len))
        return -1;
    ret = mpg123_getformat(hip->mh, &rate, &channels, &encoding);
    switch(ret) {
        case MPG123_NEED_MORE:
            /* no frame header seen yet: nothing to describe */
            if(mp3data)
                memset(mp3data, 0, sizeof(mp3data_struct));
            report_delay_padding(hip, enc_delay, enc_padding);
            return 0;
        case MPG123_OK:
            change_format = encoding != want_enc;
        break;
        default:
            return -1;
    }

    if(change_format)
    {
        mpg123_format_none(hip->mh);
        mpg123_format2(hip->mh, 0, MPG123_MONO|MPG123_STEREO, want_enc);
        /* This triggers renegotiation of output format on next decode. */
        mpg123_decoder(hip->mh, NULL);
    }

    /* Now decode for real. */
    mpg123fill = 0; /* Still zero in case of error/need more. */
    ret = mpg123_decode_frame(hip->mh, NULL, &mpg123buf, &mpg123fill);
    /* A second time if we just got notified of new format. */
    if(!mpg123fill && ret == MPG123_NEW_FORMAT)
    {
        mpg123_getformat(hip->mh, &rate, &channels, &encoding);
        ret = mpg123_decode_frame(hip->mh, NULL, &mpg123buf, &mpg123fill);
        /* True paranoia would check the encoding again. */
    }
    if(ret == MPG123_ERR)
        return -1;
    /* MPG123_NEED_MORE and MPG123_DONE (not happening here, though)
        both result in mpg123fill==0, so return 0 here, which is what fits. */
    {
        size_t const bytes_per_sample =
            (output == HIP_OUT_SAMPLE_T ? sizeof(float) : sizeof(short)) * (size_t) channels;
        size_t const decoded = mpg123fill / bytes_per_sample;
        /* the count is returned in an int and indexes the caller's buffers, so
           a frame yielding more than that can express is rejected rather than
           demultiplexed against a wrapped length */
        if (decoded > (size_t) INT_MAX)
            return -1;
        samples = (int) decoded;
    }
    /* Now demultilex the data in mpg123buf into pcm_l and pcm_r. */
    if(mpg123fill && mpg123buf)
    {
        if(output == HIP_OUT_SAMPLE_T)
        {
            /* Lame's sample_t could be wider than 32 bit, right? */
            sample_t *spcm_l = (sample_t*)pcm_l;
            sample_t *spcm_r = (sample_t*)pcm_r;
            float    *srcbuf = (float*)mpg123buf;
            int i;

            /* Into LAME's scale on the way in. Samples beyond full scale stay
               beyond it - that is what makes clipping detectable. */
            if(channels == 2) {
                for(i=0; i<samples; ++i) {
                    spcm_l[i] = *srcbuf++ * SAMPLE_T_FULL_SCALE;
                    spcm_r[i] = *srcbuf++ * SAMPLE_T_FULL_SCALE;
                }
            }
            else
                for(i=0; i<samples; ++i)
                    spcm_l[i] = *srcbuf++ * SAMPLE_T_FULL_SCALE;
        }
        else
        {
            /* It's all shorts. */
            short *spcm_l = (short*)pcm_l;
            short *spcm_r = (short*)pcm_r;
            short *srcbuf = (short*)mpg123buf;
            int i;

            if(channels == 2) {
                for(i=0; i<samples; ++i) {
                    spcm_l[i] = *srcbuf++;
                    spcm_r[i] = *srcbuf++;
                }
            }
            else
                memcpy(pcm_l, mpg123buf, sizeof(short)*samples);
        }
    }

    /* If we arrive here, there was some successful parsing of the stream at
       least, so that meaningful info is available. */
    if(mp3data) {
        struct mpg123_frameinfo fi;
        memset(mp3data, 0, sizeof(mp3data_struct));
        /* Re-using last returns from getformat() before. */
        if(MPG123_OK == mpg123_info(hip->mh, &fi)) {
            mp3data->header_parsed = 1;
            mp3data->stereo = channels; /* Channel count correct? Or is dual mono different? */
            mp3data->samplerate = rate;
            mp3data->mode = fi.mode;
            mp3data->mode_ext = fi.mode_ext;
            mp3data->framesize = mpg123_spf(hip->mh);
            mp3data->bitrate = fi.bitrate;
        }
    }
    report_delay_padding(hip, enc_delay, enc_padding);
    if(hip->pinfo)
        hip_finish_pinfo(hip);
    return samples;
}
#endif


int
hip_decode1_unclipped(hip_t hip, LAME_UNUSED unsigned char *buffer, LAME_UNUSED size_t len,
                      LAME_UNUSED sample_t pcm_l[], LAME_UNUSED sample_t pcm_r[])
{
    if (hip) {
#ifdef HAVE_MPG123
        return hip123_decode1( hip, buffer, len, pcm_l, pcm_r,
            NULL, NULL, NULL, HIP_OUT_SAMPLE_T );
#endif
    }
    return -1;
}

/*! Decode at most one frame, and report what the frame header said. */
/*!
  The same as \c hip_decode1(), and it also fills \a mp3data from the frame
  header: sample rate, number of channels, bitrate and frame size. So a caller
  learns the format of a stream that it did not encode itself. The fields have
  a meaning only when \c header_parsed is set.

  \param hip      the decoder instance.
  \param buffer   the encoded bytes to pass in.
  \param len      the number of bytes in \a buffer. 0 to empty what the
                  decoder already has.
  \param pcm_l    receives the left channel. It must have room for a full
                  frame, 1152 samples, whatever the return value is.
  \param pcm_r    receives the right channel, with the same size.
  \param mp3data  receives the frame description on every call that is not an
                  error. Its header_parsed is 0 until a frame header was read.
  \return the number of samples per channel written, 0 if more input is
          needed first, or -1 on an error, also for a NULL \a hip.
*/
int
hip_decode1_headers(hip_t hip, unsigned char *buffer,
                     size_t len, short pcm_l[], short pcm_r[], mp3data_struct * mp3data)
{
#ifdef HAVE_MPG123
    return hip123_decode1( hip, buffer, len, pcm_l, pcm_r,
        NULL, NULL, mp3data, HIP_OUT_S16 );
#else
    int     enc_delay, enc_padding;
    return hip_decode1_headersB(hip, buffer, len, pcm_l, pcm_r, mp3data, &enc_delay, &enc_padding);
#endif
}


/*! Decode at most one frame. */
/*!
  Passes \a len bytes to the decoder and returns the samples this produced. It
  returns **at most one frame** of samples. A caller with a large input buffer
  calls again with a length of 0 until the result is 0. \c hip_decode() does
  this loop itself.

  A return of 0 is normal at the start of a stream. MP3 frames depend on the
  frames before them, so the first call or two return nothing.

  \param hip     the decoder instance.
  \param buffer  the encoded bytes to pass in.
  \param len     the number of bytes in \a buffer. 0 to empty what the decoder
                 already has.
  \param pcm_l   receives the left channel. It must have room for a full
                 frame, 1152 samples.
  \param pcm_r   receives the right channel, with the same size. It is written
                 only for a stereo stream.
  \return the number of samples per channel written, 0 if more input is
          needed first, or -1 on an error, also for a NULL \a hip.
*/
int
hip_decode1(hip_t hip, unsigned char *buffer, size_t len, short pcm_l[], short pcm_r[])
{
#ifdef HAVE_MPG123
    return hip123_decode1( hip, buffer, len, pcm_l, pcm_r,
        NULL, NULL, NULL, HIP_OUT_S16 );
#else
    mp3data_struct mp3data;
    return hip_decode1_headers(hip, buffer, len, pcm_l, pcm_r, &mp3data);
#endif
}


/*! Decode everything the input yields, and report the frame header. */
/*!
  \c hip_decode(), and it also fills in the frame description. See
  \c hip_decode1_headers() for the content of \a mp3data. This function
  decodes several frames, so \a mp3data describes the **last** one. This
  matters for a stream whose frames are not all alike.

  \param hip      the decoder instance.
  \param buffer   the encoded bytes to pass in.
  \param len      the number of bytes in \a buffer.
  \param pcm_l    receives the left channel.
  \param pcm_r    receives the right channel.
  \param mp3data  receives the description of the last frame decoded.
  \return the total number of samples per channel written, 0 if more input is
          needed, or -1 on an error, also for a NULL \a hip.
*/
int
hip_decode_headers(hip_t hip, unsigned char *buffer,
                    size_t len, short pcm_l[], short pcm_r[], mp3data_struct * mp3data)
{
    int     ret;
    int     totsize = 0;     /* number of decoded samples per channel */

    for (;;) {
        switch (ret = hip_decode1_headers(hip, buffer, len, pcm_l + totsize, pcm_r + totsize, mp3data)) {
        case -1:
            return ret;
        case 0:
            return totsize;
        default:
            totsize += ret;
            len = 0;    /* future calls to decodeMP3 are just to flush buffers */
            break;
        }
    }
}


/*! Decode everything the input yields. */
/*!
  Calls \c hip_decode1() until the decoder has nothing more to return. So one
  call turns a buffer of MP3 data into all the samples it contains.

  The output buffers **must have room for every frame that the call
  completes**, also frames from the input of earlier calls. That number does
  not follow from \a len, because free-format frames can be a few bytes long.
  The function cannot check the buffer size and gives no warning. A caller
  with buffers of a fixed size calls \c hip_decode1() in a loop instead. It
  writes at most one frame per call.

  An application that decodes MP3 for its own use should use libmpg123. Its
  API takes the size of the output.

  \param hip     the decoder instance.
  \param buffer  the encoded bytes to pass in.
  \param len     the number of bytes in \a buffer.
  \param pcm_l   receives the left channel, with room as above.
  \param pcm_r   receives the right channel, with the same size.
  \return the total number of samples per channel written, 0 if more input is
          needed, or -1 on an error, also for a NULL \a hip.
*/
int
hip_decode(hip_t hip, unsigned char *buffer, size_t len, short pcm_l[], short pcm_r[])
{
    mp3data_struct mp3data;
    return hip_decode_headers(hip, buffer, len, pcm_l, pcm_r, &mp3data);
}


/*! Decode at most one frame, and report the encoder's delay and padding. */
/*!
  \c hip_decode1_headers(), plus the two values that undo what the encoder
  added: the samples before the audio and the samples after it. Removing them
  from the decoded stream restores the original length. Gapless playback of a
  sequence of files needs this.

  \c hip_decode_init_gapless() removes them inside the decoder. Use it unless
  the caller needs the numbers.

  \param hip          the decoder instance.
  \param buffer       the encoded bytes to pass in.
  \param len          the number of bytes in \a buffer.
  \param pcm_l        receives the left channel. It must have room for a full
                      frame.
  \param pcm_r        receives the right channel, with the same size.
  \param mp3data      receives the frame description.
  \param enc_delay    receives the encoder delay in samples, or **-1 if the
                      value is not available**. The LAME tag contains the two
                      values. So a stream without a LAME tag returns -1 for
                      both, and there is nothing to remove. This is normal,
                      not an error. A value too large for an \c int also
                      returns -1, because a caller cannot use it.
  \param enc_padding  receives the padding at the end in samples, in the same
                      way.
  \return the number of samples per channel written, 0 if more input is
          needed first, or -1 on an error, also for a NULL \a hip.
*/
int
hip_decode1_headersB(hip_t hip, LAME_UNUSED unsigned char *buffer,
                      LAME_UNUSED size_t len,
                      LAME_UNUSED short pcm_l[], LAME_UNUSED short pcm_r[],
                      LAME_UNUSED mp3data_struct * mp3data,
                      LAME_UNUSED int *enc_delay, LAME_UNUSED int *enc_padding)
{
    if (hip) {
#ifdef HAVE_MPG123
        return hip123_decode1( hip, buffer, len, pcm_l, pcm_r,
            enc_delay, enc_padding, mp3data, HIP_OUT_S16);
#endif
    }
    return -1;
}


/*! Install the block the decoder describes each frame into. */
/*!
  A frontend that plots what the decoder saw gives the decoder a block to
  fill. The frame analyzer in this tree does this. After this call, the
  decoder fills \a pinfo while it decodes, and \c hip_finish_pinfo() completes
  the last frame at the end of the input.

  Call it before decoding starts. A block set later describes only the frames
  decoded after it. The block belongs to the caller and must exist as long as
  the decoder, because the decoder keeps the pointer, not a copy.

  \c lame.h declares \c plotting_data but does not define it, because its
  layout is internal and can change. A caller that only uses the hooks passes
  the pointer and never needs the fields. A caller that reads them uses the
  internal header, which is not part of the API.

  \param hip    the decoder instance, or \c NULL, which does nothing.
  \param pinfo  the block to fill, or \c NULL to stop filling one.
*/
void hip_set_pinfo(hip_t hip, plotting_data* pinfo)
{
    if (hip) {
        hip->pinfo = pinfo;
#ifdef HAVE_MPG123
        mpg123_set_moreinfo(hip->mh, &hip->mi);
#endif
    }
}

/*! Complete the last frame's entry in the installed block. */
/*!
  The decoder describes a frame when it has read the next frame. So at the end
  of the input, the last frame is not complete. This function completes it. A
  frontend calls it after its last \c hip_decode() call.

  It does nothing if no block was set with \c hip_set_pinfo().

  \param hip  the decoder instance, or \c NULL, which does nothing.
*/
void hip_finish_pinfo(LAME_UNUSED hip_t hip)
{
#ifndef NOANALYSIS
#ifdef HAVE_MPG123
    struct mpg123_frameinfo fi;
    plotting_data *pinfo;
    if(!hip || !hip->pinfo)
        return;
    pinfo = hip->pinfo;

    /* TODO: convert to pointers to avoid copies. Allocation should be
       on mpg123 side (in form of the struct definition), as that is
       the writing side. */
    memcpy(pinfo->mpg123xr, hip->mi.xr, sizeof(pinfo->mpg123xr));
    memcpy(pinfo->sfb, hip->mi.sfb, sizeof(pinfo->sfb));
    memcpy(pinfo->sfb_s, hip->mi.sfb_s, sizeof(pinfo->sfb_s));
    memcpy(pinfo->qss, hip->mi.qss, sizeof(pinfo->qss));
    memcpy(pinfo->big_values, hip->mi.big_values, sizeof(pinfo->big_values));
    memcpy(pinfo->sub_gain, hip->mi.sub_gain, sizeof(pinfo->sub_gain));
    memcpy(pinfo->scalefac_scale, hip->mi.scalefac_scale, sizeof(pinfo->scalefac_scale));
    memcpy(pinfo->preflag, hip->mi.preflag, sizeof(pinfo->preflag));
    memcpy(pinfo->mpg123blocktype, hip->mi.blocktype, sizeof(pinfo->mpg123blocktype));
    memcpy(pinfo->mixed, hip->mi.mixed, sizeof(pinfo->mixed));
    memcpy(pinfo->mainbits, hip->mi.mainbits, sizeof(pinfo->mainbits));
    memcpy(pinfo->sfbits, hip->mi.sfbits, sizeof(pinfo->sfbits));
    memcpy(pinfo->scfsi, hip->mi.scfsi, sizeof(pinfo->scfsi));
    pinfo->maindata = hip->mi.maindata;
    pinfo->padding  = hip->mi.padding;
    if(MPG123_OK == mpg123_info(hip->mh, &fi)) {
        pinfo->js = (fi.mode == MPG123_M_JOINT);
        pinfo->stereo = fi.mode == MPG123_M_MONO ? 1 : 2;
        pinfo->crc = fi.flags & MPG123_CRC ? 1 : 0;
        pinfo->emph = fi.emphasis;
        pinfo->sampfreq = fi.rate;
        pinfo->bitrate = fi.bitrate;
        pinfo->ms_stereo = pinfo->js ? (fi.mode_ext & 0x2)>>1 : 0;
        pinfo->i_stereo  = pinfo->js ? (fi.mode_ext & 0x1)    : 0;
    }
#endif
#endif
}

/*! Route the decoder's error messages. */
/*!
  The decoder form of \c lame_set_errorf(), and **it does nothing**. It accepts
  the callback and discards it. libmpg123 does the decoding, and its messages
  are not passed on. So a caller who wants to know why a decode failed has
  only the -1.

  It exists because the report functions of the encoder have decoder
  functions with matching names. Removing it would break programs that set
  all six.

  \param hip   ignored.
  \param func  ignored.
*/
void hip_set_errorf(LAME_UNUSED hip_t hip, LAME_UNUSED lame_report_function func)
{
#ifdef HAVE_MPG123
    /* TODO: implement something */
#endif
}

/*! Route the decoder's debug messages. */
/*!
  Does nothing; see \c hip_set_errorf().
  \param hip   ignored.
  \param func  ignored.
*/
void hip_set_debugf(LAME_UNUSED hip_t hip, LAME_UNUSED lame_report_function func)
{
#ifdef HAVE_MPG123
    /* TODO: implement something */
#endif
}

/*! Route the decoder's informational messages. */
/*!
  Does nothing; see \c hip_set_errorf().
  \param hip   ignored.
  \param func  ignored.
*/
void hip_set_msgf  (LAME_UNUSED hip_t hip, LAME_UNUSED lame_report_function func)
{
#ifdef HAVE_MPG123
    /* TODO: implement something */
#endif
}

/*! @} */

/* end of mpglib_interface.c */
