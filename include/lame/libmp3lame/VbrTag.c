/*
 *      Xing VBR tagging for LAME.
 *
 *      Copyright (c) 1999 A.L. Faber
 *      Copyright (c) 2001 Jonathan Dee
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
# include <config.h>
#endif

#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "bitstream.h"
#include "VbrTag.h"
#include "lame_global_flags.h"
#include "tables.h"

#ifdef __sun__
/* woraround for SunOS 4.x, it has SEEK_* defined here */
#include <unistd.h>
#endif


/*
 *    4 bytes for Header Tag
 *    4 bytes for Header Flags
 *    4 bytes for FRAME SIZE
 *    4 bytes for STREAM_SIZE
 *  100 bytes for entry (NUMTOCENTRIES)
 *    4 bytes for VBR SCALE. a VBR quality indicator: 0=best 100=worst
 * ___________
 *  120 bytes
*/
#define VBRHEADERSIZE (NUMTOCENTRIES+4+4+4+4+4)

/*  the Xing/Info header above, followed by the 36-byte LAME tag, which opens
 *  with the encoder version string
 * ___________
 *  156 bytes
*/
#define LAMEHEADERSIZE (VBRHEADERSIZE + 9 + 1 + 1 + 8 + 1 + 1 + 3 + 1 + 1 + 2 + 4 + 2 + 2)

/* the size of the Xing header (MPEG1 and MPEG2) in kbps */
#define XING_BITRATE1 128
#define XING_BITRATE2  64
#define XING_BITRATE25 32

static const char VBRTag0[] = { "Xing" };
static const char VBRTag1[] = { "Info" };




/* Lookup table for fast CRC computation
 * See 'CRC_update_lookup'
 * Uses the polynomial x^16+x^15+x^2+1 */

#include "VbrTagCrc.h"





/***********************************************************************
 *  Robert Hegemann 2001-01-17
 ***********************************************************************/

static void
addVbr(VBR_seek_info_t * v, int bitrate)
{
    int     i;

    v->nVbrNumFrames++;
    v->sum += bitrate;
    v->seen++;

    if (v->seen < v->want) {
        return;
    }

    if (v->pos < v->size) {
        v->bag[v->pos] = v->sum;
        v->pos++;
        v->seen = 0;
    }
    if (v->pos == v->size) {
        for (i = 1; i < v->size; i += 2) {
            v->bag[i / 2] = v->bag[i];
        }
        v->want *= 2;
        v->pos /= 2;
    }
}

static void
Xing_seek_table(VBR_seek_info_t const* v, unsigned char *t)
{
    int     i, indx;
    int     seek_point;

    if (v->pos <= 0)
        return;

    for (i = 1; i < NUMTOCENTRIES; ++i) {
        float   j = i / (float) NUMTOCENTRIES, act, sum;
        indx = (int) (floor(j * v->pos));
        if (indx > v->pos - 1)
            indx = v->pos - 1;
        act = v->bag[indx];
        sum = v->sum;
        seek_point = (int) (256. * act / sum);
        if (seek_point > 255)
            seek_point = 255;
        t[i] = seek_point;
    }
}


/****************************************************************************
 * AddVbrFrame: Add VBR entry, used to fill the VBR the TOC entries
 * Paramters:
 *      nStreamPos: how many bytes did we write to the bitstream so far
 *                              (in Bytes NOT Bits)
 ****************************************************************************
*/
void
AddVbrFrame(lame_internal_flags * gfc)
{
    int     kbps = bitrate_table[gfc->cfg.version][gfc->ov_enc.bitrate_index];
    assert(gfc->VBR_seek_table.bag);
    addVbr(&gfc->VBR_seek_table, kbps);
}


/**
 * \internal
 * \brief Returns the bitrate of the frame that carries the Xing and LAME tags.
 *
 * \param cfg  the session configuration.
 * \return the bitrate in kbps: the stream's own in CBR, else a fixed one per
 *         MPEG version.
 */
static int
xing_frame_kbps(SessionConfig_t const *cfg)
{
    int     kbps;
    if (1 == cfg->version) {
        kbps = XING_BITRATE1;
    }
    else {
        if (cfg->samplerate_out < 16000)
            kbps = XING_BITRATE25;
        else
            kbps = XING_BITRATE2;
    }

    if (cfg->vbr == vbr_off)
        kbps = cfg->avg_bitrate;
    return kbps;
}

static void
setLameTagFrameHeader(lame_internal_flags const *gfc, unsigned char *buffer)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncResult_t const *const eov = &gfc->ov_enc;
    uint32_t const header = mpeg_header_word(cfg, eov->bitrate_index, 0, eov->mode_ext);
    char    abyte, bbyte;

    buffer[0] = (unsigned char) (header >> 24);
    buffer[1] = (unsigned char) (header >> 16);
    buffer[2] = (unsigned char) (header >> 8);
    buffer[3] = (unsigned char) header;

    /* the default VBR header. 48 kbps layer III, no padding, no crc */
    /* but sampling freq, mode andy copyright/copy protection taken */
    /* from first valid frame */
    buffer[0] = (uint8_t) 0xff;
    abyte = (buffer[1] & (unsigned char) 0xf1);
    {
        int const bitrate = xing_frame_kbps(cfg);

        if (cfg->free_format)
            bbyte = 0x00;
        else
            bbyte = 16 * BitrateIndex(bitrate, cfg->version, cfg->samplerate_out);
    }

    /* Use as much of the info from the real frames in the
     * Xing header:  samplerate, channels, crc, etc...
     */
    if (cfg->version == 1) {
        /* MPEG1 */
        buffer[1] = abyte | (char) 0x0a; /* was 0x0b; */
        abyte = buffer[2] & (char) 0x0d; /* AF keep also private bit */
        buffer[2] = (char) bbyte | abyte; /* 64kbs MPEG1 frame */
    }
    else {
        /* MPEG2 */
        buffer[1] = abyte | (char) 0x02; /* was 0x03; */
        abyte = buffer[2] & (char) 0x0d; /* AF keep also private bit */
        buffer[2] = (char) bbyte | abyte; /* 64kbs MPEG2 frame */
    }
}

/****************************************************************************
 * InitVbrTag: Initializes the header, and write empty frame to stream
 * Paramters:
 *                              fpStream: pointer to output file stream
 *                              nMode   : Channel Mode: 0=STEREO 1=JS 2=DS 3=MONO
 ****************************************************************************
*/
int
InitVbrTag(lame_global_flags * gfp)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    SessionConfig_t const *const cfg = &gfc->cfg;
    int     kbps_header;

#define MAXFRAMESIZE 2880 /* or 0xB40, the max freeformat 640 32kHz framesize */

    /*
     * Xing VBR pretends to be a 48kbs layer III frame.  (at 44.1kHz).
     * (at 48kHz they use 56kbs since 48kbs frame not big enough for
     * table of contents)
     * let's always embed Xing header inside a 64kbs layer III frame.
     * this gives us enough room for a LAME version string too.
     * size determined by sampling frequency (MPEG1)
     * 32kHz:    216 bytes@48kbs    288bytes@ 64kbs
     * 44.1kHz:  156 bytes          208bytes@64kbs     (+1 if padding = 1)
     * 48kHz:    144 bytes          192
     *
     * MPEG 2 values are the same since the framesize and samplerate
     * are each reduced by a factor of 2.
     */


    kbps_header = xing_frame_kbps(cfg);

    /* make sure LAME Header fits into Frame
     */
    {
        int     total_frame_size = ((cfg->version + 1) * 72000 * kbps_header) / cfg->samplerate_out;
        int     header_size = (cfg->sideinfo_len + LAMEHEADERSIZE);
        gfc->VBR_seek_table.TotalFrameSize = total_frame_size;
        if (total_frame_size < header_size || total_frame_size > MAXFRAMESIZE) {
            /* disable tag, it wont fit */
            gfc->cfg.write_lame_tag = 0;
            return 0;
        }
    }

    gfc->VBR_seek_table.nVbrNumFrames = 0;
    gfc->VBR_seek_table.nBytesWritten = 0;
    gfc->VBR_seek_table.sum = 0;

    gfc->VBR_seek_table.seen = 0;
    gfc->VBR_seek_table.want = 1;
    gfc->VBR_seek_table.pos = 0;

    if (gfc->VBR_seek_table.bag == NULL) {
        gfc->VBR_seek_table.bag = lame_calloc(int, 400);
        if (gfc->VBR_seek_table.bag != NULL) {
            gfc->VBR_seek_table.size = 400;
        }
        else {
            gfc->VBR_seek_table.size = 0;
            ERRORF(gfc, "Error: can't allocate VbrFrames buffer\n");
            gfc->cfg.write_lame_tag = 0;
            return -1;
        }
    }

    /* write dummy VBR tag of all 0's into bitstream */
    {
        uint8_t buffer[MAXFRAMESIZE];
        size_t  i, n;

        memset(buffer, 0, sizeof(buffer));
        setLameTagFrameHeader(gfc, buffer);
        n = gfc->VBR_seek_table.TotalFrameSize;
        for (i = 0; i < n; ++i) {
            add_dummy_byte(gfc, buffer[i], 1);
        }
    }
    /* Success */
    return 0;
}



/* fast CRC-16 computation - uses table crc16_lookup 8*/
static uint16_t
CRC_update_lookup(uint16_t crc, unsigned char const *buffer, int size)
{
    while (size >= 8) {
        crc ^= buffer[1] << 8 | buffer[0];
 
        crc = crc16_lookup[7][crc & 0xFF] ^ crc16_lookup[6][crc >> 8 ] ^
              crc16_lookup[5][buffer[2] ] ^ crc16_lookup[4][buffer[3]] ^
              crc16_lookup[3][buffer[4] ] ^ crc16_lookup[2][buffer[5]] ^
              crc16_lookup[1][buffer[6] ] ^ crc16_lookup[0][buffer[7]];

        buffer += 8;
        size -= 8;
    }

    while (size--) crc = (crc >> 8) ^ crc16_lookup[0][(crc & 0xff) ^ *buffer++];
    return crc;
}

void
UpdateMusicCRC(uint16_t * crc, unsigned char const *buffer, int size)
{
    *crc = CRC_update_lookup(*crc, buffer, size);
}





/****************************************************************************
 * Jonathan Dee 2001/08/31
 *
 * PutLameVBR: Write LAME info: mini version + info on various switches used
 * Paramters:
 *                              pbtStreamBuffer : pointer to output buffer
 *                              id3v2size               : size of id3v2 tag in bytes
 *                              crc                             : computation of crc-16 of Lame Tag so far (starting at frame sync)
 *
 ****************************************************************************
*/
static int
PutLameVBR(lame_global_flags const *gfp, size_t nMusicLength, uint8_t * pbtStreamBuffer, uint16_t crc)
{
    lame_internal_flags const *gfc = gfp->internal_flags;
    SessionConfig_t const *const cfg = &gfc->cfg;

    int     nBytesWritten = 0;

    int     enc_delay = gfc->ov_enc.encoder_delay; /* encoder delay */
    int     enc_padding = gfc->ov_enc.encoder_padding; /* encoder padding  */

    /*recall: cfg->vbr_q is for example set by the switch -V  */
    /*   gfp->quality by -q, -h, -f, etc */

    int     nQuality = (100 - 10 * gfp->VBR_q - gfp->quality);


    /*
    NOTE:
            Even though the specification for the LAME VBR tag
            did explicitly mention other encoders than LAME,
            many SW/HW decoder seem to be able to make use of
            this tag only, if the encoder version starts with LAME.
            To be compatible with such decoders, ANY encoder will
            be forced to write a fake LAME version string!
            As a result, the encoder version info becomes worthless.
    */
    const char *szVersion = get_lame_tag_encoder_short_version();
    uint8_t nVBR;
    uint8_t nRevision = 0x00;
    uint8_t nRevMethod;
    static const uint8_t vbr_type_translator[] = { 1, 5, 3, 2, 4, 0, 3 }; /*numbering different in vbr_mode vs. Lame tag */

    uint8_t nLowpass =
        (((cfg->lowpassfreq / 100.0) + .5) > 255 ? 255 : (cfg->lowpassfreq / 100.0) + .5);

    uint32_t nPeakSignalAmplitude = 0;

    uint16_t nRadioReplayGain = 0;
    uint16_t nAudiophileReplayGain = 0;

    uint8_t nNoiseShaping = cfg->noise_shaping;
    uint8_t nStereoMode = 0;
    int     bNonOptimal = 0;
    uint8_t nSourceFreq = 0;
    uint8_t nMisc = 0;
    uint16_t nMusicCRC = 0;

    /*psy model type: Gpsycho or NsPsytune */
    unsigned char bExpNPsyTune = 1; /* only NsPsytune */
    unsigned char bSafeJoint = (cfg->use_safe_joint_stereo) != 0;

    unsigned char bNoGapMore = 0;
    unsigned char bNoGapPrevious = 0;

    int     nNoGapCount = gfp->nogap_total;
    int     nNoGapCurr = gfp->nogap_current;


    uint8_t nAthType = cfg->ATHtype; /*4 bits. */

    uint8_t nFlags = 0;

    /* if ABR, {store bitrate <=255} else { store "-b"} */
    int     nABRBitrate;
    switch (cfg->vbr) {
    case vbr_abr:{
            nABRBitrate = cfg->vbr_avg_bitrate_kbps;
            break;
        }
    case vbr_off:{
            nABRBitrate = cfg->avg_bitrate;
            break;
        }
    default:{          /*vbr modes */
            nABRBitrate = bitrate_table[cfg->version][cfg->vbr_min_bitrate_index];
        }
    }


    /*revision and vbr method */
    if (cfg->vbr < dimension_of(vbr_type_translator))
        nVBR = vbr_type_translator[cfg->vbr];
    else
        nVBR = 0x00;    /*unknown. */

    nRevMethod = 0x10 * nRevision + nVBR;


    /* ReplayGain */
    if (cfg->findReplayGain) {
        int     RadioGain = gfc->ov_rpg.RadioGain;
        if (RadioGain > 0x1FE)
            RadioGain = 0x1FE;
        if (RadioGain < -0x1FE)
            RadioGain = -0x1FE;

        nRadioReplayGain = 0x2000; /* set name code */
        nRadioReplayGain |= 0xC00; /* set originator code to `determined automatically' */

        if (RadioGain >= 0)
            nRadioReplayGain |= RadioGain; /* set gain adjustment */
        else {
            nRadioReplayGain |= 0x200; /* set the sign bit */
            nRadioReplayGain |= -RadioGain; /* set gain adjustment */
        }
    }

    /* peak sample */
    if (cfg->findPeakSample) {
        /* the peak scaled to 2^23, or the field's largest value if it does not fit */
        double const peak = (((FLOAT) gfc->ov_rpg.PeakSample) / 32767.0) * pow(2, 23) + .5;
        nPeakSignalAmplitude = peak < 4294967296.0 ? (uint32_t) peak : 0xFFFFFFFFu;
    }

    /*nogap */
    if (nNoGapCount != -1) {
        if (nNoGapCurr > 0)
            bNoGapPrevious = 1;

        if (nNoGapCurr < nNoGapCount - 1)
            bNoGapMore = 1;
    }
    /* A file continuing a nogap set carries no encoder lead-in; that was
       written at the front of the set's first file. */
    if (bNoGapPrevious)
        enc_delay = 0;

    /*flags */

    nFlags = nAthType + (bExpNPsyTune << 4)
        + (bSafeJoint << 5)
        + (bNoGapMore << 6)
        + (bNoGapPrevious << 7);


    if (nQuality < 0)
        nQuality = 0;

    /*stereo mode field... a bit ugly. */

    switch (cfg->mode) {
    case MONO:
        nStereoMode = 0;
        break;
    case STEREO:
        nStereoMode = 1;
        break;
    case DUAL_CHANNEL:
        nStereoMode = 2;
        break;
    case JOINT_STEREO:
        if (cfg->force_ms)
            nStereoMode = 4;
        else
            nStereoMode = 3;
        break;
    case NOT_SET:
        /* FALLTHROUGH */
    default:
        nStereoMode = 7;
        break;
    }

    /*Intensity stereo : nStereoMode = 6. IS is not implemented */

    if (cfg->samplerate_in <= 32000)
        nSourceFreq = 0x00;
    else if (cfg->samplerate_in == 48000)
        nSourceFreq = 0x02;
    else if (cfg->samplerate_in > 48000)
        nSourceFreq = 0x03;
    else
        nSourceFreq = 0x01; /*default is 44100Hz. */


    /*Check if the user overrided the default LAME behaviour with some nasty options */

    if (cfg->short_blocks == short_block_forced || cfg->short_blocks == short_block_dispensed || ((cfg->lowpassfreq == -1) && (cfg->highpassfreq == -1)) || /* "-k" */
        (cfg->disable_reservoir && cfg->avg_bitrate < 320) ||
        cfg->noATH || cfg->ATHonly || (nAthType == 0) || cfg->samplerate_in <= 32000)
        bNonOptimal = 1;

    nMisc = nNoiseShaping + (nStereoMode << 2)
        + (bNonOptimal << 5)
        + (nSourceFreq << 6);


    nMusicCRC = gfc->nMusicCRC;


    /*Write all this information into the stream */
    put_be32(&pbtStreamBuffer[nBytesWritten], nQuality);
    nBytesWritten += 4;

    /*  LAME tag: the encoder version field has a fixed width of 9 bytes.
     *  szVersion (get_lame_tag_encoder_short_version()) must fit into 9
     *  bytes. The comment of that function in version.c gives the byte
     *  budget, the reason the width cannot change, and the compile-time
     *  check. strncpy cuts the string without a message, and without the NUL
     *  terminator, if it does not fit. */
    strncpy((char *) &pbtStreamBuffer[nBytesWritten], szVersion, 9);
    nBytesWritten += 9;

    pbtStreamBuffer[nBytesWritten] = nRevMethod;
    nBytesWritten++;

    pbtStreamBuffer[nBytesWritten] = nLowpass;
    nBytesWritten++;

    put_be32(&pbtStreamBuffer[nBytesWritten], nPeakSignalAmplitude);
    nBytesWritten += 4;

    put_be16(&pbtStreamBuffer[nBytesWritten], nRadioReplayGain);
    nBytesWritten += 2;

    put_be16(&pbtStreamBuffer[nBytesWritten], nAudiophileReplayGain);
    nBytesWritten += 2;

    pbtStreamBuffer[nBytesWritten] = nFlags;
    nBytesWritten++;

    if (nABRBitrate >= 255)
        pbtStreamBuffer[nBytesWritten] = 0xFF;
    else
        pbtStreamBuffer[nBytesWritten] = nABRBitrate;
    nBytesWritten++;

    pbtStreamBuffer[nBytesWritten] = enc_delay >> 4; /* works for win32, does it for unix? */
    pbtStreamBuffer[nBytesWritten + 1] = (enc_delay << 4) + (enc_padding >> 8);
    pbtStreamBuffer[nBytesWritten + 2] = enc_padding;

    nBytesWritten += 3;

    pbtStreamBuffer[nBytesWritten] = nMisc;
    nBytesWritten++;


    pbtStreamBuffer[nBytesWritten++] = 0; /*unused in rev0 */

    put_be16(&pbtStreamBuffer[nBytesWritten], (uint16_t) cfg->preset);
    nBytesWritten += 2;

    put_be32(&pbtStreamBuffer[nBytesWritten], (uint32_t) nMusicLength);
    nBytesWritten += 4;

    put_be16(&pbtStreamBuffer[nBytesWritten], nMusicCRC);
    nBytesWritten += 2;

    /*Calculate tag CRC.... must be done here, since it includes
     *previous information*/

    crc = CRC_update_lookup(crc, pbtStreamBuffer, nBytesWritten);

    put_be16(&pbtStreamBuffer[nBytesWritten], crc);
    nBytesWritten += 2;

    return nBytesWritten;
}

static long
skipId3v2(FILE * fpStream)
{
    size_t  nbytes;
    long    id3v2TagSize;
    unsigned char id3v2Header[10];

    /* seek to the beginning of the stream */
    if (fseek(fpStream, 0, SEEK_SET) != 0) {
        return -2;      /* not seekable, abort */
    }
    /* read 10 bytes in case there's an ID3 version 2 header here */
    nbytes = fread(id3v2Header, 1, sizeof(id3v2Header), fpStream);
    if (nbytes != sizeof(id3v2Header)) {
        return -3;      /* not readable, maybe opened Write-Only */
    }
    /* does the stream begin with the ID3 version 2 file identifier? */
    if (!strncmp((char *) id3v2Header, "ID3", 3)) {
        /* the tag size (minus the 10-byte header) is encoded into four
         * bytes where the most significant bit is clear in each byte */
        id3v2TagSize = (((id3v2Header[6] & 0x7f) << 21)
                        | ((id3v2Header[7] & 0x7f) << 14)
                        | ((id3v2Header[8] & 0x7f) << 7)
                        | (id3v2Header[9] & 0x7f))
            + sizeof id3v2Header;
    }
    else {
        /* no ID3 version 2 tag in this stream */
        id3v2TagSize = 0;
    }
    return id3v2TagSize;
}



/*! Return the finished LAME tag frame to the caller instead of writing it. */
/*!
  \ingroup api_tags
  Builds the same frame that \c lame_mp3_tags_fid() would write: the Xing/Info
  header, the seek table and LAME's own fields. It copies the frame into
  \a buffer, and the caller puts it in the right place. Use this function when
  the output is not a file that this library can seek in, or when the caller
  builds the stream itself.

  Like \c lame_mp3_tags_fid(), call it last, after \c lame_encode_flush(),
  because the frame describes what the encode produced.

  The place of the frame is fixed. LAME reserved an empty frame for the tag at
  the start of the audio data, and this frame replaces it. Unless the caller
  wrote its own data before the audio, that is the start of the file, or the
  first thing after an ID3v2 tag. A caller that writes its own data first must
  remember where the reserved frame is.

  \param gfp     the encoder instance.
  \param buffer  receives the frame.
  \param size    the size of \a buffer in bytes.
  \return the number of bytes written to \a buffer. 0 if this encode has no
          LAME tag. If \a buffer is too small, the return value is the size
          that the frame needs, which is larger than \a size. So a result
          larger than \a size means that nothing was written. Call the
          function again with a buffer of that size.
*/
size_t
lame_get_lametag_frame(lame_global_flags const *gfp, unsigned char *buffer, size_t size)
{
    lame_internal_flags *gfc;
    SessionConfig_t const *cfg;
    uint32_t stream_size;
    unsigned int  nStreamIndex;
    uint8_t btToc[NUMTOCENTRIES];

    if (gfp == 0) {
        return 0;
    }
    gfc = gfp->internal_flags;
    if (gfc == 0) {
        return 0;
    }
    if (!is_lame_internal_flags_valid(gfc)) {
        return 0;
    }
    cfg = &gfc->cfg;
    if (cfg->write_lame_tag == 0) {
        return 0;
    }
    if (gfc->VBR_seek_table.pos <= 0) {
        return 0;
    }
    if (size < gfc->VBR_seek_table.TotalFrameSize) {
        return gfc->VBR_seek_table.TotalFrameSize;
    }
    if (buffer == 0) {
        return 0;
    }

    memset(buffer, 0, gfc->VBR_seek_table.TotalFrameSize);

    /* 4 bytes frame header */

    setLameTagFrameHeader(gfc, buffer);

    /* Clear all TOC entries */
    memset(btToc, 0, sizeof(btToc));

    if (cfg->free_format) {
        int     i;
        for (i = 1; i < NUMTOCENTRIES; ++i)
            btToc[i] = 255 * i / 100;
    }
    else {
        Xing_seek_table(&gfc->VBR_seek_table, btToc);
    }

    /* Start writing the tag after the zero frame */
    nStreamIndex = cfg->sideinfo_len;
    /* note! Xing header specifies that Xing data goes in the
     * ancillary data with NO ERROR PROTECTION.  If error protecton
     * in enabled, the Xing data still starts at the same offset,
     * and now it is in sideinfo data block, and thus will not
     * decode correctly by non-Xing tag aware players */
    if (cfg->error_protection)
        nStreamIndex -= 2;

    /* Put Vbr tag */
    if (cfg->vbr == vbr_off) {
        buffer[nStreamIndex++] = VBRTag1[0];
        buffer[nStreamIndex++] = VBRTag1[1];
        buffer[nStreamIndex++] = VBRTag1[2];
        buffer[nStreamIndex++] = VBRTag1[3];

    }
    else {
        buffer[nStreamIndex++] = VBRTag0[0];
        buffer[nStreamIndex++] = VBRTag0[1];
        buffer[nStreamIndex++] = VBRTag0[2];
        buffer[nStreamIndex++] = VBRTag0[3];
    }

    /* Put header flags */
    put_be32(&buffer[nStreamIndex], FRAMES_FLAG + BYTES_FLAG + TOC_FLAG + VBR_SCALE_FLAG);
    nStreamIndex += 4;

    /* Put Total Number of frames */
    put_be32(&buffer[nStreamIndex], gfc->VBR_seek_table.nVbrNumFrames);
    nStreamIndex += 4;

    /* Put total audio stream size, including Xing/LAME Header. The field has
       32 bits: a stream of 4 GiB or more gets its largest value. */
    {
        uint64_t const bytes = gfc->VBR_seek_table.nBytesWritten + gfc->VBR_seek_table.TotalFrameSize;
        stream_size = bytes < 0xFFFFFFFFu ? (uint32_t) bytes : 0xFFFFFFFFu;
    }
    put_be32(&buffer[nStreamIndex], stream_size);
    nStreamIndex += 4;

    /* Put TOC */
    memcpy(&buffer[nStreamIndex], btToc, sizeof(btToc));
    nStreamIndex += sizeof(btToc);


    if (cfg->error_protection) {
        /* (jo) error_protection: add crc16 information to header */
        CRC_writeheader(gfc, buffer);
    }
    {
        /*work out CRC so far: initially crc = 0 */
        uint16_t crc = 0x00;
        crc = CRC_update_lookup(crc, buffer, nStreamIndex);
        /*Put LAME VBR info */
        nStreamIndex += PutLameVBR(gfp, stream_size, buffer + nStreamIndex, crc);
    }
    /* The bytes written are the bytes that InitVbrTag() reserved in the frame. */
    assert((int) nStreamIndex + (cfg->error_protection ? 2 : 0) == cfg->sideinfo_len + LAMEHEADERSIZE);

    return gfc->VBR_seek_table.TotalFrameSize;
}

/***********************************************************************
 *
 * PutVbrTag: Write final VBR tag to the file
 * Paramters:
 *                              lpszFileName: filename of MP3 bit stream
 *                              nVbrScale       : encoder quality indicator (0..100)
 ****************************************************************************
 */

int
PutVbrTag(lame_global_flags const *gfp, FILE * fpStream)
{
    lame_internal_flags *gfc = gfp->internal_flags;

    long    lFileSize;
    long    id3v2TagSize;
    size_t  nbytes;
    uint8_t buffer[MAXFRAMESIZE];

    if (gfc->VBR_seek_table.pos <= 0)
        return -1;

    /* Seek to end of file */
    fseek(fpStream, 0, SEEK_END);

    /* Get file size */
    lFileSize = ftell(fpStream);

    /* Abort if file has zero length. Yes, it can happen :) */
    if (lFileSize == 0)
        return -1;

    /*
     * The VBR tag may NOT be located at the beginning of the stream.
     * If an ID3 version 2 tag was added, then it must be skipped to write
     * the VBR tag data.
     */

    id3v2TagSize = skipId3v2(fpStream);

    if (id3v2TagSize < 0) {
        return id3v2TagSize;
    }

    /*Seek to the beginning of the stream */
    fseek(fpStream, id3v2TagSize, SEEK_SET);

    nbytes = lame_get_lametag_frame(gfp, buffer, sizeof(buffer));
    if (nbytes > sizeof(buffer)) {
        return -1;
    }

    if (nbytes < 1) {
        return 0;
    }

    /* Put it all to disk again */
    if (fwrite(buffer, nbytes, 1, fpStream) != 1) {
        return -1;
    }

    return 0;           /* success */
}
