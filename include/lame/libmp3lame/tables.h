/*
 *	MPEG layer 3 tables include file
 *
 *	Copyright (c) 1999 Albert L Faber
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

#ifndef LAME_TABLES_H
#define LAME_TABLES_H

#define HTN	34

struct huffcodetab {
    const unsigned int xlen;          /* max. x-index+   */
    const unsigned int linmax;        /* max number to be stored in linbits */
    const uint16_t *table;      /* pointer to array[xlen][ylen]  */
    const uint8_t *hlen;        /* pointer to array[xlen][ylen]  */
};

extern const struct huffcodetab ht[HTN];
    /* global memory block   */
    /* array of all huffcodtable headers */
    /* 0..31 Huffman code table 0..31  */
    /* 32,33 count1-tables   */

/*! Maps a chosen table to the table whose codewords encode it. */
unsigned int resolve_huffman_table(unsigned int table_select);

extern const uint8_t t32l[];
extern const uint8_t t33l[];

extern const uint32_t largetbl[16 * 16];
extern const uint32_t table23[3 * 3];
extern const uint32_t table56[4 * 4];

extern const int scfsi_band[5];

extern const int bitrate_table    [3][16];
extern const int samplerate_table [3][ 4];
extern const int full_bitrate_table[17];

#endif /* LAME_TABLES_H */
