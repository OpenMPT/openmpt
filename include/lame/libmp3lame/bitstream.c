/*
 *      MP3 bitstream Output interface for LAME
 *
 *      Copyright (c) 1999-2000 Mark Taylor
 *      Copyright (c) 1999-2002 Takehiro Tominaga
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
 *
 * $Id$
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "tables.h"
#include "quantize_pvt.h"
#include "lame_global_flags.h"
#include "gain_analysis.h"
#include "VbrTag.h"
#include "bitstream.h"
#include "tables.h"



/* unsigned int is at least this large:  */
/* we work with ints, so when doing bit manipulation, we limit
 * ourselves to MAX_LENGTH-2 just to be on the safe side */
#define MAX_LENGTH      32






static int
calcFrameLength(SessionConfig_t const *const cfg, int kbps, int pad)
{
  return 8 * ((cfg->version + 1) * 72000 * kbps / cfg->samplerate_out + pad);
}


/***********************************************************************
 * compute bitsperframe and mean_bits for a layer III frame
 **********************************************************************/
int
getframebits(const lame_internal_flags * gfc)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncResult_t const *const eov = &gfc->ov_enc;
    int     bit_rate;

    /* get bitrate in kbps [?] */
    if (eov->bitrate_index)
        bit_rate = bitrate_table[cfg->version][eov->bitrate_index];
    else
        bit_rate = cfg->avg_bitrate;
    /*assert(bit_rate <= 550); */
    assert(8 <= bit_rate && bit_rate <= MAX_FREE_FORMAT_KBPS);

    /* main encoding routine toggles padding on and off */
    /* one Layer3 Slot consists of 8 bits */
    return calcFrameLength(cfg, bit_rate, eov->padding);
}

int
get_max_frame_buffer_size_by_constraint(SessionConfig_t const * cfg, int constraint)
{
    int     maxmp3buf = 0;
    if (cfg->free_format && cfg->avg_bitrate > 320) {
        /* in freeformat the buffer is constant */
        if (constraint == MDB_STRICT_ISO) {
            maxmp3buf = calcFrameLength(cfg, cfg->avg_bitrate, 0);
        }
        else {
            /* maximum allowed bits per granule are 7680 */
            maxmp3buf = 7680 * (cfg->version + 1);
        }
    }
    else {
        int     max_kbps;
        if (cfg->samplerate_out < 16000) {
            max_kbps = bitrate_table[cfg->version][8]; /* default: allow 64 kbps (MPEG-2.5) */
        }
        else {
            max_kbps = bitrate_table[cfg->version][14];
        }
        switch (constraint) 
        {
        default:
        case MDB_DEFAULT:
            /* Bouvigne suggests this more lax interpretation of the ISO doc instead of using 8*960. */
            /* All mp3 decoders should have enough buffer to handle this value: size of a 320kbps 32kHz frame */
            maxmp3buf = 8 * 1440;
            break;
        case MDB_STRICT_ISO:
            maxmp3buf = calcFrameLength(cfg, max_kbps, 0);
            break;
        case MDB_MAXIMUM:
            maxmp3buf = 7680 * (cfg->version + 1);
            break;
        }
    }
    return maxmp3buf;
}


static void
putheader_bits(lame_internal_flags * gfc)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncStateVar_t *const esv = &gfc->sv_enc;
    Bit_stream_struc *bs = &gfc->bs;
    memcpy(&bs->buf[bs->buf_byte_idx], esv->header[esv->w_ptr].buf, cfg->sideinfo_len);
    bs->buf_byte_idx += cfg->sideinfo_len;
    bs->totbit += cfg->sideinfo_len * 8;
    esv->w_ptr = (esv->w_ptr + 1) & (MAX_HEADER_BUF - 1);
}




/**
 * \internal
 * \brief Writes the low \a j bits of \a val into the bit stream.
 * \param gfc           the encoder.
 * \param val           the bits, in its lowest \a j bits.
 * \param j             the number of bits.
 * \param check_header  non-zero to put the next frame header and side
 *                      information into the stream when the stream reaches
 *                      the position that is due for them.
 */
inline static void
putbits_check(lame_internal_flags * gfc, int val, int j, int check_header)
{
    EncStateVar_t const *const esv = &gfc->sv_enc;
    Bit_stream_struc *bs;
    bs = &gfc->bs;

    assert(j < MAX_LENGTH - 2);

    while (j > 0) {
        int     k;
        if (bs->buf_bit_idx == 0) {
            bs->buf_bit_idx = 8;
            bs->buf_byte_idx++;
            assert(bs->buf_byte_idx < BUFFER_SIZE);
            if (check_header) {
                assert(esv->header[esv->w_ptr].write_timing >= bs->totbit);
                if (esv->header[esv->w_ptr].write_timing == bs->totbit) {
                    putheader_bits(gfc);
                }
            }
            bs->buf[bs->buf_byte_idx] = 0;
        }

        k = Min(j, bs->buf_bit_idx);
        j -= k;

        bs->buf_bit_idx -= k;

        assert(j < MAX_LENGTH); /* 32 too large on 32 bit machines */
        assert(bs->buf_bit_idx < MAX_LENGTH);

        /* (val >> j) can carry the sign bit; shift it as unsigned so positioning
           it into the byte is defined. Only the low 8 bits are kept. */
        bs->buf[bs->buf_byte_idx] |= (unsigned int) (val >> j) << bs->buf_bit_idx;
        bs->totbit += k;
    }
}

/*write j bits into the bit stream */
inline static void
putbits2(lame_internal_flags * gfc, int val, int j)
{
    putbits_check(gfc, val, j, 1);
}

/*write j bits into the bit stream, ignoring frame headers */
inline static void
putbits_noheaders(lame_internal_flags * gfc, int val, int j)
{
    putbits_check(gfc, val, j, 0);
}

/*
  Some combinations of bitrate, Fs, and stereo make it impossible to stuff
  out a frame using just main_data, due to the limited number of bits to
  indicate main_data_length. In these situations, we put stuffing bits into
  the ancillary data...
*/

inline static void
drain_into_ancillary(lame_internal_flags * gfc, int remainingBits)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncStateVar_t *const esv = &gfc->sv_enc;
    int     i;
    assert(remainingBits >= 0);

    if (remainingBits >= 8) {
        putbits2(gfc, 0x4c, 8);
        remainingBits -= 8;
    }
    if (remainingBits >= 8) {
        putbits2(gfc, 0x41, 8);
        remainingBits -= 8;
    }
    if (remainingBits >= 8) {
        putbits2(gfc, 0x4d, 8);
        remainingBits -= 8;
    }
    if (remainingBits >= 8) {
        putbits2(gfc, 0x45, 8);
        remainingBits -= 8;
    }

    if (remainingBits >= 32) {
        const char *const version = get_lame_short_version();
        for (i = 0; i < (int) strlen(version) && remainingBits >= 8; ++i) {
            remainingBits -= 8;
            putbits2(gfc, version[i], 8);
        }
    }

    for (; remainingBits >= 1; remainingBits -= 1) {
        putbits2(gfc, esv->ancillary_flag, 1);
        esv->ancillary_flag ^= !cfg->disable_reservoir;
    }

    assert(remainingBits == 0);

}

/*write N bits into the header */
inline static void
writeheader(lame_internal_flags * gfc, int val, int j)
{
    EncStateVar_t *const esv = &gfc->sv_enc;
    int     ptr = esv->header[esv->h_ptr].ptr;

    while (j > 0) {
        int const k = Min(j, 8 - (ptr & 7));
        j -= k;
        assert(j < MAX_LENGTH); /* >> 32  too large for 32 bit machines */
        esv->header[esv->h_ptr].buf[ptr >> 3]
            |= (unsigned int) (val >> j) << (8 - (ptr & 7) - k);
        ptr += k;
    }
    esv->header[esv->h_ptr].ptr = ptr;
}


static int
CRC_update(int value, int crc)
{
    /* Shift the running CRC and the data through unsigned locals: crc grows past
       bit 30 here, so the signed <<= 1 was a shift into/of the sign bit. The
       tested bits (0x10000) and the polynomial XOR are unchanged, so the CRC is
       identical. */
    unsigned int uv = (unsigned int) value << 8;
    unsigned int uc = (unsigned int) crc;
    int     i;
    for (i = 0; i < 8; i++) {
        uv <<= 1;
        uc <<= 1;

        if (((uc ^ uv) & 0x10000))
            uc ^= CRC16_POLYNOMIAL;
    }
    return (int) uc;
}


void
CRC_writeheader(lame_internal_flags const *gfc, unsigned char *header)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    int     crc = 0xffff;    /* (jo) init crc16 for error_protection */
    int     i;

    crc = CRC_update(header[2], crc);
    crc = CRC_update(header[3], crc);
    for (i = 6; i < cfg->sideinfo_len; i++) {
        crc = CRC_update(header[i], crc);
    }

    header[4] = crc >> 8;
    header[5] = crc & 255;
}

/**
 * \internal
 * \brief Returns the 32 bits of an MP3 frame header, from the frame sync to
 *        the emphasis.
 * \param cfg            the encoder configuration.
 * \param bitrate_index  the bitrate index of the frame.
 * \param padding        1 for a frame with a padding byte, else 0.
 * \param mode_ext       the mode extension of the frame.
 * \return the header. Its first byte is in the highest 8 bits.
 */
uint32_t
mpeg_header_word(SessionConfig_t const *cfg, int bitrate_index, int padding, int mode_ext)
{
    uint32_t w = 0x7ff;      /* the 11 bits of the frame sync */

#define HEADER_FIELD(n, v) (w = (w << (n)) | ((uint32_t) (v) & ~(~0u << (n))))
    HEADER_FIELD(1, (cfg->samplerate_out < 16000) ? 0 : 1); /* 0 for MPEG-2.5 */
    HEADER_FIELD(1, cfg->version);
    HEADER_FIELD(2, 4 - 3);  /* layer III */
    HEADER_FIELD(1, !cfg->error_protection);
    HEADER_FIELD(4, bitrate_index);
    HEADER_FIELD(2, cfg->samplerate_index);
    HEADER_FIELD(1, padding);
    HEADER_FIELD(1, cfg->extension);
    HEADER_FIELD(2, cfg->mode);
    HEADER_FIELD(2, mode_ext);
    HEADER_FIELD(1, cfg->copyright);
    HEADER_FIELD(1, cfg->original);
    HEADER_FIELD(2, cfg->emphasis);
#undef HEADER_FIELD
    return w;
}

/**
 * \internal
 * \brief Writes the side information of one granule of one channel.
 * \param gfc          the encoder.
 * \param gi           the granule of the channel.
 * \param sfc_bits     the width of scalefac_compress: 4 in MPEG-1, 9 in
 *                     MPEG-2 and MPEG-2.5.
 * \param has_preflag  non-zero for MPEG-1, the only version that has the
 *                     preflag field.
 */
static void
write_granule_side_info(lame_internal_flags * gfc, gr_info const *gi, int sfc_bits, int has_preflag)
{
    writeheader(gfc, gi->huffman.part2_3_length + gi->quant.part2_length, 12);
    writeheader(gfc, gi->huffman.big_values / 2, 9);
    writeheader(gfc, gi->quant.global_gain, 8);
    writeheader(gfc, gi->quant.scalefac_compress, sfc_bits);

    if (gi->granule.block_type != NORM_TYPE) {
        writeheader(gfc, 1, 1); /* window_switching_flag */
        writeheader(gfc, gi->granule.block_type, 2);
        writeheader(gfc, gi->granule.mixed_block_flag, 1);

        writeheader(gfc, resolve_huffman_table(gi->huffman.table_select[0]),
                    5);
        writeheader(gfc, resolve_huffman_table(gi->huffman.table_select[1]),
                    5);

        writeheader(gfc, gi->quant.subblock_gain[0], 3);
        writeheader(gfc, gi->quant.subblock_gain[1], 3);
        writeheader(gfc, gi->quant.subblock_gain[2], 3);
    }
    else {
        writeheader(gfc, 0, 1); /* window_switching_flag */
        writeheader(gfc, resolve_huffman_table(gi->huffman.table_select[0]),
                    5);
        writeheader(gfc, resolve_huffman_table(gi->huffman.table_select[1]),
                    5);
        writeheader(gfc, resolve_huffman_table(gi->huffman.table_select[2]),
                    5);

        assert(0 <= gi->huffman.region0_count && gi->huffman.region0_count < REGION0_COUNT_LIMIT);
        assert(0 <= gi->huffman.region1_count && gi->huffman.region1_count < REGION1_COUNT_LIMIT);
        writeheader(gfc, gi->huffman.region0_count, 4);
        writeheader(gfc, gi->huffman.region1_count, 3);
    }
    if (has_preflag)
        writeheader(gfc, gi->quant.preflag, 1);
    writeheader(gfc, gi->quant.scalefac_scale, 1);
    writeheader(gfc, gi->huffman.count1table_select, 1);
}

inline static void
encodeSideInfo2(lame_internal_flags * gfc, int bitsPerFrame)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncResult_t const *const eov = &gfc->ov_enc;
    EncStateVar_t *const esv = &gfc->sv_enc;
    III_side_info_t *l3_side;
    int     gr, ch;

    l3_side = &gfc->l3_side;
    esv->header[esv->h_ptr].ptr = 0;
    memset(esv->header[esv->h_ptr].buf, 0, cfg->sideinfo_len);
    {
        uint32_t const h = mpeg_header_word(cfg, eov->bitrate_index, eov->padding, eov->mode_ext);
        writeheader(gfc, (int) (h >> 16), 16);
        writeheader(gfc, (int) (h & 0xffff), 16);
    }
    if (cfg->error_protection) {
        writeheader(gfc, 0, 16); /* dummy */
    }

    if (cfg->version == 1) {
        /* MPEG1 */
        assert(l3_side->main_data_begin >= 0);
        writeheader(gfc, (l3_side->main_data_begin), 9);

        if (cfg->channels_out == 2)
            writeheader(gfc, l3_side->private_bits, 3);
        else
            writeheader(gfc, l3_side->private_bits, 5);

        for (ch = 0; ch < cfg->channels_out; ch++) {
            int     band;
            for (band = 0; band < 4; band++) {
                writeheader(gfc, l3_side->scfsi[ch][band], 1);
            }
        }

        for (gr = 0; gr < 2; gr++) {
            for (ch = 0; ch < cfg->channels_out; ch++) {
                write_granule_side_info(gfc, &l3_side->tt[gr][ch], 4, 1);
            }
        }
    }
    else {
        /* MPEG2 */
        assert(l3_side->main_data_begin >= 0);
        writeheader(gfc, (l3_side->main_data_begin), 8);
        writeheader(gfc, l3_side->private_bits, cfg->channels_out);

        gr = 0;
        for (ch = 0; ch < cfg->channels_out; ch++) {
            write_granule_side_info(gfc, &l3_side->tt[gr][ch], 9, 0);
        }
    }

    if (cfg->error_protection) {
        /* (jo) error_protection: add crc16 information to header */
        CRC_writeheader(gfc, esv->header[esv->h_ptr].buf);
    }

    {
        int const old = esv->h_ptr;
        assert(esv->header[old].ptr == cfg->sideinfo_len * 8);

        esv->h_ptr = (old + 1) & (MAX_HEADER_BUF - 1);
        esv->header[esv->h_ptr].write_timing = esv->header[old].write_timing + bitsPerFrame;

        if (esv->h_ptr == esv->w_ptr) {
            /* yikes! we are out of header buffer space */
            ERRORF(gfc, "Error: MAX_HEADER_BUF too small in bitstream.c \n");
        }

    }
}


inline static int
huffman_coder_count1(lame_internal_flags * gfc, gr_info const *gi)
{
    /* Write count1 area */
    struct huffcodetab const *const h = &ht[gi->huffman.count1table_select + 32];
    int     i, bits = 0;

    int const *ix = &gi->quant.l3_enc[gi->huffman.big_values];
    FLOAT const *xr = &gi->granule.xr[gi->huffman.big_values];
    assert(gi->huffman.count1table_select < 2);

    for (i = (gi->huffman.count1 - gi->huffman.big_values) / 4; i > 0; --i) {
        int     huffbits = 0;
        int     p = 0, v;

        v = ix[0];
        if (v) {
            p += 8;
            if (xr[0] < 0.0f)
                huffbits++;
            assert(v <= 1);
        }

        v = ix[1];
        if (v) {
            p += 4;
            huffbits *= 2;
            if (xr[1] < 0.0f)
                huffbits++;
            assert(v <= 1);
        }

        v = ix[2];
        if (v) {
            p += 2;
            huffbits *= 2;
            if (xr[2] < 0.0f)
                huffbits++;
            assert(v <= 1);
        }

        v = ix[3];
        if (v) {
            p++;
            huffbits *= 2;
            if (xr[3] < 0.0f)
                huffbits++;
            assert(v <= 1);
        }

        ix += 4;
        xr += 4;
        putbits2(gfc, huffbits + h->table[p], h->hlen[p]);
        bits += h->hlen[p];
    }
    return bits;
}



/*
  Implements the pseudocode of page 98 of the IS
  */
inline static int
Huffmancode(lame_internal_flags * const gfc, const unsigned int table_select,
            int start, int end, gr_info const *gi)
{
    const unsigned int tableindex = resolve_huffman_table(table_select);
    struct huffcodetab const *const h = &ht[tableindex];
    unsigned int const linbits = h->xlen;
    int     i, bits = 0;

    assert(tableindex < 32u);
    if (!tableindex)
        return bits;

    for (i = start; i < end; i += 2) {
        int16_t  cbits = 0;
        uint16_t xbits = 0;
        unsigned int xlen = h->xlen;
        unsigned int ext = 0;
        unsigned int x1 = gi->quant.l3_enc[i];
        unsigned int x2 = gi->quant.l3_enc[i + 1];

        assert(gi->quant.l3_enc[i] >= 0);
        assert(gi->quant.l3_enc[i+1] >= 0);

        if (x1 != 0u) {
            if (gi->granule.xr[i] < 0.0f)
                ext++;
            cbits--;
        }

        if (tableindex > 15u) {
            /* use ESC-words */
            if (x1 >= 15u) {
                uint16_t const linbits_x1 = x1 - 15u;
                assert(linbits_x1 <= h->linmax);
                ext |= linbits_x1 << 1u;
                xbits = linbits;
                x1 = 15u;
            }

            if (x2 >= 15u) {
                uint16_t const linbits_x2 = x2 - 15u;
                assert(linbits_x2 <= h->linmax);
                ext <<= linbits;
                ext |= linbits_x2;
                xbits += linbits;
                x2 = 15u;
            }
            xlen = 16;
        }

        if (x2 != 0u) {
            ext <<= 1;
            if (gi->granule.xr[i + 1] < 0.0f)
                ext++;
            cbits--;
        }

        assert((x1 | x2) < 16u);

        x1 = x1 * xlen + x2;
        xbits -= cbits;
        cbits += h->hlen[x1];

        assert(cbits <= MAX_LENGTH);
        assert(xbits <= MAX_LENGTH);

        putbits2(gfc, h->table[x1], cbits);
        putbits2(gfc, (int)ext, xbits);
        bits += cbits + xbits;
    }
    return bits;
}

/*
  Note the discussion of huffmancodebits() on pages 28
  and 29 of the IS, as well as the definitions of the side
  information on pages 26 and 27.
  */
static int
ShortHuffmancodebits(lame_internal_flags * gfc, gr_info const *gi)
{
    int     bits;
    int     region1Start;

    region1Start = 3 * gfc->scalefac_band.s[3];
    if (region1Start > gi->huffman.big_values)
        region1Start = gi->huffman.big_values;

    /* short blocks do not have a region2 */
    bits = Huffmancode(gfc, gi->huffman.table_select[0], 0, region1Start, gi);
    bits += Huffmancode(gfc, gi->huffman.table_select[1], region1Start,
                        gi->huffman.big_values, gi);
    return bits;
}

static int
LongHuffmancodebits(lame_internal_flags * gfc, gr_info const *gi)
{
    unsigned int i;
    int     bigvalues, bits;
    int     region1Start, region2Start;

    bigvalues = gi->huffman.big_values;
    assert(0 <= bigvalues && bigvalues <= SAMPLES_PER_GRANULE);

    assert(gi->huffman.region0_count >= -1);
    assert(gi->huffman.region1_count >= -1);
    i = gi->huffman.region0_count + 1;
    assert((size_t) i < dimension_of(gfc->scalefac_band.l));
    region1Start = gfc->scalefac_band.l[i];
    i += gi->huffman.region1_count + 1;
    assert((size_t) i < dimension_of(gfc->scalefac_band.l));
    region2Start = gfc->scalefac_band.l[i];

    if (region1Start > bigvalues)
        region1Start = bigvalues;

    if (region2Start > bigvalues)
        region2Start = bigvalues;

    bits = Huffmancode(gfc, gi->huffman.table_select[0], 0, region1Start, gi);
    bits += Huffmancode(gfc, gi->huffman.table_select[1], region1Start,
                        region2Start, gi);
    bits += Huffmancode(gfc, gi->huffman.table_select[2], region2Start,
                        bigvalues, gi);
    return bits;
}

inline static int
writeMainData(lame_internal_flags * const gfc)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    III_side_info_t const *const l3_side = &gfc->l3_side;
    int     gr, ch, sfb, data_bits, tot_bits = 0;

    if (cfg->version == 1) {
        /* MPEG 1 */
        for (gr = 0; gr < 2; gr++) {
            for (ch = 0; ch < cfg->channels_out; ch++) {
                gr_info const *const gi = &l3_side->tt[gr][ch];
                int const slen1 = slen1_tab[gi->quant.scalefac_compress];
                int const slen2 = slen2_tab[gi->quant.scalefac_compress];
                data_bits = 0;
                for (sfb = 0; sfb < gi->granule.sfbdivide; sfb++) {
                    if (gi->quant.scalefac[sfb] == SCALEFAC_SCFSI)
                        continue; /* scfsi is used */
                    putbits2(gfc, gi->quant.scalefac[sfb], slen1);
                    data_bits += slen1;
                }
                for (; sfb < gi->granule.sfbmax; sfb++) {
                    if (gi->quant.scalefac[sfb] == SCALEFAC_SCFSI)
                        continue; /* scfsi is used */
                    putbits2(gfc, gi->quant.scalefac[sfb], slen2);
                    data_bits += slen2;
                }
                assert(data_bits == gi->quant.part2_length);

                if (gi->granule.block_type == SHORT_TYPE) {
                    data_bits += ShortHuffmancodebits(gfc, gi);
                }
                else {
                    data_bits += LongHuffmancodebits(gfc, gi);
                }
                data_bits += huffman_coder_count1(gfc, gi);
                /* does bitcount in quantize.c agree with actual bit count? */
                assert(data_bits == gi->huffman.part2_3_length + gi->quant.part2_length);
                tot_bits += data_bits;
            }           /* for ch */
        }               /* for gr */
    }
    else {
        /* MPEG 2 */
        gr = 0;
        for (ch = 0; ch < cfg->channels_out; ch++) {
            gr_info const *const gi = &l3_side->tt[gr][ch];
            int     i, sfb_partition, scale_bits = 0;
            assert(gi->quant.sfb_partition_table);
            data_bits = 0;
            sfb = 0;
            sfb_partition = 0;

            if (gi->granule.block_type == SHORT_TYPE) {
                for (; sfb_partition < 4; sfb_partition++) {
                    int const sfbs = gi->quant.sfb_partition_table[sfb_partition] / 3;
                    int const slen = gi->quant.slen[sfb_partition];
                    for (i = 0; i < sfbs; i++, sfb++) {
                        putbits2(gfc, Max(gi->quant.scalefac[sfb * 3 + 0], 0),
                                 slen);
                        putbits2(gfc, Max(gi->quant.scalefac[sfb * 3 + 1], 0),
                                 slen);
                        putbits2(gfc, Max(gi->quant.scalefac[sfb * 3 + 2], 0),
                                 slen);
                        scale_bits += 3 * slen;
                    }
                }
                data_bits += ShortHuffmancodebits(gfc, gi);
            }
            else {
                for (; sfb_partition < 4; sfb_partition++) {
                    int const sfbs = gi->quant.sfb_partition_table[sfb_partition];
                    int const slen = gi->quant.slen[sfb_partition];
                    for (i = 0; i < sfbs; i++, sfb++) {
                        putbits2(gfc, Max(gi->quant.scalefac[sfb], 0), slen);
                        scale_bits += slen;
                    }
                }
                data_bits += LongHuffmancodebits(gfc, gi);
            }
            data_bits += huffman_coder_count1(gfc, gi);
            /* does bitcount in quantize.c agree with actual bit count? */
            assert(data_bits == gi->huffman.part2_3_length);
            assert(scale_bits == gi->quant.part2_length);
            tot_bits += scale_bits + data_bits;
        }               /* for ch */
    }                   /* for gf */
    return tot_bits;
}                       /* main_data */



/* compute the number of bits required to flush all mp3 frames
   currently in the buffer.  This should be the same as the
   reservoir size.  Only call this routine between frames - i.e.
   only after all headers and data have been added to the buffer
   by format_bitstream().

   Also compute total_bits_output =
       size of mp3 buffer (including frame headers which may not
       have yet been send to the mp3 buffer) +
       number of bits needed to flush all mp3 frames.

   total_bytes_output is the size of the mp3 output buffer if
   lame_encode_flush_nogap() was called right now.

 */
int
compute_flushbits(const lame_internal_flags * gfc, int *total_bytes_output)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncStateVar_t const *const esv = &gfc->sv_enc;
    int     flushbits, remaining_headers;
    int     bitsPerFrame;
    int     last_ptr, first_ptr;
    first_ptr = esv->w_ptr; /* first header to add to bitstream */
    last_ptr = esv->h_ptr - 1; /* last header to add to bitstream */
    if (last_ptr == -1)
        last_ptr = MAX_HEADER_BUF - 1;

    /* add this many bits to bitstream so we can flush all headers */
    flushbits = esv->header[last_ptr].write_timing - gfc->bs.totbit;
    *total_bytes_output = flushbits;

    if (flushbits >= 0) {
        /* if flushbits >= 0, some headers have not yet been written */
        /* reduce flushbits by the size of the headers */
        remaining_headers = 1 + last_ptr - first_ptr;
        if (last_ptr < first_ptr)
            remaining_headers = 1 + last_ptr - first_ptr + MAX_HEADER_BUF;
        flushbits -= remaining_headers * 8 * cfg->sideinfo_len;
    }


    /* finally, add some bits so that the last frame is complete
     * these bits are not necessary to decode the last frame, but
     * some decoders will ignore last frame if these bits are missing
     */
    bitsPerFrame = getframebits(gfc);
    flushbits += bitsPerFrame;
    *total_bytes_output += bitsPerFrame;
    /* round up:   */
    if (*total_bytes_output % 8)
        *total_bytes_output = 1 + (*total_bytes_output / 8);
    else
        *total_bytes_output = (*total_bytes_output / 8);
    *total_bytes_output += gfc->bs.buf_byte_idx + 1;

    return flushbits;
}


void
flush_bitstream(lame_internal_flags * gfc)
{
    EncStateVar_t *const esv = &gfc->sv_enc;
    III_side_info_t *l3_side;
    int     nbytes;
    int     flushbits;
    int     last_ptr = esv->h_ptr - 1; /* last header to add to bitstream */
    if (last_ptr == -1)
        last_ptr = MAX_HEADER_BUF - 1;
    l3_side = &gfc->l3_side;


    if ((flushbits = compute_flushbits(gfc, &nbytes)) < 0) {
        ERRORF(gfc, "strange error flushing buffer ... \n");
        return;
    }
    drain_into_ancillary(gfc, flushbits);

    /* check that the 100% of the last frame has been written to bitstream */
    assert(esv->header[last_ptr].write_timing + getframebits(gfc)
           == gfc->bs.totbit);

    /* we have padded out all frames with ancillary data, which is the
       same as filling the bitreservoir with ancillary data, so : */
    esv->ResvSize = 0;
    l3_side->main_data_begin = 0;
}




void
add_dummy_byte(lame_internal_flags * gfc, unsigned char val, unsigned int n)
{
    EncStateVar_t *const esv = &gfc->sv_enc;
    int     i;

    while (n-- > 0u) {
        putbits_noheaders(gfc, val, 8);

        for (i = 0; i < MAX_HEADER_BUF; ++i)
            esv->header[i].write_timing += 8;
    }
}


/*
  format_bitstream()

  This is called after a frame of audio has been quantized and coded.
  It will write the encoded audio to the bitstream. Note that
  from a layer3 encoder's perspective the bit stream is primarily
  a series of main_data() blocks, with header and side information
  inserted at the proper locations to maintain framing. (See Figure A.7
  in the IS).

  Returns 0, or LAME_INTERNALERROR when the bits written disagree with the
  bit reservoir. The error is kept: every later encode call fails too.
  */
int
format_bitstream(lame_internal_flags * gfc)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncStateVar_t *const esv = &gfc->sv_enc;
    int     bits, nbytes;
    int     consistent = 1;
    III_side_info_t *l3_side;
    int     bitsPerFrame;
    l3_side = &gfc->l3_side;

    bitsPerFrame = getframebits(gfc);
    drain_into_ancillary(gfc, l3_side->resvDrain_pre);

    encodeSideInfo2(gfc, bitsPerFrame);
    bits = 8 * cfg->sideinfo_len;
    bits += writeMainData(gfc);
    drain_into_ancillary(gfc, l3_side->resvDrain_post);
    bits += l3_side->resvDrain_post;

    l3_side->main_data_begin += (bitsPerFrame - bits) / 8;

    /* compare number of bits needed to clear all buffered mp3 frames
     * with what we think the resvsize is: */
    if (compute_flushbits(gfc, &nbytes) != esv->ResvSize) {
        ERRORF(gfc, "Internal buffer inconsistency. flushbits <> ResvSize");
        consistent = 0;
    }


    /* compare main_data_begin for the next frame with what we
     * think the resvsize is: */
    if ((l3_side->main_data_begin * 8) != esv->ResvSize) {
        ERRORF(gfc, "bit reservoir error: \n"
               "l3_side->main_data_begin: %i \n"
               "Resvoir size:             %i \n"
               "resv drain (post)         %i \n"
               "resv drain (pre)          %i \n"
               "header and sideinfo:      %i \n"
               "data bits:                %i \n"
               "total bits:               %i (remainder: %i) \n"
               "bitsperframe:             %i \n",
               8 * l3_side->main_data_begin,
               esv->ResvSize,
               l3_side->resvDrain_post,
               l3_side->resvDrain_pre,
               8 * cfg->sideinfo_len,
               bits - l3_side->resvDrain_post - 8 * cfg->sideinfo_len,
               bits, bits % 8, bitsPerFrame);

        ERRORF(gfc, "This is a fatal error.  It has several possible causes:");
        ERRORF(gfc, "90%%  LAME compiled with buggy version of gcc using advanced optimizations");
        ERRORF(gfc, " 9%%  Your system is overclocked");
        ERRORF(gfc, " 1%%  bug in LAME encoding library");

        esv->ResvSize = l3_side->main_data_begin * 8;
        consistent = 0;
    }
    assert(gfc->bs.totbit % 8 == 0);

    if (gfc->bs.totbit > 1000000000) {
        /* to avoid totbit overflow, (at 8h encoding at 128kbs) lets reset bit counter */
        int     i;
        for (i = 0; i < MAX_HEADER_BUF; ++i)
            esv->header[i].write_timing -= gfc->bs.totbit;
        gfc->bs.totbit = 0;
    }

    if (!consistent) {
        esv->internal_error = 1;
        return LAME_INTERNALERROR;
    }
    return 0;
}


static int
do_gain_analysis(LAME_UNUSED lame_internal_flags * gfc, LAME_UNUSED unsigned char* buffer,
                 int minimum)
{
#ifdef HAVE_MPG123
    SessionConfig_t const *const cfg = &gfc->cfg;
    RpgStateVar_t const *const rsv = &gfc->sv_rpg;
    RpgResult_t *const rov = &gfc->ov_rpg;
    if (cfg->decode_on_the_fly) { /* decode the frame */
        sample_t pcm_buf[2][1152];
        int     mp3_in = minimum;
        int     samples_out = -1;

        /* re-synthesis to pcm.  Repeat until we get a samples_out=0 */
        while (samples_out != 0) {

            samples_out = hip_decode1_unclipped(gfc->hip, buffer, mp3_in, pcm_buf[0], pcm_buf[1]);
            /* samples_out = 0:  need more data to decode
             * samples_out = -1:  error.  Lets assume 0 pcm output
             * samples_out = number of samples output */

            /* set the lenght of the mp3 input buffer to zero, so that in the
             * next iteration of the loop we will be querying mpglib about
             * buffered data */
            mp3_in = 0;

            if (samples_out == -1) {
                /* error decoding. Not fatal, but might screw up
                 * the ReplayGain tag. What should we do? Ignore for now */
                samples_out = 0;
            }
            if (samples_out > 0) {
                /* process the PCM data */

                /* this should not be possible, and indicates we have
                 * overflown the pcm_buf buffer */
                assert(samples_out <= 1152);

                if (cfg->findPeakSample) {
                    int     i;
                    /* FIXME: is this correct? maybe Max(fabs(pcm),PeakSample) */
                    for (i = 0; i < samples_out; i++) {
                        if (pcm_buf[0][i] > rov->PeakSample)
                            rov->PeakSample = pcm_buf[0][i];
                        else if (-pcm_buf[0][i] > rov->PeakSample)
                            rov->PeakSample = -pcm_buf[0][i];
                    }
                    if (cfg->channels_out > 1)
                        for (i = 0; i < samples_out; i++) {
                            if (pcm_buf[1][i] > rov->PeakSample)
                                rov->PeakSample = pcm_buf[1][i];
                            else if (-pcm_buf[1][i] > rov->PeakSample)
                                rov->PeakSample = -pcm_buf[1][i];
                        }
                }

                if (cfg->findReplayGain)
                    if (AnalyzeSamples
                        (rsv->rgdata, pcm_buf[0], pcm_buf[1], samples_out,
                         cfg->channels_out) == GAIN_ANALYSIS_ERROR)
                        return -6;

            }       /* if (samples_out>0) */
        }           /* while (samples_out!=0) */
    }               /* if (gfc->decode_on_the_fly) */
#endif
    return minimum;
}

static int
do_copy_buffer(lame_internal_flags * gfc, unsigned char *buffer, int size)
{
    Bit_stream_struc *const bs = &gfc->bs;
    int const minimum = bs->buf_byte_idx + 1;
    if (minimum <= 0)
        return 0;
    if (minimum > size || buffer == NULL)
        return -1;      /* buffer is too small, or there is none */
    memcpy(buffer, bs->buf, minimum);
    bs->buf_byte_idx = -1;
    bs->buf_bit_idx = 0;
    return minimum;
}

/* copy data out of the internal MP3 bit buffer into a user supplied
   unsigned char buffer.

   mp3data=0      indicates data in buffer is an id3tags and VBR tags
   mp3data=1      data is real mp3 frame data.


*/
int
copy_buffer(lame_internal_flags * gfc, unsigned char *buffer, int size, int mp3data)
{
    int const minimum = do_copy_buffer(gfc, buffer, size);
    if (minimum > 0 && mp3data) {
        UpdateMusicCRC(&gfc->nMusicCRC, buffer, minimum);

        /* sum number of bytes belonging to the mp3 stream
         *  this info will be written into the Xing/LAME header for seeking
         */
        gfc->VBR_seek_table.nBytesWritten += minimum;

        return do_gain_analysis(gfc, buffer, minimum);
    }                   /* if (mp3data) */
    return minimum;
}


int
init_bit_stream_w(lame_internal_flags * gfc)
{
    EncStateVar_t *const esv = &gfc->sv_enc;

    esv->h_ptr = esv->w_ptr = 0;
    esv->header[esv->h_ptr].write_timing = 0;

    gfc->bs.buf = lame_calloc(unsigned char, BUFFER_SIZE);
    if (gfc->bs.buf == NULL) {
        ERRORF(gfc, "Error: can't allocate bitstream buffer\n");
        return -1;
    }
    gfc->bs.buf_size = BUFFER_SIZE;
    gfc->bs.buf_byte_idx = -1;
    gfc->bs.buf_bit_idx = 0;
    gfc->bs.totbit = 0;
    return 0;
}

/* end of bitstream.c */
