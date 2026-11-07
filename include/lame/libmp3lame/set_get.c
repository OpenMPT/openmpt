/* -*- mode: C; mode: fold -*- */
/*
 * set/get functions for lame_global_flags
 *
 * Copyright (c) 2001-2005 Alexander Leidinger
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
  \file   set_get.c
  \brief  The parameter interface of the public API.

  What holds for these functions as a whole, and the shape of the interface
  they make up, is described with the group: \ref api_settings.
*/

/* Every function below is part of that interface, so the group is opened once
   here rather than named on each of them. Text inside the \addtogroup block
   would be appended to the group's description, so keep this comment out of
   it. */
/*! \addtogroup api_settings
    @{ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "bitstream.h"  /* because of compute_flushbits */

#include "set_get.h"
#include "obsolete_api.h"
#include "lame_global_flags.h"

/*
 * input stream description
 */


/*! Tell the encoder how many samples the input has. */
/*!
  LAME uses this value only to estimate the total number of frames.
  \c lame_get_totalframes() returns this estimate, and the length field of the
  LAME tag is written from it. It does not limit the encode. The input can have
  more or fewer samples than this value. Only the estimate is then wrong.

  The default is 2^32-1, which means "length not known". Keep the default for
  a stream whose length is not known in advance.

  \param gfp          the encoder instance.
  \param num_samples  number of samples per channel in the input.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_num_samples(lame_global_flags * gfp, unsigned long num_samples)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 2^32-1 */
        gfp->num_samples = num_samples;
        return 0;
    }
    return -1;
}

/*! Get the number of samples the input was announced to have. */
/*!
  \param gfp the encoder instance.
  \return the value that was set, or 2^32-1 ("not known"). 0 if the instance
          is not usable.
*/
unsigned long
lame_get_num_samples(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->num_samples;
    }
    return 0;
}


/*! Set the sample rate of the input, in Hz. */
/*!
  This is one of the two settings that describe the input. The other one is
  \c lame_set_num_channels(). Both have defaults: 44100 Hz and 2 channels. So
  an encoder that is never told about its input initializes without an error
  and encodes the input as CD audio. Always set both.

  This is the sample rate of the samples passed to \c lame_encode_buffer(). It
  is not always the sample rate written into the stream. If it differs from the
  output sample rate (\c lame_set_out_samplerate()), LAME resamples.

  The MP3 stream does not store the input sample rate. Each frame stores the
  output sample rate, which must be one of the rates in the table under
  \c lame_set_out_samplerate(). A decoder reports this output rate. The file
  has no field for the input rate, so the input rate cannot be read back from
  the file. LAME resamples input with a high sample rate down to one of these
  rates.

  This function sets no upper limit. It accepts rates far above the output
  rates, and LAME resamples them.

  \param gfp             the encoder instance.
  \param in_samplerate   input sample rate in Hz. This function accepts any
                         positive value. \c lame_init_params() checks whether
                         it can be encoded.
  \return 0 on success. -1 if \a in_samplerate is below 1, or if the instance
          is not usable.
*/
int
lame_set_in_samplerate(lame_global_flags * gfp, int in_samplerate)
{
    if (is_lame_global_flags_valid(gfp)) {
        if (in_samplerate < 1)
            return -1;
        /* input sample rate in Hz,  default = 44100 Hz */
        gfp->samplerate_in = in_samplerate;
        return 0;
    }
    return -1;
}

/*! Get the input sample rate, in Hz. */
/*!
  \param gfp the encoder instance.
  \return the input sample rate. 0 if the instance is not usable.
*/
int
lame_get_in_samplerate(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->samplerate_in;
    }
    return 0;
}


/*! Set the number of channels in the input. */
/*!
  This is one of the two settings that describe the input. The other one is
  \c lame_set_in_samplerate(). If you set neither, LAME assumes CD audio:
  44100 Hz and 2 channels. Always set both.

  This is the layout of the buffers passed to the encoder. It is not the
  channel mode written into the stream. \c lame_set_mode() sets that, and LAME
  can encode two-channel input as mono.

  \param gfp           the encoder instance.
  \param num_channels  1 or 2. LAME implements no multichannel extension of
                       MP3.
  \return 0 on success. -1 if \a num_channels is not 1 or 2, or if the
          instance is not usable.
*/
int
lame_set_num_channels(lame_global_flags * gfp, int num_channels)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 2 */
        if (2 < num_channels || 0 >= num_channels) {
            return -1;  /* we don't support more than 2 channels */
        }
        gfp->num_channels = num_channels;
        return 0;
    }
    return -1;
}

/*! Get the number of channels in the input. */
/*!
  \param gfp the encoder instance.
  \return 1 or 2. 0 if the instance is not usable.
*/
int
lame_get_num_channels(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->num_channels;
    }
    return 0;
}


/*! Scale every input sample by this factor before encoding. */
/*!
  LAME multiplies both channels by this factor. The per-channel factors of
  \c lame_set_scale_left() and \c lame_set_scale_right() apply as well. The
  default is 1, which means no change. The decoder does not use it.

  The scaling happens before the psychoacoustic model sees the signal. So this
  is not a volume control for the output. It changes what is encoded, and a
  large factor can clip the signal.

  \param gfp    the encoder instance.
  \param scale  the factor, a finite number from -4096 to 4096. 0 makes the
                input silent, and a negative value inverts it. If a sample is
                louder than 4096 times full scale after all factors, the
                encode call returns -14.
  \return 0 on success. -1 if the factor is NaN, infinite or larger than 4096,
          or if the instance is not usable. The setting does not change then.
*/
int
lame_set_scale(lame_global_flags * gfp, float scale)
{
    if (is_lame_global_flags_valid(gfp) && float_is_finite(scale)
        && scale <= MAX_INPUT_SCALE && scale >= -MAX_INPUT_SCALE) {
        /* default = 1 */
        gfp->scale = scale;
        return 0;
    }
    return -1;
}

/*! Get the overall input scaling factor. */
/*!
  \param gfp the encoder instance.
  \return the factor. 0 if the instance is not usable. 0 is also a valid
          value.
*/
float
lame_get_scale(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->scale;
    }
    return 0;
}


/*! Scale the left channel of the input by this factor before encoding. */
/*!
  The factor of \c lame_set_scale() applies as well, and the two are
  multiplied. The default is 1. The decoder does not use it, and it has no
  effect on mono input.

  \param gfp    the encoder instance.
  \param scale  the factor, as for \c lame_set_scale().
  \return as \c lame_set_scale().
*/
int
lame_set_scale_left(lame_global_flags * gfp, float scale)
{
    if (is_lame_global_flags_valid(gfp) && float_is_finite(scale)
        && scale <= MAX_INPUT_SCALE && scale >= -MAX_INPUT_SCALE) {
        /* default = 1 */
        gfp->scale_left = scale;
        return 0;
    }
    return -1;
}

/*! Get the left-channel input scaling factor. */
/*!
  \param gfp the encoder instance.
  \return the factor. 0 if the instance is not usable. 0 is also a valid
          value.
*/
float
lame_get_scale_left(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->scale_left;
    }
    return 0;
}


/*! Scale the right channel of the input by this factor before encoding. */
/*!
  The same as \c lame_set_scale_left(), for the right channel.

  \param gfp    the encoder instance.
  \param scale  the factor, as for \c lame_set_scale().
  \return as \c lame_set_scale().
*/
int
lame_set_scale_right(lame_global_flags * gfp, float scale)
{
    if (is_lame_global_flags_valid(gfp) && float_is_finite(scale)
        && scale <= MAX_INPUT_SCALE && scale >= -MAX_INPUT_SCALE) {
        /* default = 1 */
        gfp->scale_right = scale;
        return 0;
    }
    return -1;
}

/*! Get the right-channel input scaling factor. */
/*!
  \param gfp the encoder instance.
  \return the factor. 0 if the instance is not usable. 0 is also a valid
          value.
*/
float
lame_get_scale_right(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->scale_right;
    }
    return 0;
}


/*! Set the sample rate written into the MP3 stream, in Hz. */
/*!
  The default is 0. 0 means that LAME chooses the output sample rate in
  \c lame_init_params(), from the bitrate settings. This is correct for most
  uses. Set a rate only if the output must have a specific sample rate.

  If the value differs from \c lame_set_in_samplerate(), LAME resamples. Only
  the rates of the MP3 formats are accepted. The rate also selects the MPEG
  version:

  | Version  | Rates (kHz)    |
  |----------|----------------|
  | MPEG-1   | 32, 44.1, 48   |
  | MPEG-2   | 16, 22.05, 24  |
  | MPEG-2.5 | 8, 11.025, 12  |

  This function rejects a rate that is not in the table. It returns -1 at
  once, not at \c lame_init_params().
  The decoder does not use it.

  \param gfp              the encoder instance.
  \param out_samplerate   output sample rate in Hz, or 0 to let LAME choose.
  \return 0 on success. -1 if the format does not allow the rate, or if the
          instance is not usable.
*/
int
lame_set_out_samplerate(lame_global_flags * gfp, int out_samplerate)
{
    if (is_lame_global_flags_valid(gfp)) {
        if (out_samplerate != 0) {
            int     v=0;
            if (SmpFrqIndex(out_samplerate, &v) < 0)
                return -1;
        }
        gfp->samplerate_out = out_samplerate;
        return 0;
    }
    return -1;
}

/*! Get the output sample rate, in Hz. */
/*!
  Before \c lame_init_params(), this returns the value that was set. 0 then
  means "not chosen yet". After \c lame_init_params(), it returns the sample
  rate that the encoder uses.

  \param gfp the encoder instance.
  \return the sample rate. 0 if it is not chosen yet, or if the instance is
          not usable.
*/
int
lame_get_out_samplerate(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->samplerate_out;
    }
    return 0;
}




/*
 * general control parameters
 */

/*! Collect the per-frame data an MP3 frame analyzer displays. */
/*!
  The encoder then fills the \c plotting_data structure while it encodes. The
  \c mp3x analyzer reads this structure. It costs time and memory and does not
  help an ordinary encode. The default is off.

  \param gfp       the encoder instance.
  \param analysis  1 to collect, 0 not to.
  \return 0 on success. -1 if the value is not 0 or 1, or if the instance is
          not usable.
*/
int
lame_set_analysis(lame_global_flags * gfp, int analysis)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 0 */

        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 > analysis || 1 < analysis)
            return -1;
        gfp->analysis = analysis;
        return 0;
    }
    return -1;
}

/*! Get whether frame-analyzer data is being collected. */
/*!
  \param gfp the encoder instance.
  \return 1 if the data is collected, 0 if not. 0 also if the instance is not
          usable.
*/
int
lame_get_analysis(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(0 <= gfp->analysis && 1 >= gfp->analysis);
        return gfp->analysis;
    }
    return 0;
}


/*! Write the LAME tag frame at the start of the stream. */
/*!
  The LAME tag frame contains the seek table that a player uses for a VBR
  file, the length, the encoder delay and padding, and the ReplayGain values.
  Without it, a player cannot seek accurately in a VBR file and must guess its
  duration from the first frame.

  The default is on, in every mode. Turn it off only if the stream must not
  start with a frame that holds no audio.

  \param gfp           the encoder instance.
  \param bWriteVbrTag  1 to write the frame, 0 not to.
  \return 0 on success. -1 if the value is not 0 or 1, or if the instance is
          not usable.
*/
int
lame_set_bWriteVbrTag(lame_global_flags * gfp, int bWriteVbrTag)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 1 */

        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 > bWriteVbrTag || 1 < bWriteVbrTag)
            return -1;
        gfp->write_lame_tag = bWriteVbrTag;
        return 0;
    }
    return -1;
}

/*! Get whether the LAME tag frame will be written. */
/*!
  \param gfp the encoder instance.
  \return 1 if the frame is written, 0 if not. 0 also if the instance is not
          usable.
*/
int
lame_get_bWriteVbrTag(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(0 <= gfp->write_lame_tag && 1 >= gfp->write_lame_tag);
        return gfp->write_lame_tag;
    }
    return 0;
}



/*! Use this instance to decode rather than encode. */
/*!
  A flag mainly for the lame tool. It marks the run as an MP3-to-WAV decode,
  so the tool takes that path. In the library, it only stops
  \c lame_init_params() from creating the decoder for
  \c lame_set_decode_on_the_fly(). The decoder functions are the separate
  \c hip_* family.

  \param gfp          the encoder instance.
  \param decode_only  1 for decoding, 0 for encoding.
  \return 0 on success. -1 if the value is not 0 or 1, or if the instance is
          not usable.
*/
int
lame_set_decode_only(lame_global_flags * gfp, int decode_only)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 0 (disabled) */

        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 > decode_only || 1 < decode_only)
            return -1;
        gfp->decode_only = decode_only;
        return 0;
    }
    return -1;
}

/*! Get whether this instance is marked as decode-only. */
/*!
  \param gfp the encoder instance.
  \return 1 if decode-only is set, 0 if not. 0 also if the instance is not
          usable.
*/
int
lame_get_decode_only(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(0 <= gfp->decode_only && 1 >= gfp->decode_only);
        return gfp->decode_only;
    }
    return 0;
}



/*! Encode a Vorbis .ogg file. */
/*!
  \deprecated This function does nothing. LAME has no Vorbis encoder.
  lame.h does not declare this function. The library still exports it, so
  that programs built against an older release still link.

  \param gfp  ignored.
  \param ogg  ignored.
  \return always -1.
*/
int
lame_set_ogg(lame_global_flags * gfp, int ogg)
{
    (void) gfp;
    (void) ogg;
    return -1;
}

/*! Get the Vorbis .ogg setting. */
/*!
  \deprecated See \c lame_set_ogg().
  \param gfp  ignored.
  \return always 0.
*/
int
lame_get_ogg(const lame_global_flags * gfp)
{
    (void) gfp;
    return 0;
}


/*! Choose how much computing time the encoder spends, 0 (most) to 9 (least). */
/*!
  This is not the setting for the audio quality. The bitrate or
  \c lame_set_VBR_quality() sets that. This setting selects how much computing
  time LAME uses to encode at that bitrate. More time makes better use of the
  bitrate.

  | Value | Meaning                          |
  |-------|----------------------------------|
  | 0     | best, very slow                  |
  | 3     | near-best, not too slow          |
  | 5     | good, fast                       |
  | 7     | acceptable, really fast          |
  | 9     | worst                            |

  This function **does not reject** a value outside 0 to 9. It changes a
  value below 0 to 0 and a value above 9 to 9, and returns 0.

  \param gfp      the encoder instance.
  \param quality  0 to 9.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_quality(lame_global_flags * gfp, int quality)
{
    if (is_lame_global_flags_valid(gfp)) {
        if (quality < 0) {
            gfp->quality = 0;
        }
        else if (quality > 9) {
            gfp->quality = 9;
        }
        else {
            gfp->quality = quality;
        }
        return 0;
    }
    return -1;
}

/*! Get the quality setting (\c lame_set_quality()). */
/*!
  \param gfp the encoder instance.
  \return 0 to 9, or -1 if it is not set yet. \c lame_init_params() sets the
          default. 0 if the instance is not usable.
*/
int
lame_get_quality(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->quality;
    }
    return 0;
}


/*! Set the channel mode written into the stream. */
/*!
  - \c STEREO: LAME encodes the two channels separately.
  - \c JOINT_STEREO: LAME uses mid/side coding in frames where this saves
    bits. This gives good stereo at low bitrates.
  - \c MONO: LAME mixes the two channels into one.
  - \c DUAL_CHANNEL: LAME does not implement this mode.

  The default is \c NOT_SET. LAME then chooses the mode in
  \c lame_init_params(), from the number of input channels and the compression
  ratio. This setting does not depend on \c lame_set_num_channels(), which
  describes the input, not the output.

  \param gfp   the encoder instance.
  \param mode  one of the \c MPEG_mode values.
  \return 0 on success. -1 if \a mode is not a known value, or if the instance
          is not usable. This function accepts \c DUAL_CHANNEL and returns 0,
          but LAME does not implement it. Do not use it.
*/
int
lame_set_mode(lame_global_flags * gfp, MPEG_mode mode)
{
    if (is_lame_global_flags_valid(gfp)) {
        int     mpg_mode = mode;
        /* default: lame chooses based on compression ratio and input channels */
        if (mpg_mode < 0 || MAX_INDICATOR <= mpg_mode)
            return -1;  /* Unknown MPEG mode! */
        gfp->mode = mode;
        return 0;
    }
    return -1;
}

/*! Get the channel mode. */
/*!
  Before \c lame_init_params(), this returns the value that was set.
  \c NOT_SET then means "LAME chooses". After \c lame_init_params(), it returns
  the mode that the encoder uses.

  \param gfp the encoder instance.
  \return the mode. \c NOT_SET if the instance is not usable.
*/
MPEG_mode
lame_get_mode(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(gfp->mode < MAX_INDICATOR);
        return gfp->mode;
    }
    return NOT_SET;
}



/*! Use an M/S mode with a threshold based on the compression ratio. */
/*!
  \deprecated This function only selects \c JOINT_STEREO, for 0 and for 1.
  LAME normally chooses this mode itself. Use \c lame_set_mode() instead.
  lame.h does not declare this function. The library still exports it, so
  that programs built against an older release still link.

  \param gfp          the encoder instance.
  \param mode_automs  0 or 1. The value is checked and then not used.
  \return 0 on success. -1 if the value is not 0 or 1, or if the instance is
          not usable.
*/
int
lame_set_mode_automs(lame_global_flags * gfp, int mode_automs)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 0 (disabled) */

        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 > mode_automs || 1 < mode_automs)
            return -1;
        lame_set_mode(gfp, JOINT_STEREO);
        return 0;
    }
    return -1;
}

/*! Get the automatic-M/S setting. */
/*!
  \deprecated See \c lame_set_mode_automs(). The value says nothing about the
  instance.
  \param gfp  ignored.
  \return always 1.
*/
int
lame_get_mode_automs(const lame_global_flags * gfp)
{
    (void) gfp;
    return 1;
}


/*! Force mid/side coding on every frame. */
/*!
  Normally the psychoacoustic model chooses left/right or mid/side coding for
  each frame. This setting removes that choice. Use it only for tests. It
  lowers the quality of frames that the encoder would code as left/right.
  It needs \c JOINT_STEREO. The default is off.

  \param gfp       the encoder instance.
  \param force_ms  1 to force mid/side, 0 to let the encoder choose.
  \return 0 on success. -1 if the value is not 0 or 1, or if the instance is
          not usable.
*/
int
lame_set_force_ms(lame_global_flags * gfp, int force_ms)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 0 (disabled) */

        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 > force_ms || 1 < force_ms)
            return -1;
        gfp->force_ms = force_ms;
        return 0;
    }
    return -1;
}

/*! Get whether mid/side coding is forced on every frame. */
/*!
  \param gfp the encoder instance.
  \return 1 if mid/side is forced, 0 if not. 0 also if the instance is not
          usable.
*/
int
lame_get_force_ms(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(0 <= gfp->force_ms && 1 >= gfp->force_ms);
        return gfp->force_ms;
    }
    return 0;
}


/*! Use the free-format bitrate. */
/*!
  With free format, a frame can use any bitrate, not only one of the values in
  the standard's table. So a bitrate between or above the table values is
  possible, up to 640 kbps: \c lame_init_params() encodes a higher one at
  640 kbps. Many decoders do not support free format. For this reason LAME
  warns about bitrates above 320 kbps. A free-format file may not play in
  other programs. The default is off.

  \param gfp          the encoder instance.
  \param free_format  1 to use free format, 0 for a bitrate from the table.
  \return 0 on success. -1 if the value is not 0 or 1, or if the instance is
          not usable.
*/
int
lame_set_free_format(lame_global_flags * gfp, int free_format)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 0 (disabled) */

        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 > free_format || 1 < free_format)
            return -1;
        gfp->free_format = free_format;
        return 0;
    }
    return -1;
}

/*! Get whether the free-format bitrate is in use. */
/*!
  \param gfp the encoder instance.
  \return 1 if free format is used, 0 if not. 0 also if the instance is not
          usable.
*/
int
lame_get_free_format(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(0 <= gfp->free_format && 1 >= gfp->free_format);
        return gfp->free_format;
    }
    return 0;
}



/*! Measure ReplayGain while encoding. */
/*!
  LAME runs the ReplayGain analysis on the input during the encode, so that
  the track gain can be written into the LAME tag. Read the results afterwards
  with \c lame_get_RadioGain() and \c lame_get_AudiophileGain().

  This measures the *input*. To measure the encoded result, also set
  \c lame_set_decode_on_the_fly(). The analysis then uses the decoded output.
  The default is off.

  \param gfp             the encoder instance.
  \param findReplayGain  1 to analyze, 0 not to.
  \return 0 on success. -1 if the value is not 0 or 1, or if the instance is
          not usable.
*/
int
lame_set_findReplayGain(lame_global_flags * gfp, int findReplayGain)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 0 (disabled) */

        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 > findReplayGain || 1 < findReplayGain)
            return -1;
        gfp->findReplayGain = findReplayGain;
        return 0;
    }
    return -1;
}

/*! Get whether ReplayGain analysis is enabled. */
/*!
  \param gfp the encoder instance.
  \return 1 if the analysis is on, 0 if not. 0 also if the instance is not
          usable.
*/
int
lame_get_findReplayGain(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(0 <= gfp->findReplayGain && 1 >= gfp->findReplayGain);
        return gfp->findReplayGain;
    }
    return 0;
}


/*! Decode each frame back as it is encoded, and measure the result. */
/*!
  LAME decodes its own output during the encode. This is the only way to know
  the real peak of the file, because encoding can make the peak higher than in
  the input. It sets \c lame_get_PeakSample() and the clipping values
  \c lame_get_noclipGainChange() and \c lame_get_noclipScale(). If
  \c lame_set_findReplayGain() is also on, the ReplayGain analysis uses the
  decoded audio.

  It roughly doubles the work, so it is off by default.

  \param gfp                the encoder instance.
  \param decode_on_the_fly  1 to decode and measure, 0 not to.
  \return 0 on success. -1 if the value is not 0 or 1, if the instance is not
          usable, **or if the library was built without the decoder**. Such a
          build rejects every call to this function.
*/
int
lame_set_decode_on_the_fly(lame_global_flags * gfp, LAME_UNUSED int decode_on_the_fly)
{
    if (is_lame_global_flags_valid(gfp)) {
#ifndef HAVE_MPG123
        return -1;
#else
        /* default = 0 (disabled) */

        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 > decode_on_the_fly || 1 < decode_on_the_fly)
            return -1;

        gfp->decode_on_the_fly = decode_on_the_fly;

        return 0;
#endif
    }
    return -1;
}

/*! Get whether the encoder decodes its own output while encoding. */
/*!
  \param gfp the encoder instance.
  \return 1 if it is on, 0 if not. 0 also if the instance is not usable, and
          always 0 in a build without the decoder.
*/
int
lame_get_decode_on_the_fly(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(0 <= gfp->decode_on_the_fly && 1 >= gfp->decode_on_the_fly);
        return gfp->decode_on_the_fly;
    }
    return 0;
}


/*! Find the peak sample. */
/*!
  \deprecated Use \c lame_set_decode_on_the_fly(). This function only calls
  it.
  \param gfp  the encoder instance.
  \param arg  1 or 0.
  \return the result of \c lame_set_decode_on_the_fly().
*/
int
lame_set_findPeakSample(lame_global_flags * gfp, int arg)
{
    return lame_set_decode_on_the_fly(gfp, arg);
}

/*! Get the peak-sample setting. */
/*!
  \deprecated Use \c lame_get_decode_on_the_fly(). This function only calls
  it.
  \param gfp the encoder instance.
  \return the result of \c lame_get_decode_on_the_fly().
*/
int
lame_get_findPeakSample(const lame_global_flags * gfp)
{
    return lame_get_decode_on_the_fly(gfp);
}

/*! Perform ReplayGain analysis on the input. */
/*!
  \deprecated Use \c lame_set_findReplayGain(). This function only calls it.
  \param gfp  the encoder instance.
  \param arg  1 or 0.
  \return the result of \c lame_set_findReplayGain().
*/
int
lame_set_ReplayGain_input(lame_global_flags * gfp, int arg)
{
    return lame_set_findReplayGain(gfp, arg);
}

/*! Get the input ReplayGain setting. */
/*!
  \deprecated Use \c lame_get_findReplayGain(). This function only calls it.
  \param gfp the encoder instance.
  \return the result of \c lame_get_findReplayGain().
*/
int
lame_get_ReplayGain_input(const lame_global_flags * gfp)
{
    return lame_get_findReplayGain(gfp);
}

/*! Perform ReplayGain analysis on the decoded output. */
/*!
  \deprecated This function sets \c lame_set_decode_on_the_fly() and
  \c lame_set_findReplayGain() together. Call those two instead.
  \param gfp  the encoder instance.
  \param arg  1 or 0.
  \return 0 if both calls succeed. -1 if one of them fails, for example in a
          build without the decoder. The first setting may then already be
          changed.
*/
int
lame_set_ReplayGain_decode(lame_global_flags * gfp, int arg)
{
    if (lame_set_decode_on_the_fly(gfp, arg) < 0 || lame_set_findReplayGain(gfp, arg) < 0)
        return -1;
    else
        return 0;
}

/*! Get whether ReplayGain is being measured on the decoded output. */
/*!
  \deprecated Use \c lame_get_decode_on_the_fly() and
  \c lame_get_findReplayGain().
  \param gfp the encoder instance.
  \return 1 only if both of those are on, 0 otherwise.
*/
int
lame_get_ReplayGain_decode(const lame_global_flags * gfp)
{
    if (lame_get_decode_on_the_fly(gfp) > 0 && lame_get_findReplayGain(gfp) > 0)
        return 1;
    else
        return 0;
}


/*! Tell the encoder how many files a gapless set has. */
/*!
  This is part of the gapless support for split files, with
  \c lame_set_nogap_currentindex() and \c lame_encode_flush_nogap(). With the
  total, the encoder can write into the LAME tag of each file where that file
  is in the sequence. A player can then join the files without a gap.

  \param gfp              the encoder instance.
  \param the_nogap_total  number of files in the set.
  \return 0 on success. -1 if the instance is not usable. This function does
          not check the value.
*/
int
lame_set_nogap_total(lame_global_flags * gfp, int the_nogap_total)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->nogap_total = the_nogap_total;
        return 0;
    }
    return -1;
}

/*! Get the number of files in the gapless set. */
/*!
  \param gfp the encoder instance.
  \return the number. 0 if the instance is not usable.
*/
int
lame_get_nogap_total(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->nogap_total;
    }
    return 0;
}

/*! Tell the encoder which file of a gapless set is being written. */
/*!
  Use it with \c lame_set_nogap_total(). Set it before each file in the
  sequence.

  \param gfp              the encoder instance.
  \param the_nogap_index  index of the current file within the set.
  \return 0 on success. -1 if the instance is not usable. This function does
          not check the value against the total.
*/
int
lame_set_nogap_currentindex(lame_global_flags * gfp, int the_nogap_index)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->nogap_current = the_nogap_index;
        return 0;
    }
    return -1;
}

/*! Get the index of the current file within the gapless set. */
/*!
  \param gfp the encoder instance.
  \return the index. 0 if the instance is not usable.
*/
int
lame_get_nogap_currentindex(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->nogap_current;
    }
    return 0;
}


/*! Route the library's error messages to a callback of your own. */
/*!
  LAME reports through three streams: errors, debug output and ordinary
  messages. By default all three go to \c stderr. An application that uses
  the library usually wants them somewhere else. These three setters set
  where they go.

  The callback gets a \c printf format string and a \c va_list. So it is
  normally one call to \c vfprintf or \c vsnprintf. LAME calls it from inside
  the encoding calls. The messages are for people. Their wording can change
  between versions, so do not parse them.

  \code
  static void report(const char *fmt, va_list ap) { vfprintf(mylog, fmt, ap); }
  lame_set_errorf(gfp, report);
  lame_set_msgf(gfp, report);
  lame_set_debugf(gfp, report);
  \endcode

  \param gfp   the encoder instance.
  \param func  the callback, or \c NULL to turn this stream off.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_errorf(lame_global_flags * gfp, lame_report_function func)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->report.errorf = func;
        return 0;
    }
    return -1;
}

/*! Route the library's debug output to a callback of your own. */
/*!
  The debug stream. \c lame_set_errorf() describes the three streams, and the
  same rules apply.

  \param gfp   the encoder instance.
  \param func  the callback, or \c NULL to turn this stream off.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_debugf(lame_global_flags * gfp, lame_report_function func)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->report.debugf = func;
        return 0;
    }
    return -1;
}

/*! Route the library's ordinary messages to a callback of your own. */
/*!
  The message stream. \c lame_set_errorf() describes the three streams.
  \c lame_print_config() and \c lame_print_internals() write to this one.

  \param gfp   the encoder instance.
  \param func  the callback, or \c NULL to turn this stream off.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_msgf(lame_global_flags * gfp, lame_report_function func)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->report.msgf = func;
        return 0;
    }
    return -1;
}


/*! Set the bitrate, in kbps. */
/*!
  For CBR this is the bitrate of every frame. For ABR, set the average
  bitrate with \c lame_set_VBR_mean_bitrate_kbps().
  Set either this or \c lame_set_compression_ratio(), not both. If neither is
  set, LAME uses a compression ratio of 11.025.

  A bitrate above 320 kbps needs free format, and free format cannot use the
  bit reservoir. So a value above 320 also **turns the bit reservoir off**.
  The function does not report this. \c lame_get_disable_reservoir() then
  returns 1.

  \param gfp    the encoder instance.
  \param brate  bitrate in kbps. This function does not check the value.
                \c lame_init_params() changes it to the nearest bitrate that
                the MPEG version and the sample rate allow.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_brate(lame_global_flags * gfp, int brate)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->brate = brate;
        if (brate > 320) {
            gfp->disable_reservoir = 1;
        }
        return 0;
    }
    return -1;
}

/*! Get the bitrate, in kbps. */
/*!
  Before \c lame_init_params(), this returns the value that was set. After
  \c lame_init_params(), it returns the bitrate that the encoder uses. The two
  can differ, because LAME changes a bitrate that is not allowed to the
  nearest allowed one.

  \param gfp the encoder instance.
  \return the bitrate in kbps. 0 if the instance is not usable. 0 also before
          \c lame_init_params() if the bitrate comes from the compression
          ratio.
*/
int
lame_get_brate(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->brate;
    }
    return 0;
}

/*! Set the bitrate indirectly, as a compression ratio. */
/*!
  The ratio of the input data rate to the output data rate. So 11 means about
  eleven times smaller. Set either this or \c lame_set_brate(), not both. If
  neither is set, \c lame_init_params() uses 11.025, which gives 128 kbps for
  44.1 kHz 16-bit stereo input.

  \c lame_init_params() converts the ratio into a bitrate. After that,
  \c lame_get_brate() returns the bitrate that the encoder uses.

  \param gfp                the encoder instance.
  \param compression_ratio  the ratio, a finite number. This function does
                            not check the range.
  \return 0 on success. -1 if the ratio is NaN or infinite, or if the instance
          is not usable. The setting does not change then.
*/
int
lame_set_compression_ratio(lame_global_flags * gfp, float compression_ratio)
{
    if (is_lame_global_flags_valid(gfp) && float_is_finite(compression_ratio)) {
        gfp->compression_ratio = compression_ratio;
        return 0;
    }
    return -1;
}

/*! Get the compression ratio. */
/*!
  \param gfp the encoder instance.
  \return the ratio. 0 if the instance is not usable. After
          \c lame_init_params(), it is the ratio of the bitrate that the
          encoder uses, whichever of the two settings was set.
*/
float
lame_get_compression_ratio(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->compression_ratio;
    }
    return 0;
}




/*
 * frame parameters
 */

/*! Set the copyright bit in the frame header. */
/*!
  One of the four flag bits in every MPEG audio frame header. LAME writes it,
  and nothing else uses it. It is a statement to the reader of the file.
  Neither the encoder nor a decoder enforces it. The same value is copied into
  the header of the LAME tag frame.

  \param gfp        the encoder instance.
  \param copyright  1 to set the bit, 0 to clear it. The default is 0.
  \return 0 on success. -1 if the value is not 0 or 1, or if the instance is
          not usable.
*/
int
lame_set_copyright(lame_global_flags * gfp, int copyright)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 0 (disabled) */

        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 > copyright || 1 < copyright)
            return -1;
        gfp->copyright = copyright;
        return 0;
    }
    return -1;
}

/*! Get the copyright bit. */
/*!
  \param gfp the encoder instance.
  \return 0 or 1. 0 if the instance is not usable. 0 is also the default.
*/
int
lame_get_copyright(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(0 <= gfp->copyright && 1 >= gfp->copyright);
        return gfp->copyright;
    }
    return 0;
}


/*! Set the original bit in the frame header. */
/*!
  Like \c lame_set_copyright(). This bit says that the file is an original
  recording, not a copy. LAME writes it into every frame header and into the
  LAME tag frame. Nothing reads it back.

  Note that the default is 1, not 0. So an encode that does not set it marks
  the file as an original.

  \param gfp       the encoder instance.
  \param original  1 to set the bit, 0 to clear it. The default is 1.
  \return 0 on success. -1 if the value is not 0 or 1, or if the instance is
          not usable.
*/
int
lame_set_original(lame_global_flags * gfp, int original)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 1 (enabled) */

        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 > original || 1 < original)
            return -1;
        gfp->original = original;
        return 0;
    }
    return -1;
}

/*! Get the original bit. */
/*!
  \param gfp the encoder instance.
  \return 0 or 1. 0 if the instance is not usable.
*/
int
lame_get_original(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(0 <= gfp->original && 1 >= gfp->original);
        return gfp->original;
    }
    return 0;
}


/*! Add a CRC checksum to every frame. */
/*!
  Turns on the optional 16-bit CRC over the frame header and the side
  information. It uses **two bytes per frame** that would otherwise hold audio
  data. So at a fixed bitrate the audio is encoded with slightly less
  precision. With the CRC, a decoder can detect a damaged frame and mute it
  instead of playing noise.

  In the frame header the protection bit has the opposite meaning: 0 means
  that the frame has a CRC.

  \param gfp               the encoder instance.
  \param error_protection  1 to add the checksum, 0 for none. The default
                           is 0.
  \return 0 on success. -1 if the value is not 0 or 1, or if the instance is
          not usable.
*/
int
lame_set_error_protection(lame_global_flags * gfp, int error_protection)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 0 (disabled) */

        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 > error_protection || 1 < error_protection)
            return -1;
        gfp->error_protection = error_protection;
        return 0;
    }
    return -1;
}

/*! Get the CRC setting. */
/*!
  \param gfp the encoder instance.
  \return 1 if frames have a CRC, 0 if not. 0 also if the instance is not
          usable.
*/
int
lame_get_error_protection(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(0 <= gfp->error_protection && 1 >= gfp->error_protection);
        return gfp->error_protection;
    }
    return 0;
}



/*! Choose how frames are padded. */
/*!
  \deprecated This function does nothing. The encoder decides for each frame
  whether it needs a padding slot to keep the average bitrate exact. lame.h
  does not declare this function. The library still exports it, so that
  programs built against an older release still link.

  This function **always returns 0**, even for an instance that is not
  usable.

  \param gfp           ignored.
  \param padding_type  ignored.
  \return always 0.
*/
int
lame_set_padding_type(lame_global_flags * gfp, Padding_type padding_type)
{
    (void) gfp;
    (void) padding_type;
    return 0;
}

/*! Get the padding mode. */
/*!
  \deprecated See \c lame_set_padding_type().
  \param gfp  ignored.
  \return always \c PAD_ADJUST.
*/
Padding_type
lame_get_padding_type(const lame_global_flags * gfp)
{
    (void) gfp;
    return PAD_ADJUST;
}


/*! Set the private bit in the frame header. */
/*!
  The fourth flag bit in the frame header. The standard reserves it for private
  use and gives it no meaning. LAME writes it into every frame header and into
  the LAME tag frame, and never reads it. An application can use this bit for
  one bit of its own data. Other applications may give the bit a different
  meaning.

  \param gfp        the encoder instance.
  \param extension  1 to set the bit, 0 to clear it. The default is 0.
  \return 0 on success. -1 if the value is not 0 or 1, or if the instance is
          not usable.
*/
int
lame_set_extension(lame_global_flags * gfp, int extension)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 0 (disabled) */
        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 > extension || 1 < extension)
            return -1;
        gfp->extension = extension;
        return 0;
    }
    return -1;
}

/*! Get the private bit. */
/*!
  \param gfp the encoder instance.
  \return 0 or 1. 0 if the instance is not usable. 0 is also the default.
*/
int
lame_get_extension(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(0 <= gfp->extension && 1 >= gfp->extension);
        return gfp->extension;
    }
    return 0;
}


/*! Choose how large a bit reservoir the bitstream may rely on. */
/*!
  This is **not a flag**, despite its name. It takes one of the three
  \c buffer_constraint values. It sets one limit only: the largest
  `main_data_begin`. That is how far back into earlier frames the audio data
  of a frame may start.

  - \c MDB_DEFAULT: a limit that all common decoders support. It is the size
    of a 320 kbps frame at 32 kHz.
  - \c MDB_STRICT_ISO: the limit that the ISO standard allows for the layout
    in use. Choose this if the output must pass a conformance checker.
  - \c MDB_MAXIMUM: the largest value the format allows, 7680 bits per
    granule. It gives the bit allocation the most room. LAME uses it unless
    another value is set.

  Note that the default is \c MDB_MAXIMUM, **not 0**.

  \param gfp  the encoder instance.
  \param val  one of the \c buffer_constraint values.
  \return 0 on success. -1 if \a val is not a \c buffer_constraint value, or
          if the instance is not usable.
*/
int
lame_set_strict_ISO(lame_global_flags * gfp, int val)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = MDB_MAXIMUM */
        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (val < MDB_DEFAULT || MDB_MAXIMUM < val)
            return -1;
        gfp->strict_ISO = val;
        return 0;
    }
    return -1;
}

/*! Get the bit reservoir constraint. */
/*!
  \param gfp the encoder instance.
  \return one of the \c buffer_constraint values. \c MDB_DEFAULT (0) if the
          instance is not usable. \c MDB_DEFAULT is a valid setting, but it is
          not the default.
*/
int
lame_get_strict_ISO(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->strict_ISO;
    }
    return 0;
}




/********************************************************************
 * quantization/noise shaping 
 ***********************************************************************/

/*! Forbid frames from borrowing space from earlier ones. */
/*!
  The bit reservoir lets a frame that needs more bits than its share take them
  from the unused space of earlier frames. With the reservoir off, each frame
  stands alone. This lowers the quality at a given bitrate, because a
  difficult passage cannot get extra bits. In return, each frame decodes
  without the frames before it.

  \c lame_set_brate() above 320 kbps also turns the reservoir off, without a
  message. Read the getter to see it.

  \param gfp                the encoder instance.
  \param disable_reservoir  1 to turn the reservoir off, 0 to use it. The
                            default is 0.
  \return 0 on success. -1 if the value is not 0 or 1, or if the instance is
          not usable.
*/
int
lame_set_disable_reservoir(lame_global_flags * gfp, int disable_reservoir)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 0 (disabled) */

        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 > disable_reservoir || 1 < disable_reservoir)
            return -1;
        gfp->disable_reservoir = disable_reservoir;
        return 0;
    }
    return -1;
}

/*! Get the bit reservoir setting. */
/*!
  \param gfp the encoder instance.
  \return 1 if the reservoir is off, 0 if it is used. 0 also if the instance
          is not usable.
*/
int
lame_get_disable_reservoir(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(0 <= gfp->disable_reservoir && 1 >= gfp->disable_reservoir);
        return gfp->disable_reservoir;
    }
    return 0;
}




/*! Set both quantization comparison functions at once. */
/*!
  It sets \c lame_set_quant_comp() and \c lame_set_quant_comp_short() to the
  same value. New code should set the two directly, because they usually need
  different values.

  \param gfp            the encoder instance.
  \param experimentalX  the comparison function, used for both.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_experimentalX(lame_global_flags * gfp, int experimentalX)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_set_quant_comp(gfp, experimentalX);
        lame_set_quant_comp_short(gfp, experimentalX);
        return 0;
    }
    return -1;
}

/*! Get the long-block quantization comparison function. */
/*!
  The setter writes both values, but this getter reads only the long-block
  one. If the short-block value was changed later, this getter does not show
  it.

  \param gfp the encoder instance.
  \return the result of \c lame_get_quant_comp().
*/
int
lame_get_experimentalX(const lame_global_flags * gfp)
{
    return lame_get_quant_comp(gfp);
}


/*! Choose how two candidate quantizations are compared. */
/*!
  The inner loop tries several quantizations of a granule and keeps the best
  one. This setting selects how the encoder measures which one is best: by the
  number of distorted scalefactor bands, the total noise, the peak noise, or a
  weighted mix of these. It does not change how much work the encoder does.
  \c lame_set_quality() sets that.

  Values 0 to 9 select a method. Any other value works like 9. A higher value
  is not better. Use this setting only for experiments.

  The default is -1, which means "not set". \c lame_init_params() then sets 1
  for long blocks. So the getter returns -1 before \c lame_init_params() and
  another value after it, although nobody called this setter.

  \param gfp         the encoder instance.
  \param quant_type  the method, 0 to 9. This function does not check the
                     value.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_quant_comp(lame_global_flags * gfp, int quant_type)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->quant_comp = quant_type;
        return 0;
    }
    return -1;
}

/*! Get the long-block quantization comparison. */
/*!
  \param gfp the encoder instance.
  \return the method, or -1 if it is not set yet. 0 if the instance is not
          usable. 0 is also a valid method.
*/
int
lame_get_quant_comp(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->quant_comp;
    }
    return 0;
}


/*! Choose the comparison used for short blocks. */
/*!
  As \c lame_set_quant_comp(), for granules encoded as short blocks. Short
  blocks cover a transient, and a different measure is often better there.
  The same values are allowed, and this function does not check them either.

  If it is not set, \c lame_init_params() sets 0, not 1 as for long blocks.

  \param gfp         the encoder instance.
  \param quant_type  the method, 0 to 9.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_quant_comp_short(lame_global_flags * gfp, int quant_type)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->quant_comp_short = quant_type;
        return 0;
    }
    return -1;
}

/*! Get the short-block quantization comparison. */
/*!
  \param gfp the encoder instance.
  \return the method, or -1 if it is not set yet. 0 if the instance is not
          usable.
*/
int
lame_get_quant_comp_short(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->quant_comp_short;
    }
    return 0;
}


/*! Suppress the extra bits normally spent above 16 kHz. */
/*!
  A non-zero value stops LAME from giving extra bits to scalefactor band 21,
  the highest band, above about 16 kHz. LAME gives these bits on MPEG-1
  material with a sample rate above 44 kHz. The result is a smaller file with
  less precision in the highest octave.

  \param gfp            the encoder instance.
  \param experimentalY  non-zero to stop the extra bits. The default is 0.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_experimentalY(lame_global_flags * gfp, int experimentalY)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->experimentalY = experimentalY;
        return 0;
    }
    return -1;
}

/*! Get the sfb21 suppression setting. */
/*!
  \param gfp the encoder instance.
  \return the value that was set. 0 if the instance is not usable. 0 is also
          the default.
*/
int
lame_get_experimentalY(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->experimentalY;
    }
    return 0;
}


/*! Compute short-block masking thresholds even where they are not needed. */
/*!
  Normally the psychoacoustic model skips the short-block analysis for a
  granule that it encodes as a long block. A non-zero value makes it compute
  the analysis anyway, so that thresholds exist for every granule. This is
  slower. Use it to compare the two paths, not for normal encoding.

  LAME reads it once, when \c lame_init_params() sets up the psychoacoustic
  model. Changing it later does nothing.

  \param gfp            the encoder instance.
  \param experimentalZ  non-zero to force the computation. The default is 0.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_experimentalZ(lame_global_flags * gfp, int experimentalZ)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->experimentalZ = experimentalZ;
        return 0;
    }
    return -1;
}

/*! Get the forced short-block analysis setting. */
/*!
  \param gfp the encoder instance.
  \return the value that was set. 0 if the instance is not usable. 0 is also
          the default.
*/
int
lame_get_experimentalZ(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->experimentalZ;
    }
    return 0;
}


/*! Set the packed psychoacoustic tuning word. */
/*!
  This is not a flag, despite its name. It is one \c int that holds several
  separate settings. \c lame_init_params() reads it as follows:

  | bits | meaning |
  |------|---------|
  | 0    | not used. Reserved for selecting a psychoacoustic model. |
  | 1    | safe joint stereo. Used only when the mode is joint stereo. |
  | 2-7  | bass adjustment |
  | 8-13 | alto (mid-range) adjustment |
  | 14-19| treble adjustment |
  | 20-25| extra adjustment for the highest scalefactor band, **added to** the treble one |

  Each adjustment is a 6-bit two's complement number in steps of a quarter
  decibel. So the range is -32 to 31 quarter-dB, that is **-8.00 to
  +7.75 dB**. A negative value gives that range less weight in the masking
  calculation.

  The tuning options of the lame tool use this setting. Combine the value with
  the current one by a bitwise OR, as the presets do. A plain assignment
  clears the fields that a preset set, without a message. Passing 1 to mean
  "on" sets only bit 0, which selects nothing.

  \param gfp            the encoder instance.
  \param exp_nspsytune  the packed value. This function does not check it.
                        Bits above the listed fields are ignored.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_exp_nspsytune(lame_global_flags * gfp, int exp_nspsytune)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 0 (disabled) */
        gfp->exp_nspsytune = exp_nspsytune;
        return 0;
    }
    return -1;
}

/*! Get the packed psychoacoustic tuning word. */
/*!
  \param gfp the encoder instance.
  \return the packed value, to change and set again. 0 if the instance is not
          usable. 0 is also the default.
*/
int
lame_get_exp_nspsytune(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->exp_nspsytune;
    }
    return 0;
}




/********************************************************************
 * VBR control
 ***********************************************************************/

/*! Choose between constant, average and variable bitrate. */
/*!
  - \c vbr_off: constant bitrate. Every frame has the bitrate of
    \c lame_set_brate().
  - \c vbr_abr: average bitrate. The bitrate changes from frame to frame, but
    the encoder keeps the average near the value of
    \c lame_set_VBR_mean_bitrate_kbps().
  - \c vbr_mtrh: variable bitrate. Each frame gets the bits that the quality
    level of \c lame_set_VBR_quality() needs. So the bitrate follows the
    material. This is \c vbr_default.
  - \c vbr_rh: the older variable bitrate implementation. Some listeners
    prefer its output.
  - \c vbr_mt: an obsolete name for \c vbr_mtrh. It stays so that old code
    still compiles.

  The default is \c vbr_off, so a caller who wants variable bitrate must
  select it. Which other settings matter depends on this mode. The encoder
  does not read the settings that do not apply.

  \param gfp  the encoder instance.
  \param VBR  one of the \c vbr_mode values, but not \c vbr_max_indicator.
  \return 0 on success. -1 if \a VBR is not a \c vbr_mode value, or if the
          instance is not usable.
*/
int
lame_set_VBR(lame_global_flags * gfp, vbr_mode VBR)
{
    if (is_lame_global_flags_valid(gfp)) {
        int     vbr_q = VBR;
        if (0 > vbr_q || vbr_max_indicator <= vbr_q)
            return -1;  /* Unknown VBR mode! */
        gfp->VBR = VBR;
        return 0;
    }
    return -1;
}

/*! Get the bitrate mode. */
/*!
  \param gfp the encoder instance.
  \return the mode. \c vbr_off if the instance is not usable. \c vbr_off is
          also the default.
*/
vbr_mode
lame_get_VBR(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(gfp->VBR < vbr_max_indicator);
        return gfp->VBR;
    }
    return vbr_off;
}


/*! Set the variable bitrate quality level, as a whole number. */
/*!
  The quality level of the variable bitrate modes. **0 is the best quality and
  the largest file, 9 the worst quality and the smallest file.** This is the
  opposite direction to a bitrate. The default is 4.

  This is the same setting as \c lame_set_VBR_quality(), which also accepts
  levels in between. Setting it here clears a fraction that was set before.

  For a value outside 0 to 9, this function stores the nearest valid value,
  0 or 9, and **returns -1**. So the setting can change even when the
  function returns -1. The instance is still usable.

  \param gfp    the encoder instance.
  \param VBR_q  quality level, 0 to 9.
  \return 0 on success. -1 if \a VBR_q was out of range and was changed to 0
          or 9, or if the instance is not usable.
*/
int
lame_set_VBR_q(lame_global_flags * gfp, int VBR_q)
{
    if (is_lame_global_flags_valid(gfp)) {
        int     ret = 0;

        if (0 > VBR_q) {
            ret = -1;   /* Unknown VBR quality level! */
            VBR_q = 0;
        }
        if (9 < VBR_q) {
            ret = -1;
            VBR_q = 9;
        }
        gfp->VBR_q = VBR_q;
        gfp->VBR_q_frac = 0;
        return ret;
    }
    return -1;
}

/*! Get the variable bitrate quality level, rounded down. */
/*!
  \param gfp the encoder instance.
  \return the whole part of the level, 0 to 9. A fraction set with
          \c lame_set_VBR_quality() is not included. 0 if the instance is not
          usable. 0 is the best quality, not a neutral value.
*/
int
lame_get_VBR_q(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(0 <= gfp->VBR_q && 10 > gfp->VBR_q);
        return gfp->VBR_q;
    }
    return 0;
}

/*! Set the variable bitrate quality level, with a fraction. */
/*!
  The same setting as \c lame_set_VBR_q(), with a fraction. 2.5 is between
  quality levels 2 and 3. The whole part selects the level, and the fraction
  interpolates within it. The lame tool passes its fractional quality values
  this way.

  The highest value is 9.999, not 9. LAME splits the value into a whole level
  and a fraction, and level 9 can have a fraction of up to 0.999. As with
  \c lame_set_VBR_q(), a value out of range is changed to the nearest end,
  and the function returns -1 although the setting changed. NaN and infinite
  values are rejected, and the setting does not change.

  \param gfp    the encoder instance.
  \param VBR_q  quality level, 0 to 9.999.
  \return 0 on success. -1 if \a VBR_q was out of range and was changed, if it
          is NaN or infinite, or if the instance is not usable.
*/
int
lame_set_VBR_quality(lame_global_flags * gfp, float VBR_q)
{
    if (is_lame_global_flags_valid(gfp) && float_is_finite(VBR_q)) {
        int     ret = 0;

        if (0 > VBR_q) {
            ret = -1;   /* Unknown VBR quality level! */
            VBR_q = 0;
        }
        if (9.999 < VBR_q) {
            ret = -1;
            VBR_q = 9.999;
        }

        gfp->VBR_q = (int) VBR_q;
        gfp->VBR_q_frac = VBR_q - gfp->VBR_q;

        return ret;
    }
    return -1;
}

/*! Get the variable bitrate quality level, fraction included. */
/*!
  \param gfp the encoder instance.
  \return the level plus its fraction. 0 if the instance is not usable. 0 is
          the best quality, not a neutral value.
*/
float
lame_get_VBR_quality(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->VBR_q + gfp->VBR_q_frac;
    }
    return 0;
}


/*! Set the target average bitrate. */
/*!
  The average bitrate that the ABR mode aims at over the whole stream, in
  kbps. The default is 128.

  The CBR mode also reads this value. In that mode, if \c lame_set_brate() was
  not called, LAME encodes at this bitrate.

  \c lame_init_params() changes the value instead of rejecting it. First it
  limits the value to what the MPEG version allows. Then it limits it to the
  range of \c lame_set_VBR_min_bitrate_kbps() and
  \c lame_set_VBR_max_bitrate_kbps(). After that, the getter returns the
  changed value.

  \param gfp                     the encoder instance.
  \param VBR_mean_bitrate_kbps   target average, in kbps. This function does
                                 not check the value. \c lame_init_params()
                                 corrects an impossible value.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_VBR_mean_bitrate_kbps(lame_global_flags * gfp, int VBR_mean_bitrate_kbps)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->VBR_mean_bitrate_kbps = VBR_mean_bitrate_kbps;
        return 0;
    }
    return -1;
}

/*! Get the target average bitrate. */
/*!
  \param gfp the encoder instance.
  \return the target in kbps: the value that was set before
          \c lame_init_params(), the value that is used after it. 0 if the
          instance is not usable.
*/
int
lame_get_VBR_mean_bitrate_kbps(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->VBR_mean_bitrate_kbps;
    }
    return 0;
}

/*! Set the lowest bitrate a variable bitrate encode may use. */
/*!
  The lowest bitrate per frame, in kbps, for the VBR and ABR modes. **0 means
  that no minimum is set**, not a minimum of 0. The encoder then allows the
  lowest bitrate of the format.

  MP3 has a fixed set of bitrates, so not every number is possible.
  \c lame_init_params() changes the value to the nearest bitrate of the table,
  for the MPEG version and sample rate that are used. After that, the getter
  returns this bitrate. For example, 100 becomes 96. This is not an error.

  The minimum is not strict. Silent passages use less, unless
  \c lame_set_VBR_hard_min() is on.

  \c lame_init_params() fails if this minimum, after the change, is above the
  maximum of \c lame_set_VBR_max_bitrate_kbps().

  \param gfp                   the encoder instance.
  \param VBR_min_bitrate_kbps  the minimum in kbps, or 0 for none. This
                               function does not check the value.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_VBR_min_bitrate_kbps(lame_global_flags * gfp, int VBR_min_bitrate_kbps)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->VBR_min_bitrate_kbps = VBR_min_bitrate_kbps;
        return 0;
    }
    return -1;
}

/*! Get the lowest bitrate a variable bitrate encode may use. */
/*!
  \param gfp the encoder instance.
  \return the minimum in kbps: before \c lame_init_params() the value that
          was set, after it the bitrate from the table. 0 before
          \c lame_init_params() means that no minimum is set. 0 also if the
          instance is not usable.
*/
int
lame_get_VBR_min_bitrate_kbps(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->VBR_min_bitrate_kbps;
    }
    return 0;
}

/*! Set the highest bitrate a variable bitrate encode may use. */
/*!
  The highest bitrate per frame, in kbps. Use it to keep a VBR file below a
  size or within the limits of a decoder. **0 means that no maximum is set.**
  The encoder then allows the highest bitrate of the format: 320 kbps for
  MPEG-1, less for the lower sample rates.

  \c lame_init_params() changes it to the nearest bitrate of the table, as for
  the minimum. After that, the getter returns this bitrate. A maximum below
  what the quality level needs is not an error. The encoder keeps to it, and
  the quality is lower in passages that need more bits.

  \param gfp                   the encoder instance.
  \param VBR_max_bitrate_kbps  the maximum in kbps, or 0 for none. This
                               function does not check the value.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_VBR_max_bitrate_kbps(lame_global_flags * gfp, int VBR_max_bitrate_kbps)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->VBR_max_bitrate_kbps = VBR_max_bitrate_kbps;
        return 0;
    }
    return -1;
}

/*! Get the highest bitrate a variable bitrate encode may use. */
/*!
  \param gfp the encoder instance.
  \return the maximum in kbps: before \c lame_init_params() the value that
          was set, after it the bitrate from the table. 0 before
          \c lame_init_params() means that no maximum is set. 0 also if the
          instance is not usable.
*/
int
lame_get_VBR_max_bitrate_kbps(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->VBR_max_bitrate_kbps;
    }
    return 0;
}


/*! Make the minimum bitrate absolute. */
/*!
  By default, the encoder can go below the minimum bitrate of
  \c lame_set_VBR_min_bitrate_kbps(). It does this for silence and
  near-silence, which need very few bits. With this setting on, every frame
  uses at least the minimum bitrate, even a silent frame.

  Use it only when the file must meet an external minimum-bitrate rule.

  \param gfp           the encoder instance.
  \param VBR_hard_min  1 to keep the minimum in every frame, 0 to let silence
                       go below it. The default is 0.
  \return 0 on success. -1 if the value is not 0 or 1, or if the instance is
          not usable.
*/
int
lame_set_VBR_hard_min(lame_global_flags * gfp, int VBR_hard_min)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 0 (disabled) */

        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 > VBR_hard_min || 1 < VBR_hard_min)
            return -1;

        gfp->VBR_hard_min = VBR_hard_min;

        return 0;
    }
    return -1;
}

/*! Get whether the minimum bitrate is absolute. */
/*!
  \param gfp the encoder instance.
  \return 1 if every frame keeps the minimum, 0 if silence can go below it.
          0 also if the instance is not usable.
*/
int
lame_get_VBR_hard_min(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(0 <= gfp->VBR_hard_min && 1 >= gfp->VBR_hard_min);
        return gfp->VBR_hard_min;
    }
    return 0;
}


/********************************************************************
 * Filtering control
 ***********************************************************************/

/*! Set the lowpass cutoff. */
/*!
  LAME removes all frequencies above this cutoff before encoding. The saved
  bits go to the lower frequencies. At low bitrates the cutoff has a large
  effect on the sound: too low sounds dull, too high gives audible artifacts.

  - A positive value is the cutoff in Hz.
  - **0 means that LAME chooses** the cutoff, from the bitrate or, in the VBR
    modes, from the quality level.
  - -1 turns the lowpass off.

  The cutoff can change the sample rate. If no output sample rate is set, LAME
  chooses the lowest rate that still contains the cutoff. So a low cutoff can
  make LAME resample the output. \c lame_init_params() also limits the value:
  to half the output sample rate, and to 20500 Hz, or to 24000 Hz for
  \c vbr_mtrh. After that, the getter returns the limited value.

  \param gfp          the encoder instance.
  \param lowpassfreq  cutoff in Hz, 0 to let LAME choose, -1 for none. This
                      function does not check the value.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_lowpassfreq(lame_global_flags * gfp, int lowpassfreq)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->lowpassfreq = lowpassfreq;
        return 0;
    }
    return -1;
}

/*! Get the lowpass cutoff. */
/*!
  \param gfp the encoder instance.
  \return the cutoff in Hz: the value that was set before
          \c lame_init_params(), the cutoff that is used after it. 0 if the
          instance is not usable. 0 also means "LAME chooses".
*/
int
lame_get_lowpassfreq(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->lowpassfreq;
    }
    return 0;
}


/*! Set the width of the lowpass transition band. */
/*!
  How far below the cutoff the filter starts, in Hz. A negative value, the
  default, lets LAME choose.

  The filter works on the 32 bands of the polyphase filter bank. So it can
  only change at band boundaries. LAME rounds a width finer than one band to
  what the filter can do.

  \param gfp           the encoder instance.
  \param lowpasswidth  width in Hz, or a negative value to let LAME choose.
                       This function does not check the value.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_lowpasswidth(lame_global_flags * gfp, int lowpasswidth)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->lowpasswidth = lowpasswidth;
        return 0;
    }
    return -1;
}

/*! Get the width of the lowpass transition band. */
/*!
  \param gfp the encoder instance.
  \return the width in Hz that was set, or a negative value if LAME chooses.
          \c lame_init_params() does **not** change this value, so it does not
          show the width that the filter uses. 0 if the instance is not
          usable.
*/
int
lame_get_lowpasswidth(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->lowpasswidth;
    }
    return 0;
}


/*! Set the highpass cutoff. */
/*!
  LAME removes all frequencies below this cutoff. It is the opposite of
  \c lame_set_lowpassfreq(). It helps against rumble and DC offset. Otherwise
  leave it off, because the lowest octave contains much of the audible energy.

  There is **no automatic highpass**. Here the highpass differs from the
  lowpass. For the lowpass, 0 lets LAME choose a cutoff and -1 turns the
  filter off. For the highpass, **0, -1 and every other value up to 0 mean the
  same: no highpass**. The library never computes a highpass frequency itself.

  \param gfp           the encoder instance.
  \param highpassfreq  cutoff in Hz, or 0 (or -1) for none. This function does
                       not check the value.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_highpassfreq(lame_global_flags * gfp, int highpassfreq)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->highpassfreq = highpassfreq;
        return 0;
    }
    return -1;
}

/*! Get the highpass cutoff. */
/*!
  \param gfp the encoder instance.
  \return the cutoff in Hz, or 0 or -1 if no highpass is set. 0 also if the
          instance is not usable. \c lame_init_params() does not change this
          value.
*/
int
lame_get_highpassfreq(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->highpassfreq;
    }
    return 0;
}


/*! Set the width of the highpass transition band. */
/*!
  How far above the cutoff the filter ends, in Hz. A negative value, the
  default, lets LAME choose. LAME uses it only when a highpass cutoff is set.
  The same rounding to the polyphase bands applies as for
  \c lame_set_lowpasswidth().

  \param gfp            the encoder instance.
  \param highpasswidth  width in Hz, or a negative value to let LAME choose.
                        This function does not check the value.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_highpasswidth(lame_global_flags * gfp, int highpasswidth)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->highpasswidth = highpasswidth;
        return 0;
    }
    return -1;
}

/*! Get the width of the highpass transition band. */
/*!
  \param gfp the encoder instance.
  \return the width in Hz that was set, or a negative value if LAME chooses.
          0 if the instance is not usable.
*/
int
lame_get_highpasswidth(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->highpasswidth;
    }
    return 0;
}




/*
 * psychoacoustics and other arguments which you should not change
 * unless you know what you are doing
 */


/*!
  \internal
  \brief Shift the masking thresholds for long blocks.

  An offset in decibels for every masking threshold that the psychoacoustic
  model computes for a long block. A positive value tells the encoder that
  more noise is masked, so it uses fewer bits. A negative value makes it more
  careful, so it uses more bits.

  It changes the result of the whole psychoacoustic model. Change it only if
  you know the model well. The presets set it to fractions of a decibel.

  \param gfp     the encoder instance.
  \param adjust  offset in dB. The default is 0. This function does not check
                 the value.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_maskingadjust(lame_global_flags * gfp, float adjust)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->maskingadjust = adjust;
        return 0;
    }
    return -1;
}

/*!
  \internal
  \brief Get the long-block masking offset.

  \param gfp the encoder instance.
  \return the offset in dB. 0 if the instance is not usable. 0 is also the
          default and means no offset.
*/
float
lame_get_maskingadjust(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->maskingadjust;
    }
    return 0;
}

/*!
  \internal
  \brief Shift the masking thresholds for short blocks.

  As \c lame_set_maskingadjust(), for granules encoded as short blocks.
  Transients can hide a different amount of noise than steady material, so
  the presets set the two separately.

  \param gfp     the encoder instance.
  \param adjust  offset in dB. The default is 0. This function does not check
                 the value.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_maskingadjust_short(lame_global_flags * gfp, float adjust)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->maskingadjust_short = adjust;
        return 0;
    }
    return -1;
}

/*!
  \internal
  \brief Get the short-block masking offset.

  \param gfp the encoder instance.
  \return the offset in dB. 0 if the instance is not usable. 0 is also the
          default.
*/
float
lame_get_maskingadjust_short(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->maskingadjust_short;
    }
    return 0;
}

/*! Mask against the absolute threshold of hearing alone. */
/*!
  The absolute threshold of hearing (ATH) is the level below which a tone
  cannot be heard in a quiet room. Normally it is only the lower limit, and
  the masking that the psychoacoustic model computes from the signal does the
  real work. This setting ignores that masking and uses only the ATH. So the
  encoder does not hide noise under loud sounds.

  Use it for diagnosis, not for quality. Files get much worse at the same
  bitrate, and the LAME tag marks the encode as non-standard. It is useful to
  hear what the psychoacoustic model contributes.

  \param gfp      the encoder instance.
  \param ATHonly  non-zero to use only the ATH. The default is 0.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_ATHonly(lame_global_flags * gfp, int ATHonly)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->ATHonly = ATHonly;
        return 0;
    }
    return -1;
}

/*! Get whether only the ATH is used for masking. */
/*!
  \param gfp the encoder instance.
  \return the value that was set. 0 if the instance is not usable. 0 is also
          the default.
*/
int
lame_get_ATHonly(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->ATHonly;
    }
    return 0;
}


/*! Mask against the ATH alone, for short blocks only. */
/*!
  Like \c lame_set_ATHonly(), but only for granules encoded as short blocks,
  that is for the transients. Long blocks keep the full psychoacoustic model.

  This setting does **not** mark the encode as non-standard in the LAME tag,
  although it changes the audio.

  \param gfp       the encoder instance.
  \param ATHshort  non-zero to use only the ATH for short blocks. The default
                   is 0.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_ATHshort(lame_global_flags * gfp, int ATHshort)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->ATHshort = ATHshort;
        return 0;
    }
    return -1;
}

/*! Get whether short blocks use the ATH alone. */
/*!
  \param gfp the encoder instance.
  \return the value that was set. 0 if the instance is not usable. 0 is also
          the default.
*/
int
lame_get_ATHshort(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->ATHshort;
    }
    return 0;
}


/*! Drop the absolute threshold of hearing entirely. */
/*!
  Lowers the ATH to a level that no signal has, so the ATH does not act as a
  lower limit. Only the masking computed from the signal is left, and the
  encoder uses bits for detail below the threshold of hearing.

  It is the opposite of \c lame_set_ATHonly(), which keeps the ATH and ignores
  the masking. It is also only for diagnosis, because it makes files worse at
  every bitrate. It marks the encode as non-standard in the LAME tag.

  \param gfp    the encoder instance.
  \param noATH  non-zero to drop the ATH. The default is 0.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_noATH(lame_global_flags * gfp, int noATH)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->noATH = noATH;
        return 0;
    }
    return -1;
}

/*! Get whether the ATH is dropped. */
/*!
  \param gfp the encoder instance.
  \return the value that was set. 0 if the instance is not usable. 0 is also
          the default.
*/
int
lame_get_noATH(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->noATH;
    }
    return 0;
}


/*! Choose which formula produces the ATH curve. */
/*!
  LAME has six curves, numbered 0 to 5. They are variants of the same
  published equal-loudness approximation. They differ in how careful they are
  and in the frequency range they fit. In the current code, 4 and 5 use a
  shape parameter (\c lame_set_ATHcurve()), and the others do not.

  A higher number is not a better curve.

  The default is -1, which means "not set". \c lame_init_params() then chooses
  a formula. The choice can change between releases. Before
  \c lame_init_params() the getter returns -1. After it, the getter returns
  the formula in use. Read the getter. Do not assume a number.

  \param gfp      the encoder instance.
  \param ATHtype  the formula, 0 to 5, or -1 for the default.
  \return 0 on success. -1 for any other value, which leaves the setting as
          it was, or if the instance is not usable.
*/
int
lame_set_ATHtype(lame_global_flags * gfp, int ATHtype)
{
    if (is_lame_global_flags_valid(gfp)) {
        if (ATHtype < -1 || ATHtype > 5)
            return -1;
        /* XXX: ATHtype should be converted to an enum. */
        gfp->ATHtype = ATHtype;
        return 0;
    }
    return -1;
}

/*! Get the ATH formula. */
/*!
  \param gfp the encoder instance.
  \return the formula, or -1 if it is not set yet. 0 if the instance is not
          usable. 0 is also a valid formula.
*/
int
lame_get_ATHtype(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->ATHtype;
    }
    return 0;
}


/*!
  \internal
  \brief Set the shape parameter of the ATH curve.

  Changes the shape of the curve for the ATH formulas that read it, in the
  current code 4 and 5. It moves sensitivity between the ends and the middle
  of the spectrum. With a formula that ignores it, it has no effect and gives
  no message.

  The default is -1, which means "not set". \c lame_init_params() then chooses
  a shape. The value it chooses is not fixed, so read it back if you need it.

  \param gfp       the encoder instance.
  \param ATHcurve  the shape. This function does not check the value. Only the
                   formulas that use a shape read it.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_ATHcurve(lame_global_flags * gfp, float ATHcurve)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->ATHcurve = ATHcurve;
        return 0;
    }
    return -1;
}

/*!
  \internal
  \brief Get the shape parameter of the ATH curve.

  \param gfp the encoder instance.
  \return the shape, or -1 if it is not set yet. 0 if the instance is not
          usable.
*/
float
lame_get_ATHcurve(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->ATHcurve;
    }
    return 0;
}


/*! Lower the whole ATH curve. */
/*!
  Lowers the threshold by this many decibels. The encoder then treats quieter
  sounds as audible and codes them. Files get larger, and more of the very
  quiet detail is kept. A negative value raises the curve and does the
  opposite.

  It applies to the whole curve, whichever formula made it.

  \param gfp       the encoder instance.
  \param ATHlower  how far to lower the curve, in dB. The default is 0. A
                   finite number. This function does not check the range.
  \return 0 on success. -1 if the value is NaN or infinite, or if the instance
          is not usable. The setting does not change then.
*/
int
lame_set_ATHlower(lame_global_flags * gfp, float ATHlower)
{
    if (is_lame_global_flags_valid(gfp) && float_is_finite(ATHlower)) {
        gfp->ATH_lower_db = ATHlower;
        return 0;
    }
    return -1;
}

/*! Get how far the ATH curve is lowered. */
/*!
  \param gfp the encoder instance.
  \return the shift in dB. 0 if the instance is not usable. 0 is also the
          default and means no shift.
*/
float
lame_get_ATHlower(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->ATH_lower_db;
    }
    return 0;
}


/*! Select the adaptive ATH scheme. */
/*!
  The adaptive adjustment moves the threshold with the loudness of the
  material. A listener turns a quiet passage up and a loud one down, so the
  quiet passage needs more careful coding. It is on by default.

  **0 turns the adjustment off, and every other value turns it on.** So this
  is a flag, although it selects a "scheme". The default is -1, which means
  "not set". \c lame_init_params() then chooses a scheme. The choice can
  change between releases. Before \c lame_init_params() the getter returns -1.
  After it, the getter returns the scheme in use. Read the getter. Do not
  assume a number.

  \param gfp         the encoder instance.
  \param athaa_type  0 to turn the adjustment off, non-zero to turn it on.
                     This function does not check the value.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_athaa_type(lame_global_flags * gfp, int athaa_type)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->athaa_type = athaa_type;
        return 0;
    }
    return -1;
}

/*! Get the adaptive ATH scheme. */
/*!
  \param gfp the encoder instance.
  \return the scheme in use, or -1 if it is not set yet. 0 if the instance is
          not usable. 0 also means "off", so an instance that is not usable
          looks as if the adjustment were off.
*/
int
lame_get_athaa_type(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->athaa_type;
    }
    return 0;
}



/*! Select the loudness approximation the adaptive ATH uses. */
/*!
  \deprecated This function does nothing. Only one approximation is left, so
  there is nothing to select. lame.h does not declare this function. The
  library still exports it, so that programs built against an older release
  still link.

  This function always returns 0, even for an instance that is not usable.

  \param gfp               ignored.
  \param athaa_loudapprox  ignored.
  \return always 0.
*/
int
lame_set_athaa_loudapprox(lame_global_flags * gfp, int athaa_loudapprox)
{
    (void) gfp;
    (void) athaa_loudapprox;
    return 0;
}

/*! Get the loudness approximation the adaptive ATH uses. */
/*!
  \deprecated See \c lame_set_athaa_loudapprox().
  \param gfp  ignored.
  \return always 2, the number of the approximation that is left.
*/
int
lame_get_athaa_loudapprox(const lame_global_flags * gfp)
{
    (void) gfp;
    /* obsolete, the type known under number 2 is the only survival */
    return 2;
}


/*! Shift the loudness at which the adaptive ATH starts adjusting. */
/*!
  The adaptive adjustment of \c lame_set_athaa_type() starts only below a
  certain loudness. This setting moves that point, in decibels. A positive
  value makes the encoder start sooner, so more material counts as quiet.

  LAME reads it only when the adaptive adjustment is on.

  \param gfp                 the encoder instance.
  \param athaa_sensitivity   the shift in dB. The default is 0, which means no
                             shift. A finite number. This function does not
                             check the range.
  \return 0 on success. -1 if the value is NaN or infinite, or if the instance
          is not usable. The setting does not change then.
*/
int
lame_set_athaa_sensitivity(lame_global_flags * gfp, float athaa_sensitivity)
{
    if (is_lame_global_flags_valid(gfp) && float_is_finite(athaa_sensitivity)) {
        gfp->athaa_sensitivity = athaa_sensitivity;
        return 0;
    }
    return -1;
}

/*! Get the adaptive ATH sensitivity shift. */
/*!
  \param gfp the encoder instance.
  \return the shift in dB. 0 if the instance is not usable. 0 is also the
          default and means no shift.
*/
float
lame_get_athaa_sensitivity(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->athaa_sensitivity;
    }
    return 0;
}


/*! Set the predictability limit of the ISO tonality formula. */
/*!
  \deprecated This function does nothing. LAME does not use the ISO tonality
  formula, and nothing reads this value. lame.h does not declare this
  function. The library still exports it, so that programs built against an
  older release still link.

  \param gfp      ignored.
  \param cwlimit  ignored.
  \return always 0.
*/
int
lame_set_cwlimit(lame_global_flags * gfp, int cwlimit)
{
    (void) gfp;
    (void) cwlimit;
    return 0;
}

/*! Get the predictability limit. */
/*!
  \deprecated See \c lame_set_cwlimit().
  \param gfp  ignored.
  \return always 0.
*/
int
lame_get_cwlimit(const lame_global_flags * gfp)
{
    (void) gfp;
    return 0;
}



/*! Allow the two channels to use different block types. */
/*!
  When a transient is in one channel only, LAME can code that channel with
  short blocks and the other with long blocks. This follows the material more
  closely. Coupling the two keeps both channels at the same block type.

  This function, \c lame_set_no_short_blocks() and
  \c lame_set_force_short_blocks() write the same setting. The last call sets
  the value.

  **For stereo and joint stereo, \c lame_init_params() ignores this setting**
  and gives both channels the same block type. After \c lame_init_params() the
  getter then returns 0.

  \param gfp               the encoder instance.
  \param allow_diff_short  non-zero to allow the channels to differ, 0 to
                           couple them.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_allow_diff_short(lame_global_flags * gfp, int allow_diff_short)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->short_blocks = allow_diff_short ? short_block_allowed : short_block_coupled;
        return 0;
    }
    return -1;
}

/*! Get whether the channels may use different block types. */
/*!
  \param gfp the encoder instance.
  \return 1 only if the block type setting is "may differ". 0 for every other
          state, including "short blocks forced" and "no short blocks". This
          getter cannot tell these apart. 0 also if the instance is not
          usable.
*/
int
lame_get_allow_diff_short(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        if (gfp->short_blocks == short_block_allowed)
            return 1;   /* short blocks allowed to differ */
        else
            return 0;   /* not set, dispensed, forced or coupled */
    }
    return 0;
}


/*! Use temporal masking. */
/*!
  Temporal masking: a loud sound hides quieter sounds shortly before and
  after it, not only at the same time. With this setting, the encoder can be
  less careful around transients, where the ear notices the least.

  It is on by default, except with \c vbr_mtrh, where it is off unless it is
  set. To use it in every mode, set it.

  The "not set" state, which gives the mode-dependent default, **cannot be set
  with this function**. It accepts only 0 and 1, so after a call the default
  cannot be restored.

  \param gfp          the encoder instance.
  \param useTemporal  1 to use temporal masking, 0 not to.
  \return 0 on success. -1 if the value is not 0 or 1, or if the instance is
          not usable.
*/
int
lame_set_useTemporal(lame_global_flags * gfp, int useTemporal)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 1 (enabled) */

        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 <= useTemporal && useTemporal <= 1) {
            gfp->useTemporal = useTemporal;
            return 0;
        }
    }
    return -1;
}

/*! Get whether temporal masking is used. */
/*!
  \param gfp the encoder instance.
  \return 1 or 0 after a value was chosen, by this setter or by
          \c lame_init_params(). **-1 before that**, which means "not set". 0
          if the instance is not usable. So an instance that is not usable
          cannot be told apart from a 0 that was set.
*/
int
lame_get_useTemporal(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* -1 is the "not chosen yet" marker lame_init() stores and
           lame_init_params() resolves, so it is a legal value to be asked for
           before initialization - as lame_get_interChRatio() already allows
           for its own. */
        assert((0 <= gfp->useTemporal && 1 >= gfp->useTemporal)
               || -1 == gfp->useTemporal);
        return gfp->useTemporal;
    }
    return 0;
}


/*! Let one channel mask the other. */
/*!
  Mixes part of the masking threshold of each channel into the other channel.
  A listener hears both channels together. 0 keeps the channels separate. 1
  gives the threshold of the other channel full weight.

  The default is off. As with \c lame_set_useTemporal(), the internal "not
  set" state cannot be restored after a value is set.

  \param gfp    the encoder instance.
  \param ratio  0 to 1.
  \return 0 on success. -1 if \a ratio is not in the range 0 to 1, or if the
          instance is not usable.
*/
int
lame_set_interChRatio(lame_global_flags * gfp, float ratio)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 0.0 (no inter-channel maskin) */
        if (0 <= ratio && ratio <= 1.0) {
            gfp->interChRatio = ratio;
            return 0;
        }
    }
    return -1;
}

/*! Get the inter-channel masking ratio. */
/*!
  \param gfp the encoder instance.
  \return the ratio, 0 to 1. -1 if nothing is set yet and
          \c lame_init_params() has not set it to 0. 0 if the instance is not
          usable. 0 is also the default that \c lame_init_params() sets.
*/
float
lame_get_interChRatio(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert((0 <= gfp->interChRatio && gfp->interChRatio <= 1.0) || EQ(gfp->interChRatio, -1.0));
        return gfp->interChRatio;
    }
    return 0;
}


/*!
  \internal
  \brief Enable pseudo substep noise shaping.

  Substep shaping drops spectral lines whose contribution is too small to be
  worth the bits. It also codes some bands at half the step of the
  scalefactor, where the full step would waste bits. It gives a little more
  quality at the same bitrate, and takes some extra encoding time.

  This is also a packed value, not a method number:

  | bit | meaning |
  |-----|---------|
  | 0   | apply the shaping |
  | 1   | start every band in half-step mode instead of deciding per band |
  | 2   | extend the shaping to short blocks, which are otherwise skipped |

  The default is off. The quality settings 0 to 2 (\c lame_set_quality()) turn
  on bit 1 when the caller left this at 0. The values 0 to 7 are accepted,
  that is every combination of the three bits.

  \param gfp     the encoder instance.
  \param method  the bit combination, 0 to 7.
  \return 0 on success. -1 if \a method is not in the range 0 to 7, or if the
          instance is not usable.
*/
int
lame_set_substep(lame_global_flags * gfp, int method)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* default = 0.0 (no substep noise shaping) */
        if (0 <= method && method <= 7) {
            gfp->substep_shaping = method;
            return 0;
        }
    }
    return -1;
}

/*!
  \internal
  \brief Get the substep noise shaping setting.

  \param gfp the encoder instance.
  \return the bit combination, 0 to 7. 0 if the instance is not usable. 0 is
          also the default and means that the shaping is off.
*/
int
lame_get_substep(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(0 <= gfp->substep_shaping && gfp->substep_shaping <= 7);
        return gfp->substep_shaping;
    }
    return 0;
}

/*!
  \internal
  \brief Use the finer scalefactor scale.

  MP3 has two step sizes for the scalefactors that shape the noise. The finer
  step lets the encoder place the quantization noise more precisely, but each
  scalefactor then needs one more bit.

  Despite its name, this is a choice between the two noise-shaping variants of
  the library. \c lame_set_quality() comes first: at quality levels 8 and 9,
  noise shaping is off, and \c lame_init_params() ignores this setting.

  \param gfp  the encoder instance.
  \param val  non-zero for the finer scale, 0 for the coarser one. The default
              is 0.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_sfscale(lame_global_flags * gfp, int val)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->noise_shaping = (val != 0) ? 2 : 1;
        return 0;
    }
    return -1;
}

/*!
  \internal
  \brief Get whether the finer scalefactor scale is used.

  \param gfp the encoder instance.
  \return 1 if the finer scale is selected, 0 if not. 0 also when noise
          shaping is off, which this getter cannot tell apart from the coarser
          scale. 0 if the instance is not usable.
*/
int
lame_get_sfscale(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return (gfp->noise_shaping == 2) ? 1 : 0;
    }
    return 0;
}

/*!
  \internal
  \brief Allow the outer loop to raise the gain of individual sub-blocks.

  A short-block granule has three sub-blocks, and each can have its own gain
  offset. If the noise-shaping loop may use them, it helps where a transient
  puts very different energy into the three. That is when LAME chooses short
  blocks.

  Only positive values turn it on. -1, the default, leaves the choice to
  \c lame_set_quality(), which turns it on at every quality level that uses
  noise shaping.

  \param gfp     the encoder instance.
  \param sbgain  positive to allow it, 0 to forbid it, -1 to let the quality
                 level choose. This function does not check the value.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_subblock_gain(lame_global_flags * gfp, int sbgain)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->subblock_gain = sbgain;
        return 0;
    }
    return -1;
}

/*!
  \internal
  \brief Get whether sub-block gains may be used.

  \param gfp the encoder instance.
  \return the value that was set, or -1 if the quality level chooses. 0 if the
          instance is not usable, which looks like "no".
*/
int
lame_get_subblock_gain(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->subblock_gain;
    }
    return 0;
}


/*! Encode everything in long blocks. */
/*!
  At a transient, the encoder switches to short blocks. They have less
  frequency resolution and more time resolution, so quantization noise cannot
  spread back into the silence before a drum hit. This setting forbids them,
  and attacks then sound smeared. It is for a few old decoders that handle
  short blocks badly.

  This function, \c lame_set_allow_diff_short() and
  \c lame_set_force_short_blocks() write one block type setting. The last call
  decides. Setting 0 here means "short blocks allowed", and overwrites what
  the other two set before.

  \param gfp              the encoder instance.
  \param no_short_blocks  1 to encode everything in long blocks, 0 to allow
                          short blocks.
  \return 0 on success. -1 if the value is not 0 or 1, or if the instance is
          not usable.
*/
int
lame_set_no_short_blocks(lame_global_flags * gfp, int no_short_blocks)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 <= no_short_blocks && no_short_blocks <= 1) {
            gfp->short_blocks = no_short_blocks ? short_block_dispensed : short_block_allowed;
            return 0;
        }
    }
    return -1;
}

/*! Get whether short blocks are forbidden. */
/*!
  \param gfp the encoder instance.
  \return 1 if short blocks are forbidden, 0 if they are allowed in any form.
          **-1 if nothing is set yet**, and also -1 if the instance is not
          usable. This is one of the few getters here that does not return 0
          in that case.
*/
int
lame_get_no_short_blocks(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        switch (gfp->short_blocks) {
        default:
        case short_block_not_set:
            return -1;
        case short_block_dispensed:
            return 1;
        case short_block_allowed:
        case short_block_coupled:
        case short_block_forced:
            return 0;
        }
    }
    return -1;
}


/*! Encode everything in short blocks. */
/*!
  The opposite of \c lame_set_no_short_blocks(): every granule is coded in
  short blocks, with or without a transient. Steady material loses frequency
  resolution and the encode gets worse, so this is a setting for tests.

  Turning it **off** works differently. Passing 0 has an effect only if short
  blocks were forced. Then the setting goes back to "allowed". If another
  block type choice is set, passing 0 does not change it. Here the three
  functions do not simply overwrite each other.

  \param gfp           the encoder instance.
  \param short_blocks  1 to force short blocks, 0 to stop forcing them.
  \return 0 on success. -1 if the value is not 0 or 1, or if the instance is
          not usable.
*/
int
lame_set_force_short_blocks(lame_global_flags * gfp, int short_blocks)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* enforce disable/enable meaning, if we need more than two values
           we need to switch to an enum to have an apropriate representation
           of the possible meanings of the value */
        if (0 > short_blocks || 1 < short_blocks)
            return -1;

        if (short_blocks == 1)
            gfp->short_blocks = short_block_forced;
        else if (gfp->short_blocks == short_block_forced)
            gfp->short_blocks = short_block_allowed;

        return 0;
    }
    return -1;
}

/*! Get whether short blocks are forced. */
/*!
  \param gfp the encoder instance.
  \return 1 if short blocks are forced, 0 for any other block type choice. -1
          if nothing is set yet, and also -1 if the instance is not usable.
*/
int
lame_get_force_short_blocks(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        switch (gfp->short_blocks) {
        default:
        case short_block_not_set:
            return -1;
        case short_block_dispensed:
        case short_block_allowed:
        case short_block_coupled:
            return 0;
        case short_block_forced:
            return 1;
        }
    }
    return -1;
}

/*!
  \internal
  \brief Set the attack threshold for the left, right and mid channels.

  How sharp a rise in energy counts as an attack. An attack makes the encoder
  switch the granule to short blocks. A lower value switches more often, which
  keeps transients sharp and costs bits. A higher value switches less often.

  A negative value, the default, keeps LAME's own threshold.

  \param gfp  the encoder instance.
  \param lrm  the threshold, or a negative value for LAME's own. This function
              does not check the value.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_short_threshold_lrm(lame_global_flags * gfp, float lrm)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->attackthre = lrm;
        return 0;
    }
    return -1;
}

/*!
  \internal
  \brief Get the attack threshold for the left, right and mid channels.

  \param gfp the encoder instance.
  \return the threshold, or a negative value if LAME's own is used. LAME never
          writes the value it uses into this setting. 0 if the instance is not
          usable.
*/
float
lame_get_short_threshold_lrm(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->attackthre;
    }
    return 0;
}

/*!
  \internal
  \brief Set the attack threshold for the side channel.

  As \c lame_set_short_threshold_lrm(), for the side channel of a mid/side
  encode. The side channel usually has much less energy, so the same
  threshold would mean something different there.

  \param gfp  the encoder instance.
  \param s    the threshold, or a negative value for LAME's own. This function
              does not check the value.
  \return 0 on success. -1 if the instance is not usable.
*/
int
lame_set_short_threshold_s(lame_global_flags * gfp, float s)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->attackthre_s = s;
        return 0;
    }
    return -1;
}

/*!
  \internal
  \brief Get the attack threshold for the side channel.

  \param gfp the encoder instance.
  \return the threshold, or a negative value if LAME's own is used. 0 if the
          instance is not usable.
*/
float
lame_get_short_threshold_s(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->attackthre_s;
    }
    return 0;
}

/*!
  \internal
  \brief Set both attack thresholds.

  Sets both thresholds in one call: \c lame_set_short_threshold_lrm() and
  \c lame_set_short_threshold_s(). There is no matching getter. Read the two
  values separately.

  \param gfp  the encoder instance.
  \param lrm  threshold for the left, right and mid channels.
  \param s    threshold for the side channel.
  \return 0 on success. -1 if the instance is not usable. The two inner calls
          cannot fail for a usable instance, so their results are not
          checked.
*/
int
lame_set_short_threshold(lame_global_flags * gfp, float lrm, float s)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_set_short_threshold_lrm(gfp, lrm);
        lame_set_short_threshold_s(gfp, s);
        return 0;
    }
    return -1;
}


/*! Declare that the input is pre-emphasized. */
/*!
  A frame header field that says the audio has a treble boost, which a decoder
  should undo. It comes from a few early CDs and is almost never used.

  **Do not use this.** LAME does not apply the emphasis. It only writes the
  field, so the input must already be emphasized. The psychoacoustic model
  does not take the emphasis into account, so the encode is tuned for the
  wrong spectrum. Many decoders ignore the field, so the listener hears the
  boosted treble.

  The values are those of the two-bit field: 0 none, 1 the 50/15 microsecond
  curve, 2 reserved, 3 the CCITT J.17 curve. All four are accepted, also the
  reserved one.

  \param gfp       the encoder instance.
  \param emphasis  0 to 3.
  \return 0 on success. -1 if \a emphasis is not in the range 0 to 3, or if
          the instance is not usable.
*/
int
lame_set_emphasis(lame_global_flags * gfp, int emphasis)
{
    if (is_lame_global_flags_valid(gfp)) {
        /* XXX: emphasis should be converted to an enum */
        if (0 <= emphasis && emphasis < 4) {
            gfp->emphasis = emphasis;
            return 0;
        }
    }
    return -1;
}

/*! Get the emphasis declaration. */
/*!
  \param gfp the encoder instance.
  \return 0 to 3. 0 if the instance is not usable. 0 is also the default and
          means no emphasis.
*/
int
lame_get_emphasis(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        assert(0 <= gfp->emphasis && gfp->emphasis < 4);
        return gfp->emphasis;
    }
    return 0;
}




/***************************************************************/
/* internal variables, cannot be set...                        */
/* provided because they may be of use to calling application  */
/***************************************************************/

/*! Get the MPEG version that the encoder uses. */
/*!
  The caller does not choose it. It follows from the output sample rate, so it
  has a meaning only after \c lame_init_params().

  0 is MPEG-2, 1 is MPEG-1, 2 is MPEG-2.5. This is not the numbering of the
  frame header, and not the version of the library (\c get_lame_version()).

  \param gfp the encoder instance.
  \return 0, 1 or 2. 0 if the instance is not initialized, which cannot be
          told apart from an MPEG-2 encode.
*/
int
lame_get_version(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            return gfc->cfg.version;
        }
    }
    return 0;
}


/*! Get the number of samples the encoder prepended. */
/*!
  The filter bank needs samples before the ones it outputs, so the encoded
  stream starts with silence that was not in the input. To restore the
  original timing, a decoder removes this many samples from the start.

  The LAME tag also contains this value, so a gapless decoder can read it from
  the file.

  \param gfp the encoder instance.
  \return the delay in samples per channel. 0 if the instance is not
          initialized.
*/
int
lame_get_encoder_delay(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            return gfc->ov_enc.encoder_delay;
        }
    }
    return 0;
}

/*! Get the number of samples the encoder appended. */
/*!
  The same as \c lame_get_encoder_delay(), at the end of the stream. A frame
  has a fixed number of samples, so the last frame is filled with silence. To
  restore the original length, a decoder removes this many samples from the
  end.

  The value is known only after \c lame_encode_flush(), which writes the last
  frame.

  \param gfp the encoder instance.
  \return the padding in samples per channel. 0 if the instance is not
          initialized.
*/
int
lame_get_encoder_padding(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            return gfc->ov_enc.encoder_padding;
        }
    }
    return 0;
}


/*! Get how many samples one frame holds. */
/*!
  1152 for MPEG-1, 576 for MPEG-2 and MPEG-2.5. An MPEG-1 frame has two
  granules, the others have one. This is a number of samples per channel, not
  a size in bytes.

  \param gfp the encoder instance.
  \return 1152 or 576. 0 if the instance is not initialized.
*/
int
lame_get_framesize(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            SessionConfig_t const *const cfg = &gfc->cfg;
            return SAMPLES_PER_GRANULE * cfg->mode_gr;
        }
    }
    return 0;
}


/*! Get how many frames have been written so far. */
/*!
  The value grows during the encode. So it is the natural value for a
  progress display, with \c lame_get_totalframes() when the input length is
  known.

  \param gfp the encoder instance.
  \return the number of frames written. 0 before the first frame, and also 0
          if the instance is not initialized.
*/
int
lame_get_frameNum(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            return gfc->ov_enc.frame_number;
        }
    }
    return 0;
}

/*! Get how many samples are still held inside the encoder. */
/*!
  Samples that were passed in but are not yet part of a frame. The filter bank
  needs a whole frame plus a look-ahead before it can output a frame. The
  value is non-zero at any time during the encode. This is why a caller must
  call \c lame_encode_flush() at the end.

  \param gfp the encoder instance.
  \return the number of buffered samples per channel. 0 if the instance is not
          initialized.
*/
int
lame_get_mf_samples_to_encode(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            return gfc->sv_enc.mf_samples_to_encode;
        }
    }
    return 0;
}

/*! Get how many bytes of MP3 data the encoder still holds. */
/*!
  The number of bytes of MP3 data that the encoder has produced but not yet
  returned. \c lame_encode_flush_nogap() returns this many bytes.
  \c lame_encode_flush() returns more, because it first encodes the PCM
  samples that are still in the buffer.

  It is computed on each call, which takes a little time. The value changes
  during the encode, as the bit reservoir fills and empties.

  \param gfp the encoder instance.
  \return the number of bytes. 0 if the instance is not initialized.
*/
int     CDECL
lame_get_size_mp3buffer(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            int     size;
            compute_flushbits(gfc, &size);
            return size;
        }
    }
    return 0;
}

/*! Get the ReplayGain figure for this track. */
/*!
  How much the track should be turned up or down to match a common playback
  loudness, **in tenths of a decibel**. 45 means 4.5 dB up. The value is
  available only if \c lame_set_findReplayGain() was on, and only after the
  whole track is encoded, because it describes the whole track.

  LAME writes it into the LAME tag itself, so a caller usually needs it only
  to report it.

  \param gfp the encoder instance.
  \return the gain in tenths of a dB. 0 if the analysis did not run, if the
          track was too short, or if the instance is not initialized. These
          cases cannot be told apart from a real 0.
*/
int
lame_get_RadioGain(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            return gfc->ov_rpg.RadioGain;
        }
    }
    return 0;
}

/*! Get the album ReplayGain figure. */
/*!
  This is **always 0**. The album gain keeps a whole album at one loudness,
  instead of each track separately. An encoder that sees one track cannot
  compute it, so LAME does not produce it. The function exists because the
  LAME tag has a field for it.

  \param gfp the encoder instance.
  \return always 0.
*/
int
lame_get_AudiophileGain(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            return 0;
        }
    }
    return 0;
}

/*! Get the loudest sample seen. */
/*!
  The largest absolute sample value, on a scale where full scale is 32767. So
  a value above that means that the material clips on playback. It needs
  \c lame_set_decode_on_the_fly(), which makes the encoder decode its own
  output. So it needs a LAME built with the decoder, because a build without
  one rejects that setting.

  \param gfp the encoder instance.
  \return the peak. 0 if the measurement is not on, or if the instance is not
          initialized.
*/
float
lame_get_PeakSample(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            return (float) gfc->ov_rpg.PeakSample;
        }
    }
    return 0;
}

/*! Get the gain change that would avoid clipping. */
/*!
  How much the ReplayGain value must be lowered so that playback never clips,
  in tenths of a decibel, rounded up. A positive value means that the decoded
  stream goes above full scale. Zero or negative means that it does not.

  It comes from \c lame_get_PeakSample(), so it needs the same measurement.

  For material where all samples are zero, there is no peak, and the value is
  0. This is the same as for material that reaches exactly full scale, and for
  material that was not measured. To tell these apart, read
  \c lame_get_PeakSample(), which needs a LAME built with the decoder.

  \param gfp the encoder instance.
  \return the change in tenths of a dB. 0 if the measurement is not on, or if
          the instance is not initialized.
*/
int
lame_get_noclipGainChange(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            return gfc->ov_rpg.noclipGainChange;
        }
    }
    return 0;
}

/*! Get the scale factor that would avoid clipping. */
/*!
  The factor for the input of a new encode, so that the decoded output no
  longer goes above full scale. It is rounded down to two decimals, so it is
  safe, but not exact.

  A value of **-1 means that no scaling is needed**. It is also the value
  before any measurement, and for material where all samples are zero. These
  three cases cannot be told apart.

  \param gfp the encoder instance.
  \return the factor, or -1 if the material does not clip. -1 also if the
          measurement is not on. 0 if the instance is not initialized.
*/
float
lame_get_noclipScale(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            return gfc->ov_rpg.noclipScale;
        }
    }
    return 0;
}


/*! Get the estimated number of frames the encode will produce. */
/*!
  LAME computes it from the input length of \c lame_set_num_samples(), the
  output sample rate and the frame size, plus the padding at the end. It is an
  estimate: it assumes that the input length is right. It does not change
  during the encode.

  A return of **0 means that the total is not known**. An instance whose input
  length was never set returns 0, because the default length means "not
  known", not a real count. Check for 0 before you divide a frame count by it.

  \param gfp the encoder instance.
  \return the estimated number of frames. 0 if it cannot be estimated, or if
          the instance is not initialized.
*/
int
lame_get_totalframes(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            SessionConfig_t const *const cfg = &gfc->cfg;
            unsigned long const pcm_samples_per_frame = 576ul * cfg->mode_gr;
            unsigned long pcm_samples_to_encode = gfp->num_samples;
            unsigned long end_padding = 0;
            int frames = 0;

            /* compare against the documented unknown sentinel (lame.h:
               default = 2^32-1); the (0ul-1ul) test alone matches it only
               where long is 32-bit, and is kept to preserve existing LP64
               behavior */
            if (pcm_samples_to_encode == MAX_U_32_NUM
                || pcm_samples_to_encode == (0ul-1ul))
                return 0; /* unknown */

            /* estimate based on user set num_samples: */
            if (cfg->samplerate_in != cfg->samplerate_out) {
                /* resampling, estimate new samples_to_encode */
                double resampled_samples_to_encode = 0.0, frames_f = 0.0;
                if (cfg->samplerate_in > 0) {
                    resampled_samples_to_encode = pcm_samples_to_encode;
                    resampled_samples_to_encode *= cfg->samplerate_out;
                    resampled_samples_to_encode /= cfg->samplerate_in;
                }
                if (resampled_samples_to_encode <= 0.0)
                    return 0; /* unlikely to happen, so what, no estimate! */
                frames_f = floor(resampled_samples_to_encode / pcm_samples_per_frame);
                if (frames_f >= (INT_MAX-2))
                    return 0; /* overflow, happens eventually, no estimate! */
                frames = frames_f;
                resampled_samples_to_encode -= frames * pcm_samples_per_frame;
                pcm_samples_to_encode = ceil(resampled_samples_to_encode);
            }
            else {
                if (pcm_samples_to_encode / pcm_samples_per_frame >= (unsigned long)(INT_MAX-2))
                    return 0; /* overflow, happens eventually, no estimate! */
                frames = pcm_samples_to_encode / pcm_samples_per_frame;
                pcm_samples_to_encode -= frames * pcm_samples_per_frame;
            }
            pcm_samples_to_encode += 576ul;
            end_padding = pcm_samples_per_frame - (pcm_samples_to_encode % pcm_samples_per_frame);
            if (end_padding < 576ul) {
                end_padding += pcm_samples_per_frame;
            }
            pcm_samples_to_encode += end_padding;
            frames += (pcm_samples_to_encode / pcm_samples_per_frame);
            /* check to see if we underestimated totalframes */
            /*    if (totalframes < gfp->frameNum) */
            /*        totalframes = gfp->frameNum; */
            return frames;
        }
    }
    return 0;
}





/*! Apply a preset. */
/*!
  A preset is a set of the settings above that were tuned together by
  listening tests. Using a preset is the recommended way to configure the
  encoder. The settings affect each other, and setting them one by one can
  give a combination that nobody has tested.

  Three kinds of value are accepted:

  - **8 to 320**: an average bitrate in kbps. The preset selects the ABR mode
    and tunes it for that bitrate.
  - **the quality constants** \c V0 to \c V9 (also \c VBR_100 down to
    \c VBR_10): variable bitrate at that quality level. \c V0 is the best.
  - **the named presets** \c STANDARD, \c EXTREME, \c INSANE, \c MEDIUM, their
    \c _FAST variants and \c R3MIX. They stay for compatibility with the
    command-line options of the same names.

  This function writes all the settings of the preset at once. It overwrites
  earlier calls for the same settings. Later calls overwrite the preset. So
  set the preset first, then change single settings.

  A value that is neither a bitrate in the range nor one of the constants is
  rejected: the call changes nothing and returns -1.

  \param gfp     the encoder instance.
  \param preset  a bitrate, a \c preset_mode value, or one of the named
                 presets.
  \return the preset that was applied, not 0. For a named preset this is the
          value it stands for, for example \c V2 for \c STANDARD and 320 for
          \c INSANE, otherwise \a preset itself. -1 for a value that is no
          preset, or if the instance is not usable.
*/
int
lame_set_preset(lame_global_flags * gfp, int preset)
{
    if (is_lame_global_flags_valid(gfp)) {
        if (!preset_is_known(preset))
            return -1;
        gfp->preset = preset;
        return apply_preset(gfp, preset, 1);
    }
    return -1;
}



/*! Allow or forbid one family of processor instructions. */
/*!
  \deprecated Use \c lame_set_vector_routines().

  Every family is allowed by default. LAME uses a family only if the processor
  has it. So this function can *forbid* an instruction set, but not require
  one. Forbidding one helps to compare the hand-written routines with the plain
  C code, and to work around a processor whose implementation of an
  instruction set is not reliable.

  The families are the values of \c asm_optimizations. They name x86
  instruction sets and mean nothing on other architectures. There the call is
  accepted and changes nothing. The list of families depends on the release,
  so read it from the enum.

  The families depend on each other. \c AVX2 uses the SSE2 routines, so
  forbidding \c SSE also forbids \c AVX2. Forbidding only \c AVX2 keeps the
  SSE tier.

  \c MMX and \c AMD_3DNOW are different. This library has no MMX or 3DNow!
  code, so there is nothing to allow or forbid. The function returns -2 for
  them and stores nothing.

  \since \c AVX2 is available from LAME 4.1, with the vectorized inner loops.
         The other families are older than this interface. Source code that
         must also compile with a 3.100 or older header cannot use the name
         \c AVX2, but it can pass the value. A library that does not know the
         family accepts the call and does nothing.

  \param gfp    the encoder instance.
  \param optim  the family, one of the \c asm_optimizations values.
  \param mode   1 to allow the family, anything else to forbid it.
  \return -1 if the instance is not usable. -2 for \c MMX and \c AMD_3DNOW.
          Otherwise \a optim, also for a value this library does not know. So
          the return value cannot tell you whether the family is known. Use
          \c lame_get_asm_optimizations() to check.

  \see lame_get_asm_optimizations()

  \code{.c}
  // Forbid the SSE tier, for example to compare the hand-written routines with the C.
  if (lame_set_asm_optimizations(gfp, SSE, 0) == -1)
      return -1;    // the instance is unusable; nothing was set

  // Any other return means the call was accepted. It does not mean this build
  // has SSE routines, and it does not mean the processor has SSE.
  \endcode
*/
int
lame_set_asm_optimizations(lame_global_flags * gfp, int optim, int mode)
{
    if (is_lame_global_flags_valid(gfp)) {
        mode = (mode == 1 ? 1 : 0);
        switch (optim) {
        case MMX:
        case AMD_3DNOW:
            /* Nothing to allow or forbid: this library has no MMX or 3DNow!
               code. Answering -2 rather than storing the flag tells a caller
               who checks that the request was not acted on, instead of
               reporting back a setting that selects nothing. */
            return -2;
        case SSE:{
                gfp->asm_optimizations.sse = mode;
                return optim;
            }
        case AVX2:{
                gfp->asm_optimizations.avx2 = mode;
                return optim;
            }
        default:
            return optim;
        }
    }
    return -1;
}

/*! Get whether an instruction-set family may be used. */
/*!
  \deprecated Use \c lame_get_vector_routines().

  Returns the setting for the one family named, in the form that
  \c lame_set_asm_optimizations() takes. A family that controls something is
  allowed at the start, so a new instance returns 1 for \c SSE and \c AVX2.
  \c MMX and \c AMD_3DNOW always return 0. This library has no code for them,
  so they are never on.

  This returns the **flag**, not the instruction set that an encode uses. The
  families depend on each other: \c AVX2 uses the SSE2 routines. So with
  \c SSE forbidden, the instance does not use \c AVX2 either, but this
  function still returns 1 for \c AVX2. An allowed family also does not mean
  that the build has those routines, or that the processor has that
  instruction set. The lame tool prints the routines that were used with
  \c --verbose.

  \since LAME 4.1.

  \param gfp    the encoder instance.
  \param optim  the family, one of the \c asm_optimizations values.
  \return 1 if the family may be used. 0 if it is forbidden, or if this
          library has no code for it. -1 if the instance is not usable. -2 if
          the family is not one this interface knows.
          \c lame_set_asm_optimizations() cannot report this last case: it
          accepts the value, sets nothing, and returns the value as if it
          were known.

  \code{.c}
  switch (lame_get_asm_optimizations(gfp, AVX2)) {
  case  1:  break;      // allowed
  case  0:  break;      // forbidden, or a family with no code behind it
  case -2:  break;      // not a family this interface knows
  default:  return -1;  // the instance is unusable
  }
  \endcode

  \see lame_set_asm_optimizations()
*/
int
lame_get_asm_optimizations(const lame_global_flags * gfp, int optim)
{
    if (is_lame_global_flags_valid(gfp)) {
        switch (optim) {
        case MMX:
        case AMD_3DNOW:
            /* Never enabled, because there is nothing to enable. 0 is the
               truthful answer, and also the one that makes the usual
               "if (lame_get_asm_optimizations(gfp, MMX))" take the branch
               that does not expect those instructions to be used. */
            return 0;
        case SSE:
            return gfp->asm_optimizations.sse;
        case AVX2:
            return gfp->asm_optimizations.avx2;
        default:
            return -2;
        }
    }
    return -1;
}


/*
 * ---- vector routines ------------------------------------------------------
 *
 * These four functions replace the asm_optimizations pair above.  The sets
 * of routines are named in a table, so a new instruction set needs a new
 * table row and no change to lame.h.  See @ref vector_dispatch.
 */

/**
  \brief How many sets of vector routines this build compiled in.

  \return the number, 0 or more. <b>This function cannot fail.</b> It takes
          no arguments and no encoder instance, and reads a table that is
          fixed when the library is built.

  0 is a normal result, not an error. A build for an architecture without
  vector routines, or a build where the compiler could not compile the
  intrinsics, has none. lame_set_vector_routines() still accepts \c "none"
  there.

  You can call it before lame_init(), so a program can report what the library
  contains without creating an encoder.

  \see lame_get_vector_routines_name(), lame_set_vector_routines()
*/
int
lame_get_num_vector_routines(void)
{
    return vector_impl_count();
}


/**
  \brief The name of one set of vector routines.

  \param index in <code>[0, lame_get_num_vector_routines())</code>.
  \return the name, or NULL if \p index is outside that range.

  The name is lowercase and static. It is valid for the whole process and must
  not be freed. It names the real instruction set (\c "sse2", not \c "sse"),
  and lame_set_vector_routines() accepts it. For display, convert it to
  uppercase.

  <b>The indices are in ascending order of capability</b>, so index
  <code>count-1</code> is the widest set of this build. They are stable within
  one process and <b>nowhere else</b>. A build without AVX2 moves every index
  above it, and another architecture has a different list.
  <b>Store the name, never the index.</b>

  lame_set_vector_routines() accepts every name that this function returns in
  this build. It can still report that the processor cannot run it, but never
  that the name is unknown.

  \code
  int i, n = lame_get_num_vector_routines();
  for (i = 0; i < n; ++i)
      puts(lame_get_vector_routines_name(i));
  \endcode

  \see lame_get_num_vector_routines()
*/
const char *
lame_get_vector_routines_name(int index)
{
    return vector_impl_name_at(index);
}


/**
  \brief Choose which vector routines this instance runs.

  \param gfp  the encoder instance.
  \param name one of the names lame_get_vector_routines_name() reports, or
              \c "none" to run the plain C code, or \c "auto" (the default) to
              use the widest set the processor offers.
  \return 0 on success, or:
          - \c -1 if \p gfp is not a usable instance.
          - \c -2 if \p name is not a name this library knows.
          - \c -3 if it names a set that this build does not include.
          - \c -4 if it names a set that this build has, but this processor
            cannot run.

  <b>The name must match exactly, in lowercase.</b> It is an identifier, not
  free text. Convert user input to lowercase and check it before you pass it
  here.

  Naming a set is for performance tests. For example, \c "sse2" on a processor
  that also has AVX2 measures the SSE2 routines. This function checks the name
  at once, not in lame_init_params(). So an impossible request fails at once
  and does not fall back without a message.

  It takes effect in lame_init_params().

  When it names something other than \c "auto", it overrides the deprecated
  lame_set_asm_optimizations(). With \c "auto", the older flags still apply.

  \code
  if (lame_set_vector_routines(gfp, "sse2") != 0) {
      // this build or this processor cannot do it: report it and continue
  }
  \endcode

  \see lame_get_vector_routines(), lame_get_vector_routines_name()
*/
int
lame_set_vector_routines(lame_global_flags * gfp, const char *name)
{
    vector_impl_t impl;

    if (!is_lame_global_flags_valid(gfp))
        return -1;
    if (name == 0)
        return -2;
    /* sizeof a string literal counts its terminator, so these compare the
       whole word and nothing beyond it. */
    if (strncmp(name, "auto", sizeof("auto")) == 0) {
        gfp->vector_routines_request = VECTOR_IMPL_AUTO;
        return 0;
    }
    if (strncmp(name, "none", sizeof("none")) == 0) {
        gfp->vector_routines_request = VECTOR_IMPL_NONE;
        return 0;
    }
    if (!vector_impl_from_name(name, &impl)) {
        /* Distinguish "LAME has never heard of this" from "this build left it
           out".  Only the second is answered by rebuilding, and the caller
           cannot tell them apart from a single failure code. */
        return vector_impl_known(name) ? -3 : -2;
    }
    if (!vector_impl_supported(impl))
        return -4;
    gfp->vector_routines_request = (int) impl;
    return 0;
}


/**
  \brief Which vector routines this instance is actually running.

  \param gfp the encoder instance.
  \return the name, \c "none" if it runs the plain C code, or NULL if \p gfp
          is not usable or lame_init_params() has not run yet.

  It never returns \c "auto", because it returns the result, not the request.
  Before lame_init_params() there is no result yet: the processor was not
  checked, and the request was not applied. So the function returns NULL.

  Use it to find out which set of routines was measured.

  \see lame_set_vector_routines()
*/
const char *
lame_get_vector_routines(const lame_global_flags * gfp)
{
    lame_internal_flags const *gfc;

    if (!is_lame_global_flags_valid(gfp))
        return 0;
    gfc = gfp->internal_flags;
    if (gfc == 0 || !is_lame_internal_flags_valid(gfc))
        return 0;
    return vector_impl_name(vector_implementation(gfc));
}


/*! Choose whether the library writes the ID3 tags itself. */
/*!
  By default LAME writes the ID3v2 tag before the audio and the ID3v1 tag
  after it, as part of the encoded stream. Turning this off leaves both to the
  caller. An application that writes its own tags needs this, or the file gets
  two tags.

  The tag *content* is still set with the \c id3tag_ functions. This setting
  only decides who writes the tags.

  \param gfp  the encoder instance.
  \param v    non-zero to let LAME write the tags, 0 to not write them. The
              default is non-zero.
  \note The function returns nothing, so it ignores an instance that is not
        usable without a message.
*/
void
lame_set_write_id3tag_automatic(lame_global_flags * gfp, int v)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->write_id3tag_automatic = v;
    }
}


/*! Get whether the library writes the ID3 tags itself. */
/*!
  \param gfp the encoder instance.
  \return non-zero if LAME writes the tags, 0 if the caller does. **1 if the
          instance is not usable.** This is the one getter here that returns
          the "on" state, not 0, in that case.
*/
int
lame_get_write_id3tag_automatic(lame_global_flags const *gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->write_id3tag_automatic;
    }
    return 1;
}


/*

UNDOCUMENTED, experimental settings.  These routines are not prototyped
in lame.h.  You should not use them, they are experimental and may
change.  

*/


/*
 *  just another daily changing developer switch  
 */
void
lame_set_tune(lame_global_flags * gfp, float val)
{
    if (is_lame_global_flags_valid(gfp)) {
        gfp->tune_value_a = val;
        gfp->tune = 1;
    }
}

/*! Limit how much the mid/side masking may exceed the left/right masking. */
/*!
  In a joint stereo encode, the mid and side channels have their own masking
  thresholds. These can allow much more noise than the thresholds for left and
  right. This setting limits that. Where the mid/side threshold is higher than
  the left/right threshold by more than this factor, LAME lowers it. A larger
  value allows more. 0 turns the limit off.

  A positive value also turns on the ATH adjustment in the same calculation,
  so it is not only a limit. That is why the presets set it as part of a
  tuning, not alone.

  \param gfp    the encoder instance.
  \param msfix  the factor. 0 or less turns the limit off. The default is "not
                set", and \c lame_init_params() then sets 0.
  \note The function returns nothing. So it ignores an instance that is not
        usable, and a factor that is NaN or infinite, without a message. The
        caller cannot tell that the setting was not stored.
*/
void
lame_set_msfix(lame_global_flags * gfp, double msfix)
{
    if (is_lame_global_flags_valid(gfp) && double_is_finite(msfix)) {
        /* default = 0 */
        gfp->msfix = msfix;
    }
}

/*! Get the mid/side masking cap. */
/*!
  The setter takes a \c double, but the value is stored and returned as a
  \c float. So the value read back is not always the value that was set.

  \param gfp the encoder instance.
  \return the factor, or -1 if nothing is set and \c lame_init_params() has
          not set it to 0. 0 if the instance is not usable.
*/
float
lame_get_msfix(const lame_global_flags * gfp)
{
    if (is_lame_global_flags_valid(gfp)) {
        return gfp->msfix;
    }
    return 0;
}


/*! Select the experimental options of a preset. */
/*!
  \deprecated This function does nothing. The preset variants it selected do
  not exist. lame.h does not declare this function. The library still exports
  it, so that programs built against an older release still link.

  \param gfp             ignored.
  \param preset_expopts  ignored.
  \return always 0.
*/
int
lame_set_preset_expopts(lame_global_flags * gfp, int preset_expopts)
{
    (void) gfp;
    (void) preset_expopts;
    return 0;
}


int
lame_set_preset_notune(lame_global_flags * gfp, int preset_notune)
{
    (void) gfp;
    (void) preset_notune;
    return 0;
}

static int
calc_maximum_input_samples_for_buffer_size(lame_internal_flags const* gfc, size_t buffer_size)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    int const pcm_samples_per_frame = SAMPLES_PER_GRANULE * cfg->mode_gr;
    int     frames_per_buffer = 0, input_samples_per_buffer = 0;
    int     kbps = 320;

    if (cfg->samplerate_out < 16000)
        kbps = 64;
    else if (cfg->samplerate_out < 32000)
        kbps = 160;
    else
        kbps = 320;
    if (cfg->free_format)
        kbps = cfg->avg_bitrate;
    else if (cfg->vbr == vbr_off) {
        kbps = cfg->avg_bitrate;
    }
    {
        int const pad = 1;
        int const bpf = ((cfg->version + 1) * 72000 * kbps / cfg->samplerate_out + pad);
        /* What the stream already holds goes out first: after
           lame_init_params() that is the ID3v2 tag and the Xing/LAME frame. */
        size_t const pending = gfc->bs.buf_byte_idx >= 0 ? (size_t) gfc->bs.buf_byte_idx + 1 : 0;
        size_t const room = buffer_size > pending ? buffer_size - pending : 0;
        /* A frame's header goes into the stream where the preceding frame's
           main data reaches it, which the bit reservoir can make earlier than
           that data's end, so a call can return bytes beyond its own frames.
           One frame of the buffer is held back for them. */
        size_t const whole = room / (size_t) bpf;
        /* The resampler keeps up to its filter's length of input back for the
           next call, which then returns that input's output as well - the
           filter length times the rate ratio, in samples. */
        size_t const held = cfg->samplerate_in == cfg->samplerate_out ? 0
            : (size_t) ((RESAMPLE_HELD_INPUT * (double) cfg->samplerate_out / cfg->samplerate_in
                         + pcm_samples_per_frame - 1) / pcm_samples_per_frame);
        size_t const reserve = 1 + held;
        size_t const frames = whole > reserve ? whole - reserve : 0;
        /* an encode call takes an int sample count, so a buffer holding more
           frames than that can express is reported at the representable
           ceiling instead of wrapping into a negative estimate */
        frames_per_buffer = frames > (size_t) INT_MAX ? INT_MAX : (int) frames;
    }
    {
        double const ratio = (double) cfg->samplerate_in / cfg->samplerate_out;
        double const samples = (double) pcm_samples_per_frame * frames_per_buffer * ratio;
        input_samples_per_buffer = samples >= (double) INT_MAX ? INT_MAX : (int) samples;
    }
    return input_samples_per_buffer;
}

/*! Ask how many samples fit in a given output buffer. */
/*!
  This function returns how many samples per channel you can pass to one
  encode call, so that the output fits into a buffer of \a buffer_size bytes.
  Use it when the buffer size is fixed.

  The result assumes the worst case for the current settings: the highest
  bitrate for the sample rate, or the set bitrate where it is fixed. It
  includes resampling. It also includes the data that the next call writes in
  addition to its own frames. This is the data that the stream already holds,
  such as the ID3v2 tag before the first call, and the data that the bit
  reservoir and the resampler keep from one call to the next. So call it again
  before each encode call. If the buffer could hold more samples than one
  encode call accepts, the result is that maximum.

  \param gfp          the encoder instance, after \c lame_init_params().
  \param buffer_size  the output buffer size in bytes.
  \return the number of samples per channel that may be passed.
          \c LAME_GENERICERROR if the instance is not initialized.
*/
int
lame_get_maximum_number_of_samples(lame_t gfp, size_t buffer_size)
{
    if (is_lame_global_flags_valid(gfp)) {
        lame_internal_flags const *const gfc = gfp->internal_flags;
        if (is_lame_internal_flags_valid(gfc)) {
            return calc_maximum_input_samples_for_buffer_size(gfc, buffer_size);
        }
    }
    return LAME_GENERICERROR;
}

/*! @} */
