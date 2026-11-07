/*
 * id3tag.c -- Write ID3 version 1 and 2 tags.
 *
 * Copyright (C) 2000 Don Melton
 * Copyright (C) 2011-2017 Robert Hegemann
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

/*
 * HISTORY: This source file is part of LAME (see https://lame.sourceforge.io/)
 * and was originally adapted by Conrad Sanderson <c.sanderson@me.gu.edu.au>
 * from mp3info by Ricardo Cerqueira <rmc@rccn.net> to write only ID3 version 1
 * tags.  Don Melton <don@blivet.com> COMPLETELY rewrote it to support version
 * 2 tags and be more conformant to other standards while remaining flexible.
 *
 * NOTE: See http://id3.org/ for more information about ID3 tag formats.
 */

/* $Id$ */

/*!
  \file   id3tag.c
  \brief  The metadata interface of the public API.

  The group \ref api_tags describes the public tag functions of this file.
  Most of this file is the tag code behind them and is not part of the
  interface.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef STDC_HEADERS
# include <stddef.h>
# include <stdlib.h>
# include <string.h>
# include <ctype.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char   *strchr(), *strrchr();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
# endif
#endif


#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "id3tag.h"
#include "lame_global_flags.h"
#include "util.h"
#include "obsolete_api.h"
#include "bitstream.h"


/** \internal \brief The result of a tag setter when memory runs out. */
#define ID3_RESULT_NO_MEMORY (-254)
/** \internal \brief The result of a tag setter that cannot write the frame. */
#define ID3_RESULT_NOT_WRITABLE (-255)

static const char *const genre_names[] = {
    /*
     * NOTE: The spelling of these genre names is identical to those found in
     * Winamp and mp3info.
     */
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge",
    "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies", "Other", "Pop", "R&B",
    "Rap", "Reggae", "Rock", "Techno", "Industrial", "Alternative", "Ska",
    "Death Metal", "Pranks", "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop",
    "Vocal", "Jazz+Funk", "Fusion", "Trance", "Classical", "Instrumental",
    "Acid", "House", "Game", "Sound Clip", "Gospel", "Noise", "Alternative Rock",
    "Bass", "Soul", "Punk", "Space", "Meditative", "Instrumental Pop",
    "Instrumental Rock", "Ethnic", "Gothic", "Darkwave", "Techno-Industrial",
    "Electronic", "Pop-Folk", "Eurodance", "Dream", "Southern Rock", "Comedy",
    "Cult", "Gangsta", "Top 40", "Christian Rap", "Pop/Funk", "Jungle",
    "Native US", "Cabaret", "New Wave", "Psychedelic", "Rave",
    "Showtunes", "Trailer", "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz",
    "Polka", "Retro", "Musical", "Rock & Roll", "Hard Rock", "Folk",
    "Folk-Rock", "National Folk", "Swing", "Fast Fusion", "Bebob", "Latin",
    "Revival", "Celtic", "Bluegrass", "Avantgarde", "Gothic Rock",
    "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock",
    "Big Band", "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech",
    "Chanson", "Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass",
    "Primus", "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
    "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet",
    "Punk Rock", "Drum Solo", "A Cappella", "Euro-House", "Dance Hall",
    "Goa", "Drum & Bass", "Club-House", "Hardcore", "Terror", "Indie",
    "BritPop", "Negerpunk", "Polsk Punk", "Beat", "Christian Gangsta",
    "Heavy Metal", "Black Metal", "Crossover", "Contemporary Christian",
    "Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime", "JPop",
    "SynthPop"
};

#define GENRE_NAME_COUNT \
    ((int)(sizeof genre_names / sizeof (const char *const)))

static const int genre_alpha_map[] = {
    123, 34, 74, 73, 99, 20, 40, 26, 145, 90, 116, 41, 135, 85, 96, 138, 89, 0,
    107, 132, 65, 88, 104, 102, 97, 136, 61, 141, 32, 1, 112, 128, 57, 140, 2,
    139, 58, 3, 125, 50, 22, 4, 55, 127, 122, 120, 98, 52, 48, 54, 124, 25, 84,
    80, 115, 81, 119, 5, 30, 36, 59, 126, 38, 49, 91, 6, 129, 79, 137, 7, 35,
    100, 131, 19, 33, 46, 47, 8, 29, 146, 63, 86, 71, 45, 142, 9, 77, 82, 64,
    133, 10, 66, 39, 11, 103, 12, 75, 134, 13, 53, 62, 109, 117, 23, 108, 92,
    67, 93, 43, 121, 15, 68, 14, 16, 76, 87, 118, 17, 78, 143, 114, 110, 69, 21,
    111, 95, 105, 42, 37, 24, 56, 44, 101, 83, 94, 106, 147, 113, 18, 51, 130,
    144, 60, 70, 31, 72, 27, 28
};

#define GENRE_ALPHA_COUNT ((int)(sizeof genre_alpha_map / sizeof (int)))

#define GENRE_INDEX_OTHER 12


#define FRAME_ID(a, b, c, d) \
    ( ((unsigned long)(a) << 24) \
    | ((unsigned long)(b) << 16) \
    | ((unsigned long)(c) <<  8) \
    | ((unsigned long)(d) <<  0) )

typedef enum UsualStringIDs { ID_TITLE = FRAME_ID('T', 'I', 'T', '2')
        , ID_ARTIST = FRAME_ID('T', 'P', 'E', '1')
        , ID_ALBUM = FRAME_ID('T', 'A', 'L', 'B')
        , ID_GENRE = FRAME_ID('T', 'C', 'O', 'N')
        , ID_ENCODER = FRAME_ID('T', 'S', 'S', 'E')
        , ID_PLAYLENGTH = FRAME_ID('T', 'L', 'E', 'N')
        , ID_COMMENT = FRAME_ID('C', 'O', 'M', 'M') /* full text string */
} UsualStringIDs;

typedef enum NumericStringIDs { ID_DATE = FRAME_ID('T', 'D', 'A', 'T') /* "ddMM" */
        , ID_TIME = FRAME_ID('T', 'I', 'M', 'E') /* "hhmm" */
        , ID_TPOS = FRAME_ID('T', 'P', 'O', 'S') /* '0'-'9' and '/' allowed */
        , ID_TRACK = FRAME_ID('T', 'R', 'C', 'K') /* '0'-'9' and '/' allowed */
        , ID_YEAR = FRAME_ID('T', 'Y', 'E', 'R') /* "yyyy" */
} NumericStringIDs;

typedef enum MiscIDs { ID_TXXX = FRAME_ID('T', 'X', 'X', 'X')
        , ID_WXXX = FRAME_ID('W', 'X', 'X', 'X')
        , ID_SYLT = FRAME_ID('S', 'Y', 'L', 'T')
        , ID_APIC = FRAME_ID('A', 'P', 'I', 'C')
        , ID_GEOB = FRAME_ID('G', 'E', 'O', 'B')
        , ID_PCNT = FRAME_ID('P', 'C', 'N', 'T')
        , ID_AENC = FRAME_ID('A', 'E', 'N', 'C')
        , ID_LINK = FRAME_ID('L', 'I', 'N', 'K')
        , ID_ENCR = FRAME_ID('E', 'N', 'C', 'R')
        , ID_GRID = FRAME_ID('G', 'R', 'I', 'D')
        , ID_PRIV = FRAME_ID('P', 'R', 'I', 'V')
        , ID_USLT = FRAME_ID('U', 'S', 'L', 'T') /* full text string */
        , ID_USER = FRAME_ID('U', 'S', 'E', 'R') /* full text string */
        , ID_PCST = FRAME_ID('P', 'C', 'S', 'T') /* iTunes Podcast indicator, only presence important */
        , ID_WFED = FRAME_ID('W', 'F', 'E', 'D') /* iTunes Podcast URL as TEXT FRAME !!! violates standard */
} MiscIDs;


static int
frame_id_matches(int id, int mask)
{
    /* window is a byte mask that walks up to 0xff000000; keep it unsigned so
       reaching and shifting past the top byte is a defined wrap rather than a
       signed overflow / shift of a negative value. The masks are bit patterns,
       so the results are unchanged. */
    int     result = 0, i;
    unsigned int window = 0xff;
    for (i = 0; i < 4; ++i, window <<= 8) {
        int const mw = (int) (mask & window);
        int const iw = (int) (id & window);
        if (mw != 0 && mw != iw) {
            result |= iw;
        }
    }
    return result;
}

static int
isFrameIdMatching(int id, int mask)
{
    return frame_id_matches(id, mask) == 0 ? 1 : 0;
}

static int
test_tag_spec_flags(lame_internal_flags const *gfc, unsigned int tst)
{
    return (gfc->tag_spec.flags & tst) != 0u ? 1 : 0;
}

static int
is_lame_internal_flags_null(lame_t gfp)
{
    return (gfp && gfp->internal_flags) ? 0 : 1;
}

// moved this function up so it can be used in copyV1ToV2 (for ID3v2.4 support)
static char const*
id3v2_get_language(lame_t gfp)
{
    lame_internal_flags const* gfc = gfp ? gfp->internal_flags : 0;
    if (gfc) return gfc->tag_spec.language;
    return 0;
}

static int
id3v2_add_ucs2_lng(lame_t gfp, uint32_t frame_id, unsigned short const *desc, unsigned short const *text);
static int
id3v2_add_latin1_lng(lame_t gfp, uint32_t frame_id, char const *desc, char const *text);

// forward-declare the id3v2_add_utf8_lng function to be used in copyV1ToV2
static int
id3v2_add_utf8_lng(lame_t gfp, uint32_t frame_id, char const *desc, char const *text);


static void
copyV1ToV2(lame_t gfp, int frame_id, char const *s)
{
    lame_internal_flags *gfc = gfp != 0 ? gfp->internal_flags : 0;
    if (gfc != 0) {
        unsigned int flags = gfc->tag_spec.flags;
        
        // add V2 tag: either ID3v2.3 or ID3v2.4 according to
        // whether V2_4_UTF8_FLAG is used
        
        // if we are writing UTF-8 ID3v2.4 tag
        if (test_tag_spec_flags(gfc, V2_4_UTF8_FLAG)) {
            id3v2_add_utf8_lng(gfp, frame_id, 0, s);
        } else { // if we are writing latin1 ID3v2.3 tag
            id3v2_add_latin1_lng(gfp, frame_id, 0, s);
        }
        gfc->tag_spec.flags = flags;
    }
}


static void
id3v2AddLameVersion(lame_t gfp)
{
    char    buffer[1024];
    const char *b = get_lame_os_bitness();
    const char *v = get_lame_version();
    const char *u = get_lame_url();
    const size_t lenb = strlen(b);

    /* each part is bounded, so the text always fits the buffer */
    if (lenb > 0) {
        snprintf(buffer, sizeof(buffer), "LAME %.8s version %.256s (%.256s)", b, v, u);
    }
    else {
        snprintf(buffer, sizeof(buffer), "LAME version %.256s (%.256s)", v, u);
    }
    copyV1ToV2(gfp, ID_ENCODER, buffer);
}

static void
id3v2AddAudioDuration(lame_t gfp, double ms)
{
    SessionConfig_t const *const cfg = &gfp->internal_flags->cfg; /* caller checked pointers */
    char    buffer[1024];
    uint64_t const max_ms = (uint64_t) -1;      /* full width of the playlength field */
    double const max_ms_dbl = (double) max_ms;  /* rounds up past the field, so use >= */
    uint64_t playlength_ms;

    ms *= 1000;
    ms /= cfg->samplerate_in;
    if (ms >= max_ms_dbl) {
        playlength_ms = max_ms;
    }
    else if (ms < 0) {
        playlength_ms = 0;
    }
    else {
        playlength_ms = (uint64_t) ms;
    }
    snprintf(buffer, sizeof(buffer), "%llu", (unsigned long long) playlength_ms);
    copyV1ToV2(gfp, ID_PLAYLENGTH, buffer);
}

/*! Enumerate the ID3v1 genres. */
/*!
  \ingroup api_tags
  Calls \a handler once for each genre in the ID3v1 list, in alphabetical
  order, with the number and the name of the genre. Use it to build a menu, or
  to check a name before you pass it to \c id3tag_set_genre().

  It needs no encoder instance, because the list belongs to the format, not to
  an encode.

  \param handler  called as <tt>handler(number, name, cookie)</tt>. If it is
                  NULL, the call does nothing.
  \param cookie   passed to \a handler unchanged.
*/
void
id3tag_genre_list(void (*handler) (int, const char *, void *), void *cookie)
{
    if (handler) {
        int     i;
        for (i = 0; i < GENRE_ALPHA_COUNT; ++i) {
            int     j = genre_alpha_map[i];
            handler(j, genre_names[j], cookie);
        }
    }
}

#define GENRE_NUM_UNKNOWN 255



/*! Discard every tag field set so far. */
/*!
  \ingroup api_tags
  Frees all tag data that was set and returns the instance to its initial tag
  state: no fields, no genre, ID3v2 padding at its default of 128 bytes, and
  the LAME version in the tag, as on a new instance.

  You do not need to call it before setting fields, because a new instance is
  already in this state. Call it when you reuse an instance for a second file,
  so that the tags of the first file are not kept.

  \param gfp the encoder instance.
*/
void
id3tag_init(lame_t gfp)
{
    lame_internal_flags *gfc = 0;

    if (is_lame_internal_flags_null(gfp)) {
        return;
    }
    gfc = gfp->internal_flags;
    free_id3tag(gfc);
    memset(&gfc->tag_spec, 0, sizeof gfc->tag_spec);
    gfc->tag_spec.genre_id3v1 = GENRE_NUM_UNKNOWN;
    gfc->tag_spec.padding_size = 128;
    id3v2AddLameVersion(gfp);
}



/*! Write an ID3v2 tag as well as the ID3v1 one. */
/*!
  \ingroup api_tags
  Always writes both tags. Without this call, LAME writes an ID3v2 tag only
  when a field needs one. Use it when a program that reads the file expects an
  ID3v2 tag in every file.

  Cancels a previous \c id3tag_v1_only().

  \param gfp the encoder instance.
*/
void
id3tag_add_v2(lame_t gfp)
{
    lame_internal_flags *gfc = 0;

    if (is_lame_internal_flags_null(gfp)) {
        return;
    }
    gfc = gfp->internal_flags;
    gfc->tag_spec.flags &= ~V1_ONLY_FLAG;
    gfc->tag_spec.flags |= ADD_V2_FLAG;
}

/*! Write a UTF-8 ID3v2.4 tag as well as the ID3v1 one. */
/*!
  \ingroup api_tags
  As \c id3tag_add_v2(), but the ID3v2 tag is written in version 2.4, with
  the text in UTF-8. Choose this for text outside Latin-1: non-European
  scripts, or European text with characters that the encodings of ID3v2.3
  cannot represent.

  The ID3v1 tag does not change and is always Latin-1. So a title in a
  non-Latin script is kept in the ID3v2 tag only.

  \param gfp the encoder instance.
*/
void
id3tag_add_v2_4_UTF8(lame_t gfp)
{
    lame_internal_flags *gfc = 0;

    if (is_lame_internal_flags_null(gfp)) {
        return;
    }
    gfc = gfp->internal_flags;
    // use both ID3v1 and ID3v2 tags
    gfc->tag_spec.flags &= ~V1_ONLY_FLAG;
    gfc->tag_spec.flags |= ADD_V2_FLAG;
    
    // for ID3v2 tags, specifically use ID3v2.4 with UTF-8 characters
    gfc->tag_spec.flags |= V2_4_UTF8_FLAG;
}

/*! Write only the ID3v1 tag. */
/*!
  \ingroup api_tags
  Writes no ID3v2 tag, even when a field needs one. Text that does not fit
  into ID3v1 is lost without a warning. Use it for players that cannot handle
  a tag before the audio.

  \param gfp the encoder instance.
*/
void
id3tag_v1_only(lame_t gfp)
{
    lame_internal_flags *gfc = 0;

    if (is_lame_internal_flags_null(gfp)) {
        return;
    }
    gfc = gfp->internal_flags;
    gfc->tag_spec.flags &= ~(ADD_V2_FLAG | V2_ONLY_FLAG);
    gfc->tag_spec.flags |= V1_ONLY_FLAG;
}

/*! Write only the ID3v2 tag. */
/*!
  \ingroup api_tags
  Writes no 128-byte ID3v1 tag at the end of the file. Nothing is lost,
  because every ID3v1 field also exists in ID3v2. The file is cleaner for
  programs that read ID3v2.

  \param gfp the encoder instance.
*/
void
id3tag_v2_only(lame_t gfp)
{
    lame_internal_flags *gfc = 0;

    if (is_lame_internal_flags_null(gfp)) {
        return;
    }
    gfc = gfp->internal_flags;
    gfc->tag_spec.flags &= ~V1_ONLY_FLAG;
    gfc->tag_spec.flags |= V2_ONLY_FLAG;
}

/*! Write only a UTF-8 ID3v2.4 tag. */
/*!
  \ingroup api_tags
  \c id3tag_v2_only() with the version and encoding of
  \c id3tag_add_v2_4_UTF8(): one tag, ID3v2.4, text in UTF-8. Choose this
  when the tag text is not Latin-1 and a cut ID3v1 copy would be worse than no
  ID3v1 tag.

  \param gfp the encoder instance.
*/
void
id3tag_v2_4_UTF8_only(lame_t gfp)
{
    lame_internal_flags *gfc = 0;

    if (is_lame_internal_flags_null(gfp)) {
        return;
    }
    gfc = gfp->internal_flags;
    
    // use ID3v2 tags and not ID3v1
    gfc->tag_spec.flags &= ~V1_ONLY_FLAG;
    gfc->tag_spec.flags |= V2_ONLY_FLAG;
    
    // for ID3v2 tags, specifically use ID3v2.4 with UTF-8 characters
    gfc->tag_spec.flags |= V2_4_UTF8_FLAG;
}

/*! Pad the ID3v1 fields with spaces instead of zeros. */
/*!
  \ingroup api_tags
  ID3v1 fields have a fixed length, and the standard does not say what fills
  the unused bytes. LAME writes zeros. This function selects spaces, which a
  few older readers need. The text is the same in both cases.

  It also cancels \c id3tag_v2_only(), because this setting needs an ID3v1
  tag.

  \param gfp the encoder instance.
*/
void
id3tag_space_v1(lame_t gfp)
{
    lame_internal_flags *gfc = 0;

    if (is_lame_internal_flags_null(gfp)) {
        return;
    }
    gfc = gfp->internal_flags;
    gfc->tag_spec.flags &= ~V2_ONLY_FLAG;
    gfc->tag_spec.flags |= SPACE_V1_FLAG;
}

/*! Pad the ID3v2 tag with the default amount of free space. */
/*!
  \ingroup api_tags
  \c id3tag_set_pad() with 128 bytes. With padding, a tag editor can make the
  tag larger later without rewriting the whole file.

  \param gfp the encoder instance.
*/
void
id3tag_pad_v2(lame_t gfp)
{
    id3tag_set_pad(gfp, 128);
}

/*! Reserve free space at the end of the ID3v2 tag. */
/*!
  \ingroup api_tags
  Writes \a n bytes of padding after the frames of the tag. An editor can then
  add or extend a frame later by overwriting the padding, without moving the
  audio. More padding only makes the file larger.

  Padding needs an ID3v2 tag, so this function also turns one on.

  \param gfp  the encoder instance.
  \param n    padding size in bytes. A value above \c LAME_MAXID3V2PADDING
              is reduced to it.
*/
void
id3tag_set_pad(lame_t gfp, size_t n)
{
    lame_internal_flags *gfc = 0;

    if (is_lame_internal_flags_null(gfp)) {
        return;
    }
    gfc = gfp->internal_flags;
    gfc->tag_spec.flags &= ~V1_ONLY_FLAG;
    gfc->tag_spec.flags |= PAD_V2_FLAG;
    gfc->tag_spec.flags |= ADD_V2_FLAG;
    if (n > LAME_MAXID3V2PADDING) {
        n = LAME_MAXID3V2PADDING;
    }
    gfc->tag_spec.padding_size = (unsigned int)n;
}

static int
hasUcs2ByteOrderMarker(unsigned short bom)
{
    if (bom == 0xFFFEu || bom == 0xFEFFu) {
        return 1;
    }
    return 0;
}


static unsigned short
swap_bytes(unsigned short w)
{
    return (0xff00u & (w << 8)) | (0x00ffu & (w >> 8));
}


static unsigned short
toLittleEndian(unsigned short bom, unsigned short c)
{
    if (bom == 0xFFFEu) {
        return swap_bytes(c);
    }
    return c;
}

static unsigned short
fromLatin1Char(const unsigned short* s, unsigned short c)
{
    if (s[0] == 0xFFFEu) {
        return swap_bytes(c);
    }
    return c;
}


static  size_t
local_strdup(char **dst, const char *src)
{
    if (dst == 0) {
        return 0;
    }
    free(*dst);
    *dst = 0;
    if (src != 0) {
        size_t  n;
        for (n = 0; src[n] != 0; ++n) { /* calc src string length */
        }
        if (n > 0) {    /* string length without zero termination */
            *dst = lame_calloc(char, n + 1);
            if (*dst != 0) {
                memcpy(*dst, src, n * sizeof(**dst));
                (*dst)[n] = 0;
                return n;
            }
        }
    }
    return 0;
}

static  size_t
local_ucs2_strdup(unsigned short **dst, unsigned short const *src)
{
    if (dst == 0) {
        return 0;
    }
    free(*dst);         /* free old string pointer */
    *dst = 0;
    if (src != 0) {
        size_t  n;
        for (n = 0; src[n] != 0; ++n) { /* calc src string length */
        }
        if (n > 0) {    /* string length without zero termination */
            assert(sizeof(*src) >= 2);
            *dst = lame_calloc(unsigned short, n + 1);
            if (*dst != 0) {
                memcpy(*dst, src, n * sizeof(**dst));
                (*dst)[n] = 0;
                return n;
            }
        }
    }
    return 0;
}


static  size_t
local_ucs2_strlen(unsigned short const *s)
{
    size_t  n = 0;
    if (s != 0) {
        while (*s++) {
            ++n;
        }
    }
    return n;
}


static size_t
local_ucs2_substr(unsigned short** dst, unsigned short const* src, size_t start, size_t end)
{
    size_t const len = 1 + 1 + ((start < end) ? (end - start) : 0);
    size_t n = 0;
    unsigned short *ptr = lame_calloc(unsigned short, len);
    *dst = ptr;
    if (ptr == 0 || src == 0) {
        return 0;
    }
    if (hasUcs2ByteOrderMarker(src[0])) {
        ptr[n++] = src[0];
        if (start == 0) {
            ++start;
        }
    }
    while (start < end) {
        ptr[n++] = src[start++];
    }
    ptr[n] = 0;
    return n;
}

static int
local_ucs2_pos(unsigned short const* str, unsigned short c)
{
    int     i;
    for (i = 0; str != 0 && str[i] != 0; ++i) {
        if (str[i] == c) {
            return i;
        }
    }
    return -1;
}

static int
local_char_pos(char const* str, char c)
{
    int     i;
    for (i = 0; str != 0 && str[i] != 0; ++i) {
        if (str[i] == c) {
            return i;
        }
    }
    return -1;
}

static int
maybeLatin1(unsigned short const* text)
{
    if (text) {
        unsigned short bom = *text++;
        while (*text) {
            unsigned short c = toLittleEndian(bom, *text++);
            if (c > 0x00fe) return 0;
        }
    }
    return 1;
}

static int searchGenre(char const* genre);
static int sloppySearchGenre(char const* genre);

static int
lookupGenre(char const* genre)
{
    char   *str;
    long const number = strtol(genre, &str, 10);
    int     num;
    /* is the input a string or a valid number? */
    if (*str) {
        num = searchGenre(genre);
        if (num == GENRE_NAME_COUNT) {
            num = sloppySearchGenre(genre);
        }
        if (num == GENRE_NAME_COUNT) {
            return -2; /* no common genre text found */
        }
    }
    else {
        if ((number < 0) || (number >= GENRE_NAME_COUNT)) {
            return -1; /* number unknown */
        }
        num = (int) number;
    }
    return num;
}

static unsigned char *
writeLoBytes(unsigned char *frame, unsigned short const *str, size_t n);

static char*
local_strdup_utf16_to_latin1(unsigned short const* utf16)
{
    size_t  len = local_ucs2_strlen(utf16);
    unsigned char* latin1 = lame_calloc(unsigned char, len+1);
    if (latin1 != 0) {
        writeLoBytes(latin1, utf16, len);
    }
    return (char*)latin1;
}


static int
id3tag_set_genre_utf8(lame_t gfp, char const* text)
{
    lame_internal_flags* gfc = gfp->internal_flags;
    int   ret;
    if (text == 0) {
        return -3;
    }
    {
        int     num = lookupGenre(text);
        if (num == -1) return -1; /* number out of range */
        if (num >= 0) {           /* common genre found  */
            gfc->tag_spec.flags |= CHANGED_FLAG;
            gfc->tag_spec.genre_id3v1 = num;
            copyV1ToV2(gfp, ID_GENRE, genre_names[num]);
            return 0;
        }
    }
    ret = id3v2_add_utf8_lng(gfp, ID_GENRE, 0, text);
    if (ret == 0) {
        gfc->tag_spec.flags |= CHANGED_FLAG;
        gfc->tag_spec.genre_id3v1 = GENRE_INDEX_OTHER;
    }
    return ret;
}

static int
id3tag_set_genre_utf16(lame_t gfp, unsigned short const* text)
{
    lame_internal_flags* gfc = gfp->internal_flags;
    int   ret;
    if (text == 0) {
        return -3;
    }
    if (!hasUcs2ByteOrderMarker(text[0])) {
        return -3;
    }
    if (maybeLatin1(text)) {
        char*   latin1 = local_strdup_utf16_to_latin1(text);
        int     num;
        if (latin1 == 0) {
            return ID3_RESULT_NO_MEMORY;
        }
        num = lookupGenre(latin1);
        free(latin1);
        if (num == -1) return -1; /* number out of range */
        if (num >= 0) {           /* common genre found  */
            gfc->tag_spec.flags |= CHANGED_FLAG;
            gfc->tag_spec.genre_id3v1 = num;
            copyV1ToV2(gfp, ID_GENRE, genre_names[num]);
            return 0;
        }
    }
    ret = id3v2_add_ucs2_lng(gfp, ID_GENRE, 0, text);
    if (ret == 0) {
        gfc->tag_spec.flags |= CHANGED_FLAG;
        gfc->tag_spec.genre_id3v1 = GENRE_INDEX_OTHER;
    }
    return ret;
}

/*
Some existing options for ID3 tag can be specified by --tv option
as follows.
--tt <value>, --tv TIT2=value
--ta <value>, --tv TPE1=value
--tl <value>, --tv TALB=value
--ty <value>, --tv TYER=value
--tn <value>, --tv TRCK=value
--tg <value>, --tv TCON=value
(although some are not exactly same)*/

/*! Attach cover art. */
/*!
  \ingroup api_tags
  Puts an image into the ID3v2 tag, and turns that tag on. The image is
  copied, so the caller can free its buffer afterwards.

  Note that **the format is detected from the data**, not given by the caller.
  LAME recognizes JPEG, PNG and GIF by their first bytes and rejects anything
  else. So a file with the right extension but the wrong content is rejected.
  The MIME type written into the tag must match the data.

  Cover art that was set before is freed first. So NULL, or a size of 0,
  removes the cover art.

  \param gfp    the encoder instance.
  \param image  the image bytes, or NULL to remove the art.
  \param size   the image size in bytes.
  \retval 0  success, or the art was removed. **0 also if memory allocation
             fails**; the output then has no cover art, without a message.
  \retval -1 the data is not a JPEG, PNG or GIF image.
*/
int
id3tag_set_albumart(lame_t gfp, const char *image, size_t size)
{
    int     mimetype = MIMETYPE_NONE;
    lame_internal_flags *gfc = 0;

    if (is_lame_internal_flags_null(gfp)) {
        return 0;
    }
    gfc = gfp->internal_flags;

    if (image != 0) {
        unsigned char const *data = (unsigned char const *) image;
        /* determine MIME type from the actual image data */
        if (2 < size && data[0] == 0xFF && data[1] == 0xD8) {
            mimetype = MIMETYPE_JPEG;
        }
        else if (4 < size && data[0] == 0x89 && strncmp((const char *) &data[1], "PNG", 3) == 0) {
            mimetype = MIMETYPE_PNG;
        }
        else if (4 < size && strncmp((const char *) data, "GIF8", 4) == 0) {
            mimetype = MIMETYPE_GIF;
        }
        else {
            return -1;
        }
    }
    if (gfc->tag_spec.albumart != 0) {
        free(gfc->tag_spec.albumart);
        gfc->tag_spec.albumart = 0;
        gfc->tag_spec.albumart_size = 0;
        gfc->tag_spec.albumart_mimetype = MIMETYPE_NONE;
    }
    if (size < 1 || mimetype == MIMETYPE_NONE) {
        return 0;
    }
    gfc->tag_spec.albumart = lame_calloc(unsigned char, size);
    if (gfc->tag_spec.albumart != 0) {
        memcpy(gfc->tag_spec.albumart, image, size);
        gfc->tag_spec.albumart_size = (unsigned int)size;
        gfc->tag_spec.albumart_mimetype = mimetype;
        gfc->tag_spec.flags |= CHANGED_FLAG;
        id3tag_add_v2(gfp);
    }
    return 0;
}

static unsigned char *
set_4_byte_value(unsigned char *bytes, uint32_t value)
{
    put_be32(bytes, value);
    return bytes + 4;
}

/**
 * \internal
 * \brief Writes a frame header's size field.
 *
 * ID3v2.3 stores the size as a plain 32-bit big-endian integer. ID3v2.4
 * stores it synchsafe: 28 bits, seven in each byte, with the top bit of each
 * byte clear. The size of the whole tag is limited to 28 bits before any
 * frame is written, so every frame size fits into both forms.
 *
 * \param bytes      where the four size bytes go.
 * \param size       the frame's size, header excluded.
 * \param synchsafe  nonzero for an ID3v2.4 tag.
 * \return the position after the size field.
 */
static unsigned char *
set_frame_size(unsigned char *bytes, uint32_t size, int synchsafe)
{
    if (synchsafe) {
        size = ((size & 0x0FE00000u) << 3) | ((size & 0x001FC000u) << 2)
            | ((size & 0x00003F80u) << 1) | (size & 0x0000007Fu);
    }
    return set_4_byte_value(bytes, size);
}

static uint32_t
toID3v2TagId(char const *s)
{
    unsigned int i, x = 0;
    if (s == 0) {
        return 0;
    }
    for (i = 0; i < 4 && s[i] != 0; ++i) {
        char const c = s[i];
        unsigned int const u = 0x0ff & c;
        x <<= 8;
        x |= u;
        if (c < 'A' || 'Z' < c) {
            if (c < '0' || '9' < c) {
                return 0;
            }
        }
    }
    if (i < 4) {
        return 0;
    }
    return x;
}

static uint32_t
toID3v2TagId_ucs2(unsigned short const *s)
{
    unsigned int i, x = 0;
    unsigned short bom = 0;
    if (s == 0) {
        return 0;
    }
    bom = s[0];
    if (hasUcs2ByteOrderMarker(bom)) {
        ++s;
    }
    for (i = 0; i < 4 && s[i] != 0; ++i) {
        unsigned short const c = toLittleEndian(bom, s[i]);
        if (c < 'A' || 'Z' < c) {
            if (c < '0' || '9' < c) {
                return 0;
            }
        }
        x <<= 8;
        x |= c;
    }
    if (i < 4) {
        return 0;
    }
    return x;
}

static int
isMultiFrame(uint32_t frame_id)
{
    switch (frame_id) {
    case ID_TXXX:
    case ID_WXXX:
    case ID_COMMENT:
    case ID_SYLT:
    case ID_APIC:
    case ID_GEOB:
    case ID_PCNT:
    case ID_AENC:
    case ID_LINK:
    case ID_ENCR:
    case ID_GRID:
    case ID_PRIV:
        return 1;
    }
    return 0;
}

#if 0
static int
isFullTextString(int frame_id)
{
    switch (frame_id) {
    case ID_VSLT:
    case ID_COMMENT:
        return 1;
    }
    return 0;
}
#endif

static FrameDataNode *
findNode(id3tag_spec const *tag, uint32_t frame_id, FrameDataNode const *last)
{
    FrameDataNode *node = last ? last->nxt : tag->v2_head;
    while (node != 0) {
        if (node->fid == frame_id) {
            return node;
        }
        node = node->nxt;
    }
    return 0;
}

static void
appendNode(id3tag_spec * tag, FrameDataNode * node)
{
    if (tag->v2_tail == 0 || tag->v2_head == 0) {
        tag->v2_head = node;
        tag->v2_tail = node;
    }
    else {
        tag->v2_tail->nxt = node;
        tag->v2_tail = node;
    }
}

static void
setLang(char *dst, char const *src)
{
    int     i;
    if (src == 0 || src[0] == 0) {
        dst[0] = 'e';
        dst[1] = 'n';
        dst[2] = 'g';
    }
    else {
        for (i = 0; i < 3 && src[i] != 0; ++i) {
            dst[i] = src[i];
        }
        for (; i < 3; ++i) {
            dst[i] = ' ';
        }
    }
}

/*  The <ctype.h> functions are defined for the values of an unsigned char and
    for EOF only.
 */
static char
toLowerChar(char c)
{
    return (char) tolower((unsigned char) c);
}

static char
toUpperChar(char c)
{
    return (char) toupper((unsigned char) c);
}

static int
isSameLang(char const *l1, char const *l2)
{
    char    d[3];
    int     i;
    setLang(d, l2);
    for (i = 0; i < 3; ++i) {
        char    a = toLowerChar(l1[i]);
        char    b = toLowerChar(d[i]);
        if (a < ' ')
            a = ' ';
        if (b < ' ')
            b = ' ';
        if (a != b) {
            return 0;
        }
    }
    return 1;
}

/*
 * A frame which carries a description is keyed by it, so two descriptions are
 * the same one only if they also end in the same place.  One which merely
 * begins the other names a different frame, and an absent description is the
 * empty one.  Both comparisons stop at the stored description's length, which
 * is held without its terminator, so the incoming one is asked for its own end
 * there.
 */
static int
isSameDescriptor(FrameDataNode const *node, char const *dsc)
{
    size_t  i;
    if (node->dsc.enc == ID3V2_ENC_UCS2 && node->dsc.dim > 0) {
        return 0;
    }
    if (dsc == 0) {
        return node->dsc.dim == 0;
    }
    for (i = 0; i < node->dsc.dim; ++i) {
        if (node->dsc.ptr.l[i] != dsc[i]) {
            return 0;
        }
    }
    return dsc[i] == 0;
}

static int
isSameDescriptorUcs2(FrameDataNode const *node, unsigned short const *dsc)
{
    size_t  i;
    if (node->dsc.enc != ID3V2_ENC_UCS2 && node->dsc.dim > 0) {
        return 0;
    }
    if (dsc == 0) {
        return node->dsc.dim == 0;
    }
    for (i = 0; i < node->dsc.dim; ++i) {
        if (node->dsc.ptr.u[i] != dsc[i]) {
            return 0;
        }
    }
    return dsc[i] == 0;
}

/**
 * \internal
 * \brief Returns the node for a frame, and appends a new node if the tag has
 *        none for it yet.
 *
 * A frame that a tag can hold several times (isMultiFrame()) is found by its
 * language and its description. Any other frame is found by its identifier.
 *
 * \param tag       the tag.
 * \param frame_id  the frame identifier.
 * \param lang      the language, as setLang() writes it.
 * \param enc       the encoding of \a desc.
 * \param desc      the description. NULL for none.
 * \return the node. NULL if a new node cannot be allocated.
 */
static FrameDataNode *
findOrAppendNode(id3tag_spec * tag, uint32_t frame_id, char const *lang, id3v2_text_encoding enc,
                 void const *desc)
{
    FrameDataNode *node = findNode(tag, frame_id, 0);
    if (isMultiFrame(frame_id)) {
        while (node) {
            if (isSameLang(node->lng, lang)) {
                int const same = (enc == ID3V2_ENC_UCS2)
                    ? isSameDescriptorUcs2(node, (unsigned short const *) desc)
                    : isSameDescriptor(node, (char const *) desc);
                if (same) {
                    break;
                }
            }
            node = findNode(tag, frame_id, node);
        }
    }
    if (node == 0) {
        node = lame_calloc(FrameDataNode, 1);
        if (node != 0) {
            appendNode(tag, node);
        }
    }
    return node;
}

static int
id3v2_add_ucs2(lame_t gfp, uint32_t frame_id, char const *lng, unsigned short const *desc, unsigned short const *text)
{
    lame_internal_flags *gfc = gfp != 0 ? gfp->internal_flags : 0;
    if (gfc != 0) {
        FrameDataNode *node;
        char lang[4];
        setLang(lang, lng);
        node = findOrAppendNode(&gfc->tag_spec, frame_id, lang, ID3V2_ENC_UCS2, desc);
        if (node == 0) {
            return ID3_RESULT_NO_MEMORY;
        }
        node->fid = frame_id;
        setLang(node->lng, lang);
        node->dsc.dim = local_ucs2_strdup(&node->dsc.ptr.u, desc);
        node->dsc.enc = ID3V2_ENC_UCS2;
        node->txt.dim = local_ucs2_strdup(&node->txt.ptr.u, text);
        node->txt.enc = ID3V2_ENC_UCS2;
        gfc->tag_spec.flags |= (CHANGED_FLAG | ADD_V2_FLAG);
        return 0;
    }
    return ID3_RESULT_NOT_WRITABLE;
}

/*
* internal helper, adds encoded text to node.
* For Latin-1 or UTF-8.
*/
static int
id3v2_add_enc(lame_t gfp, uint32_t frame_id, char const *lng, char const *desc, char const *text,
              id3v2_text_encoding enc)
{
    lame_internal_flags *gfc = gfp != 0 ? gfp->internal_flags : 0;
    if (gfc != 0) {
        FrameDataNode *node;
        char lang[4];
        setLang(lang, lng);
        node = findOrAppendNode(&gfc->tag_spec, frame_id, lang, enc, desc);
        if (node == 0) {
            return ID3_RESULT_NO_MEMORY;
        }
        node->fid = frame_id;
        setLang(node->lng, lang);
        node->dsc.dim = local_strdup(&node->dsc.ptr.l, desc);
        node->dsc.enc = enc;
        node->txt.dim = local_strdup(&node->txt.ptr.l, text);
        node->txt.enc = enc;
        gfc->tag_spec.flags |= (CHANGED_FLAG | ADD_V2_FLAG);
        return 0;
    }
    return ID3_RESULT_NOT_WRITABLE;
}

static int
id3v2_add_latin1(lame_t gfp, uint32_t frame_id, char const *lng, char const *desc, char const *text)
{
    return id3v2_add_enc(gfp, frame_id, lng, desc, text, ID3V2_ENC_LATIN1);
}

// function to add UTF-8-encoded text to node
static int
id3v2_add_utf8(lame_t gfp, uint32_t frame_id, char const *lng, char const *desc, char const *text)
{
    return id3v2_add_enc(gfp, frame_id, lng, desc, text, ID3V2_ENC_UTF8);
}

static int
id3v2_add_utf8_lng(lame_t gfp, uint32_t frame_id, char const *desc, char const *text)
{
    char const* lang = id3v2_get_language(gfp);
    return id3v2_add_utf8(gfp, frame_id, lang, desc, text);
}

static int
id3v2_add_ucs2_lng(lame_t gfp, uint32_t frame_id, unsigned short const *desc, unsigned short const *text)
{
    char const* lang = id3v2_get_language(gfp);
    return id3v2_add_ucs2(gfp, frame_id, lang, desc, text);
}

static int
id3v2_add_latin1_lng(lame_t gfp, uint32_t frame_id, char const *desc, char const *text)
{
    char const* lang = id3v2_get_language(gfp);
    return id3v2_add_latin1(gfp, frame_id, lang, desc, text);
}

/**
 * \internal
 * \brief Adds a frame from a "description=text" string in 8-bit text.
 * \param gfp         the encoder instance.
 * \param id          the frame identifier.
 * \param fieldvalue  the description, "=", and the text.
 * \param enc         the encoding, ::ID3V2_ENC_LATIN1 or ::ID3V2_ENC_UTF8.
 * \return 0 on success, -7 if there is no "=", -254 if memory runs out.
 */
static int
id3tag_set_userinfo_enc(lame_t gfp, uint32_t id, char const *fieldvalue, id3v2_text_encoding enc)
{
    char const separator = '=';
    int     rc = -7;
    int     a = local_char_pos(fieldvalue, separator);
    if (a >= 0) {
        char*   dup = 0;
        local_strdup(&dup, fieldvalue);
        if (dup == 0) {
            return ID3_RESULT_NO_MEMORY;
        }
        dup[a] = 0;
        rc = id3v2_add_enc(gfp, id, id3v2_get_language(gfp), dup, dup+a+1, enc);
        free(dup);
    }
    return rc;
}

static int
id3tag_set_userinfo_ucs2(lame_t gfp, uint32_t id, unsigned short const *fieldvalue)
{
    unsigned short const separator = fromLatin1Char(fieldvalue,'=');
    int     rc = -7;
    size_t  b = local_ucs2_strlen(fieldvalue);
    int     a = local_ucs2_pos(fieldvalue, separator);
    if (a >= 0) {
        unsigned short* dsc = 0, *val = 0;
        local_ucs2_substr(&dsc, fieldvalue, 0, a);
        local_ucs2_substr(&val, fieldvalue, a+1, b);
        rc = id3v2_add_ucs2_lng(gfp, id, dsc, val);
        free(dsc);
        free(val);
    }
    return rc;
}

/** \internal The kinds of frame that the text frame setters tell apart. */
typedef enum {
    TEXTINFO_REJECT,         /* not a frame that they write */
    TEXTINFO_USERINFO,       /* TXXX, WXXX, COMM: "description=text" */
    TEXTINFO_GENRE,          /* TCON: through the genre setter */
    TEXTINFO_TEXT,           /* the other T and W frames, and PCST: text only */
    TEXTINFO_DESCRIPTION     /* USER, WFED: the text is the description */
} textinfo_kind;

/**
 * \internal
 * \brief Returns how the id3tag_set_textinfo_*() functions write a frame.
 * \param frame_id  the frame identifier.
 * \return the kind of frame.
 */
static textinfo_kind
textinfoKind(uint32_t frame_id)
{
    if (frame_id == ID_TXXX || frame_id == ID_WXXX || frame_id == ID_COMMENT) {
        return TEXTINFO_USERINFO;
    }
    if (frame_id == ID_GENRE) {
        return TEXTINFO_GENRE;
    }
    if (frame_id == ID_PCST) {
        return TEXTINFO_TEXT;
    }
    if (frame_id == ID_USER || frame_id == ID_WFED) {
        return TEXTINFO_DESCRIPTION; /* iTunes expects WFED to be a text frame */
    }
    if (isFrameIdMatching(frame_id, FRAME_ID('T', 0, 0, 0))
      ||isFrameIdMatching(frame_id, FRAME_ID('W', 0, 0, 0))) {
        return TEXTINFO_TEXT;
    }
    return TEXTINFO_REJECT;
}

/*! Set an ID3v2 text frame, taking UTF-8 text. */
/*!
  \ingroup api_tags
  \c id3tag_set_textinfo_latin1() for UTF-8 input. Use it with
  \c id3tag_add_v2_4_UTF8() or \c id3tag_v2_4_UTF8_only(), because only an
  ID3v2.4 tag stores text as UTF-8.

  \param gfp   the encoder instance.
  \param id    the four-character ID3v2 frame identifier.
  \param text  the text, in UTF-8. NULL is accepted and does nothing.
  \return as \c id3tag_set_textinfo_latin1().
*/
int
id3tag_set_textinfo_utf8(lame_t gfp, char const *id, char const *text)
{
    uint32_t const frame_id = toID3v2TagId(id);
    if (frame_id == 0) {
        return -1;
    }
    if (is_lame_internal_flags_null(gfp)) {
        return 0;
    }
    if (text == 0) {
        return 0;
    }
    switch (textinfoKind(frame_id)) {
    case TEXTINFO_USERINFO:
        return id3tag_set_userinfo_enc(gfp, frame_id, text, ID3V2_ENC_UTF8);
    case TEXTINFO_GENRE:
        return id3tag_set_genre_utf8(gfp, text);
    case TEXTINFO_TEXT:
        return id3v2_add_utf8_lng(gfp, frame_id, 0, text);
    case TEXTINFO_DESCRIPTION:
        return id3v2_add_utf8_lng(gfp, frame_id, text, 0);
    default:
        return ID3_RESULT_NOT_WRITABLE;
    }
}

/*! Set an ID3v2 text frame, taking UTF-16 text. */
/*!
  \ingroup api_tags
  \c id3tag_set_textinfo_latin1() for UTF-16 input.

  The text **must begin with a byte order mark** (BOM). Without a BOM, the
  call fails and returns -3. UTF-16 text can be stored in either byte order.
  The BOM tells the reader of the tag which order is used.

  \param gfp   the encoder instance.
  \param id    the four-character ID3v2 frame identifier.
  \param text  the text, in UTF-16, beginning with a byte order mark. NULL is
               accepted and does nothing.
  \return as \c id3tag_set_textinfo_latin1(), and -3 if the byte order mark is
          missing.
*/
int
id3tag_set_textinfo_utf16(lame_t gfp, char const *id, unsigned short const *text)
{
    uint32_t const frame_id = toID3v2TagId(id);
    if (frame_id == 0) {
        return -1;
    }
    if (is_lame_internal_flags_null(gfp)) {
        return 0;
    }
    if (text == 0) {
        return 0;
    }
    if (!hasUcs2ByteOrderMarker(text[0])) {
        return -3;  /* BOM missing */
    }
    switch (textinfoKind(frame_id)) {
    case TEXTINFO_USERINFO:
        return id3tag_set_userinfo_ucs2(gfp, frame_id, text);
    case TEXTINFO_GENRE:
        return id3tag_set_genre_utf16(gfp, text);
    case TEXTINFO_TEXT:
        return id3v2_add_ucs2_lng(gfp, frame_id, 0, text);
    case TEXTINFO_DESCRIPTION:
        return id3v2_add_ucs2_lng(gfp, frame_id, text, 0);
    default:
        return ID3_RESULT_NOT_WRITABLE;
    }
}

/*! Set an ID3v2 text frame, taking UCS-2 text. */
/*!
  \ingroup api_tags
  \deprecated Use \c id3tag_set_textinfo_utf16(). This function only calls
  it. lame.h does not declare this function. The library still exports it, so
  that programs built against an older release still link.

  \param gfp   the encoder instance.
  \param id    the four-character ID3v2 frame identifier.
  \param text  the text, beginning with a byte order mark.
  \return as \c id3tag_set_textinfo_utf16().
*/
int
id3tag_set_textinfo_ucs2(lame_t gfp, char const *id, unsigned short const *text)
{
    return id3tag_set_textinfo_utf16(gfp, id, text);
}

/*! Set an ID3v2 text frame, taking Latin-1 text. */
/*!
  \ingroup api_tags
  The same as \c id3tag_set_fieldvalue(), with the frame identifier and the
  text as separate arguments instead of one "ID=text" string.

  It accepts text and URL frames, whose identifier begins with T or W, and a
  few frames with a special layout. It rejects all other frames. For some
  identifiers it calls the setter that knows the layout of the frame. For
  example, \c "TCON" here is the same as \c id3tag_set_genre().

  \param gfp   the encoder instance.
  \param id    the four-character ID3v2 frame identifier.
  \param text  the text, in Latin-1. NULL is accepted and does nothing.
  \retval 0    success.
  \retval -1   \a id is not a valid identifier.
  \retval -255 this function cannot write the frame.
  When the call goes to another setter, it returns the result of that setter.
*/
int
id3tag_set_textinfo_latin1(lame_t gfp, char const *id, char const *text)
{
    uint32_t const frame_id = toID3v2TagId(id);
    if (frame_id == 0) {
        return -1;
    }
    if (is_lame_internal_flags_null(gfp)) {
        return 0;
    }
    if (text == 0) {
        return 0;
    }
    switch (textinfoKind(frame_id)) {
    case TEXTINFO_USERINFO:
        return id3tag_set_userinfo_enc(gfp, frame_id, text, ID3V2_ENC_LATIN1);
    case TEXTINFO_GENRE:
        return id3tag_set_genre(gfp, text);
    case TEXTINFO_TEXT:
        return id3v2_add_latin1_lng(gfp, frame_id, 0, text);
    case TEXTINFO_DESCRIPTION:
        return id3v2_add_latin1_lng(gfp, frame_id, text, 0);
    default:
        return ID3_RESULT_NOT_WRITABLE;
    }
}


/*! Add a comment frame, taking Latin-1 text. */
/*!
  \ingroup api_tags
  The full form of \c id3tag_set_comment(). An ID3v2 comment frame has a
  language and a short description in addition to the text. So a file can
  have several comments, for example notes in two languages. The language and
  the description identify the frame, so a second call with the same language
  and description replaces the first.

  \param gfp   the encoder instance.
  \param lang  a three-letter ISO 639-2 language code. NULL or empty becomes
               "eng". A shorter string is filled with spaces, and a longer one
               is cut. Neither is reported.
  \param desc  the description that names this comment. It can be empty, as
               for the simple setter.
  \param text  the comment, in Latin-1.
  \return 0 on success, non-zero on failure.
*/
int
id3tag_set_comment_latin1(lame_t gfp, char const *lang, char const *desc, char const *text)
{
    if (is_lame_internal_flags_null(gfp)) {
        return 0;
    }
    return id3v2_add_latin1(gfp, ID_COMMENT, lang, desc, text);
}

/*! Add a comment frame, taking UTF-8 text. */
/*!
  \ingroup api_tags
  \c id3tag_set_comment_latin1() for UTF-8 input. Use it with an ID3v2.4 tag.

  \param gfp   the encoder instance.
  \param lang  a three-letter ISO 639-2 language code.
  \param desc  the description that names this comment.
  \param text  the comment, in UTF-8.
  \return 0 on success, non-zero on failure.
*/
int
id3tag_set_comment_utf8(lame_t gfp, char const *lang, char const *desc, char const *text)
{
    if (is_lame_internal_flags_null(gfp)) {
        return 0;
    }
    return id3v2_add_utf8(gfp, ID_COMMENT, lang, desc, text);
}


/*! Add a comment frame, taking UTF-16 text. */
/*!
  \ingroup api_tags
  \c id3tag_set_comment_latin1() for UTF-16 input. Both \a desc and \a text
  must begin with a byte order mark, as for \c id3tag_set_textinfo_utf16().

  \param gfp   the encoder instance.
  \param lang  a three-letter ISO 639-2 language code.
  \param desc  the description, in UTF-16 with a byte order mark.
  \param text  the comment, in UTF-16 with a byte order mark.
  \return 0 on success, non-zero on failure.
*/
int
id3tag_set_comment_utf16(lame_t gfp, char const *lang, unsigned short const *desc, unsigned short const *text)
{
    if (is_lame_internal_flags_null(gfp)) {
        return 0;
    }
    return id3v2_add_ucs2(gfp, ID_COMMENT, lang, desc, text);
}


/*! Add a comment frame, taking UCS-2 text. */
/*!
  \ingroup api_tags
  \deprecated Use \c id3tag_set_comment_utf16(). This function only calls
  it.

  \param gfp   the encoder instance.
  \param lang  a three-letter ISO 639-2 language code.
  \param desc  the description, with a byte order mark.
  \param text  the comment, with a byte order mark.
  \return as \c id3tag_set_comment_utf16().
*/
int
id3tag_set_comment_ucs2(lame_t gfp, char const *lang, unsigned short const *desc, unsigned short const *text)
{
    if (is_lame_internal_flags_null(gfp)) {
        return 0;
    }
    return id3tag_set_comment_utf16(gfp, lang, desc, text);
}


/*! Set the track title. */
/*!
  \ingroup api_tags
  Written to both tags: to the ID3v1 title field, where it is cut to 30
  characters, and in full to the ID3v2 frame. If the text needs an ID3v2 tag,
  this turns the tag on.

  A NULL or empty \a title is ignored. It does not clear a title that was set
  before. Use \c id3tag_init() for that.

  \param gfp    the encoder instance.
  \param title  the title, in Latin-1 or, with a UTF-8 tag selected, in UTF-8.
*/
void
id3tag_set_title(lame_t gfp, const char *title)
{
    lame_internal_flags *gfc = gfp != 0 ? gfp->internal_flags : 0;
    if (gfc && title && *title) {
        local_strdup(&gfc->tag_spec.title, title);
        gfc->tag_spec.flags |= CHANGED_FLAG;
        copyV1ToV2(gfp, ID_TITLE, title);
    }
}

/*! Set the artist. */
/*!
  \ingroup api_tags
  Written to both tags. The ID3v1 field has room for 30 characters, and the
  rest is cut. A NULL or empty \a artist is ignored.

  \param gfp     the encoder instance.
  \param artist  the artist name.
*/
void
id3tag_set_artist(lame_t gfp, const char *artist)
{
    lame_internal_flags *gfc = gfp != 0 ? gfp->internal_flags : 0;
    if (gfc && artist && *artist) {
        local_strdup(&gfc->tag_spec.artist, artist);
        gfc->tag_spec.flags |= CHANGED_FLAG;
        copyV1ToV2(gfp, ID_ARTIST, artist);
    }
}

/*! Set the album. */
/*!
  \ingroup api_tags
  Written to both tags. The ID3v1 field has room for 30 characters, and the
  rest is cut. A NULL or empty \a album is ignored.

  \param gfp    the encoder instance.
  \param album  the album name.
*/
void
id3tag_set_album(lame_t gfp, const char *album)
{
    lame_internal_flags *gfc = gfp != 0 ? gfp->internal_flags : 0;
    if (gfc && album && *album) {
        local_strdup(&gfc->tag_spec.album, album);
        gfc->tag_spec.flags |= CHANGED_FLAG;
        copyV1ToV2(gfp, ID_ALBUM, album);
    }
}

/*! Set the year. */
/*!
  \ingroup api_tags
  Takes text, but the ID3v1 field has four digits. So LAME reads the text as a
  number, limits it to 0 to 9999, and writes that to ID3v1. The ID3v2 frame
  gets the text as given, so a full date is kept there.

  A value that reads as 0, also text that is not a number, leaves the ID3v1
  year empty. The ID3v2 frame is still written. Nothing reports this.

  \param gfp   the encoder instance.
  \param year  the year as text.
*/
void
id3tag_set_year(lame_t gfp, const char *year)
{
    lame_internal_flags *gfc = gfp != 0 ? gfp->internal_flags : 0;
    if (gfc && year && *year) {
        long    number = strtol(year, NULL, 10);
        int     num;
        if (number < 0) {
            number = 0;
        }
        /* limit a year to 4 digits so it fits in a version 1 tag */
        if (number > 9999) {
            number = 9999;
        }
        num = (int) number;
        if (num) {
            gfc->tag_spec.year = num;
            gfc->tag_spec.flags |= CHANGED_FLAG;
        }
        copyV1ToV2(gfp, ID_YEAR, year);
    }
}

/*! Set the comment. */
/*!
  \ingroup api_tags
  Written to both tags. The ID3v1 comment field has room for 30 characters,
  **or 28 if a track number is set**. In ID3v1.1, the last two bytes of the
  field store the track number. So setting a track can shorten a comment that
  was already set.

  The ID3v2 frame uses the encoding of the selected tag version.

  \param gfp      the encoder instance.
  \param comment  the comment text.
*/
void
id3tag_set_comment(lame_t gfp, const char *comment)
{
    lame_internal_flags *gfc = gfp != 0 ? gfp->internal_flags : 0;
    if (gfc && comment && *comment) {
        local_strdup(&gfc->tag_spec.comment, comment);
        gfc->tag_spec.flags |= CHANGED_FLAG;
        {
            uint32_t const flags = gfc->tag_spec.flags;
            
            // add V2 comment tag: either ID3v2.3 or ID3v2.4
            // according to whether V2_4_UTF8_FLAG is used
            
            // if we are writing UTF-8 ID3v2.4 tag
            if (test_tag_spec_flags(gfc, V2_4_UTF8_FLAG)) {
                char const* lang = id3v2_get_language(gfp);
                id3v2_add_utf8(gfp, ID_COMMENT, lang, "", comment);
            } else { // if we are writing latin1 ID3v2.3 tag
                id3v2_add_latin1_lng(gfp, ID_COMMENT, "", comment);
            }
            
            gfc->tag_spec.flags = flags;
        }
    }
}

/*! Set the track number. */
/*!
  \ingroup api_tags
  Accepts a plain number or the form <tt>number/total</tt>. ID3v1 has room for
  one byte, so only 1 to 255 fits there, and only the number before the
  slash. The ID3v2 frame gets the text as given, with the total.

  A number outside that range, or a total, is not an error. LAME writes it to
  ID3v2 and turns the ID3v2 tag on for it. The return value **-1 only says
  that ID3v1 could not hold it**. The call still stored what it could.

  \param gfp    the encoder instance.
  \param track  the track number, optionally followed by "/" and the total.
  \retval 0  the number also fits into ID3v1.
  \retval -1 the number does not fit into ID3v1.
*/
int
id3tag_set_track(lame_t gfp, const char *track)
{
    char const *trackcount;
    lame_internal_flags *gfc = gfp != 0 ? gfp->internal_flags : 0;
    int     ret = 0;

    if (gfc && track && *track) {
        long const number = strtol(track, NULL, 10);
        /* check for valid ID3v1 track number range */
        int     num = (number >= 1 && number <= 255) ? (int) number : 0;
        if (num == 0) {
            ret = -1;   /* track number out of ID3v1 range, ignored for ID3v1 */
            gfc->tag_spec.flags |= (CHANGED_FLAG | ADD_V2_FLAG);
        }
        if (num) {
            gfc->tag_spec.track_id3v1 = num;
            gfc->tag_spec.flags |= CHANGED_FLAG;
        }
        /* Look for the total track count after a "/", same restrictions */
        trackcount = strchr(track, '/');
        if (trackcount && *trackcount) {
            gfc->tag_spec.flags |= (CHANGED_FLAG | ADD_V2_FLAG);
        }
        copyV1ToV2(gfp, ID_TRACK, track);
    }
    return ret;
}

/* would use real "strcasecmp" but it isn't portable */
static int
local_strcasecmp(const char *s1, const char *s2)
{
    unsigned char c1;
    unsigned char c2;
    do {
        c1 = (unsigned char) toLowerChar(*s1);
        c2 = (unsigned char) toLowerChar(*s2);
        if (!c1) {
            break;
        }
        ++s1;
        ++s2;
    } while (c1 == c2);
    return c1 - c2;
}


static 
const char* nextUpperAlpha(const char* p, char x)
{
    char c;
    for (c = toUpperChar(*p); *p != 0; c = toUpperChar(*++p)) {
        if ('A' <= c && c <= 'Z') {
            if (c != x) {
                return p;
            }
        }
    }
    return p;
}


static int
sloppyCompared(const char* p, const char* q)
{
    char cp, cq;
    p = nextUpperAlpha(p, 0);
    q = nextUpperAlpha(q, 0);
    cp = toUpperChar(*p);
    cq = toUpperChar(*q);
    while (cp == cq) {
        if (cp == 0) {
            return 1;
        }
        if (p[1] == '.') { /* some abbrevation */
            while (*q && *q++ != ' ') {
            }
        }
        p = nextUpperAlpha(p, cp);
        q = nextUpperAlpha(q, cq);
        cp = toUpperChar(*p);
        cq = toUpperChar(*q);
    }
    return 0;
}


static int 
sloppySearchGenre(const char *genre)
{
    int i;
    for (i = 0; i < GENRE_NAME_COUNT; ++i) {
        if (sloppyCompared(genre, genre_names[i])) {
            return i;
        }
    }
    return GENRE_NAME_COUNT;
}


static int
searchGenre(const char* genre)
{
    int i;
    for (i = 0; i < GENRE_NAME_COUNT; ++i) {
        if (!local_strcasecmp(genre, genre_names[i])) {
            return i;
        }
    }
    return GENRE_NAME_COUNT;
}


/*! Set the genre. */
/*!
  \ingroup api_tags
  Accepts an ID3v1 genre number or a genre name. LAME looks a name up in the
  ID3v1 list, first exactly, then loosely, so that "R & B" finds "R&B". If it
  matches, **LAME stores the spelling from the list**, not the caller's. A
  name that matches nothing is kept as free text in the ID3v2 frame, and
  ID3v1 gets "Other".

  \c id3tag_genre_list() lists the names that this function accepts.

  \param gfp    the encoder instance.
  \param genre  a genre number or name.
  \retval 0  success. A name that is not in the list is **not** an error.
  \retval -1 \a genre is a number outside the ID3v1 list.
*/
int
id3tag_set_genre(lame_t gfp, const char *genre)
{
    lame_internal_flags *gfc = gfp != 0 ? gfp->internal_flags : 0;
    int     ret = 0;
    if (gfc && genre && *genre) {
        int const num = lookupGenre(genre);
        if (num == -1) return num;
        gfc->tag_spec.flags |= CHANGED_FLAG;
        if (num >= 0) {
            gfc->tag_spec.genre_id3v1 = num;
            genre = genre_names[num];
        }
        else {
            gfc->tag_spec.genre_id3v1 = GENRE_INDEX_OTHER;
            gfc->tag_spec.flags |= ADD_V2_FLAG;
        }
        copyV1ToV2(gfp, ID_GENRE, genre);
    }
    return ret;
}


/**
 * \internal
 * \brief Returns the size of a string in a frame.
 * \param s           the string.
 * \param terminated  non-zero to count its terminator: 1 byte, or 2 for
 *                    UCS-2 text.
 * \return the size in bytes.
 */
static size_t
sizeOfString(FrameString const *s, int terminated)
{
    size_t const unit = (s->enc == ID3V2_ENC_UCS2) ? 2 : 1;
    return (s->dim + (terminated ? 1 : 0)) * unit;
}

/**
 * \internal
 * \brief The layouts of the frames LAME writes from a FrameDataNode.
 */
enum frame_kind {
    FRAME_KIND_TEXT,    /**< a text frame */
    FRAME_KIND_COMMENT, /**< COMM or USER: with a language */
    FRAME_KIND_URL      /**< a W... frame: the URL in Latin-1 */
};

/**
 * \internal
 * \brief Returns the layout of a frame, for the size and the writing alike.
 *
 * \param node  the frame.
 * \return its layout.
 */
static enum frame_kind
frame_kind(FrameDataNode const *node)
{
    if (node->fid == ID_COMMENT || node->fid == ID_USER) {
        return FRAME_KIND_COMMENT;
    }
    if (isFrameIdMatching(node->fid, FRAME_ID('W',0,0,0))) {
        return FRAME_KIND_URL;
    }
    return FRAME_KIND_TEXT;
}

static  size_t
sizeOfNode(FrameDataNode const *node)
{
    size_t  n = 0;
    if (node) {
        n = 10;         /* header size */
        n += 1;         /* text encoding flag */
        if (node->dsc.dim > 0) {
            n += sizeOfString(&node->dsc, 1);
        }
        n += sizeOfString(&node->txt, 0);
    }
    return n;
}

static  size_t
sizeOfCommentNode(FrameDataNode const *node)
{
    size_t  n = 0;
    if (node) {
        n = 10;         /* header size */
        n += 1;         /* text encoding flag */
        n += 3;         /* language */
        n += sizeOfString(&node->dsc, 1);
        n += sizeOfString(&node->txt, 0);
    }
    return n;
}

static size_t
sizeOfWxxxNode(FrameDataNode const *node)
{
    size_t  n = 0;
    if (node) {
        n = 10;         /* header size */
        if (node->dsc.dim > 0) {
            n += 1;         /* text encoding flag */
            n += sizeOfString(&node->dsc, 1);
        }
        if (node->txt.dim > 0) {
            switch (node->txt.enc) {
            default:
            case ID3V2_ENC_LATIN1:
                n += node->txt.dim;
                break;
            case ID3V2_ENC_UCS2:
                /* the setters keep UCS-2 text with its BOM, which
                   writeLoBytes() drops */
                assert(hasUcs2ByteOrderMarker(node->txt.ptr.u[0]));
                n += node->txt.dim - 1; /* UCS2 -> Latin1, skip BOM */
                break;
            }
        }
        if (n == 10) {
            n = 0;              /* nothing to carry: set_frame_wxxx() writes no frame */
        }
    }
    return n;
}

static unsigned char *
writeChars(unsigned char *frame, char const *str, size_t n)
{
    while (n--) {
        *frame++ = *str++;
    }
    return frame;
}

static unsigned char *
writeUcs2s(unsigned char *frame, unsigned short const *str, size_t n)
{
    if (n > 0) {
        unsigned short const bom = *str;
        while (n--) {
            unsigned short const c = toLittleEndian(bom, *str++);
            *frame++ = 0x00ffu & c;
            *frame++ = 0x00ffu & (c >> 8);
        }
    }
    return frame;
}

static unsigned char *
writeLoBytes(unsigned char *frame, unsigned short const *str, size_t n)
{
    if (n > 0) {
        unsigned short const bom = *str;
        if (hasUcs2ByteOrderMarker(bom)) {
            str++; n--; /* skip BOM */
        }
        while (n--) {
            unsigned short const c = toLittleEndian(bom, *str++);
            if (c < 0x0020u || 0x00ffu < c) {
                *frame++ = 0x0020; /* blank */
            }
            else {
                *frame++ = c;
            }
        }
    }
    return frame;
}

/**
 * \internal
 * \brief Writes a frame header: the identifier, the size and two clear flag
 *        bytes.
 * \param frame      where the header goes.
 * \param fid        the frame identifier.
 * \param size       the size of the frame, header excluded.
 * \param synchsafe  nonzero for an ID3v2.4 tag.
 * \return the position after the header.
 */
static unsigned char *
writeFrameHeader(unsigned char *frame, uint32_t fid, size_t size, int synchsafe)
{
    frame = set_4_byte_value(frame, fid);
    frame = set_frame_size(frame, (uint32_t) size, synchsafe);
    /* clear 2-byte header flags */
    *frame++ = 0;
    *frame++ = 0;
    return frame;
}

/**
 * \internal
 * \brief Writes a string of a frame, in its own encoding.
 * \param frame       where the string goes.
 * \param s           the string.
 * \param terminated  non-zero to write its terminator: a zero byte, or two
 *                    for UCS-2 text.
 * \return the position after the string.
 */
static unsigned char *
writeString(unsigned char *frame, FrameString const *s, int terminated)
{
    if (s->enc != ID3V2_ENC_UCS2) {
        frame = writeChars(frame, s->ptr.l, s->dim);
        if (terminated) {
            *frame++ = 0;
        }
    }
    else {
        frame = writeUcs2s(frame, s->ptr.u, s->dim);
        if (terminated) {
            *frame++ = 0;
            *frame++ = 0;
        }
    }
    return frame;
}

static unsigned char *
set_frame_comment(unsigned char *frame, FrameDataNode const *node, int synchsafe)
{
    size_t const n = sizeOfCommentNode(node);
    if (n > 10) {
        frame = writeFrameHeader(frame, node->fid, n - 10, synchsafe);
        /* use the specified encoding descriptor byte */
        *frame++ = node->txt.enc;
        /* 3 bytes language */
        *frame++ = node->lng[0];
        *frame++ = node->lng[1];
        *frame++ = node->lng[2];
        /* descriptor with zero byte(s) separator */
        frame = writeString(frame, &node->dsc, 1);
        /* comment full text */
        frame = writeString(frame, &node->txt, 0);
    }
    return frame;
}

static unsigned char *
set_frame_custom2(unsigned char *frame, FrameDataNode const *node, int synchsafe)
{
    size_t const n = sizeOfNode(node);
    if (n > 10) {
        frame = writeFrameHeader(frame, node->fid, n - 10, synchsafe);
        /* use the specified encoding descriptor byte */
        *frame++ = node->txt.enc;
        if (node->dsc.dim > 0) {
            frame = writeString(frame, &node->dsc, 1);
        }
        frame = writeString(frame, &node->txt, 0);
    }
    return frame;
}

static unsigned char *
set_frame_wxxx(unsigned char *frame, FrameDataNode const *node, int synchsafe)
{
    size_t const n = sizeOfWxxxNode(node);
    if (n > 10) {
        frame = writeFrameHeader(frame, node->fid, n - 10, synchsafe);
        if (node->dsc.dim > 0) {
            /* use the specified encoding descriptor byte */
            *frame++ = node->dsc.enc;
            frame = writeString(frame, &node->dsc, 1);
        }
        if (node->txt.enc != ID3V2_ENC_UCS2) {
            frame = writeChars(frame, node->txt.ptr.l, node->txt.dim);
        }
        else {
            frame = writeLoBytes(frame, node->txt.ptr.u, node->txt.dim);
        }
    }
    return frame;
}

static unsigned char *
set_frame_apic(unsigned char *frame, const char *mimetype, const unsigned char *data, size_t size,
               int synchsafe)
{
    /* ID3v2.3 standard APIC frame:
     *     <Header for 'Attached picture', ID: "APIC">
     *     Text encoding    $xx
     *     MIME type        <text string> $00
     *     Picture type     $xx
     *     Description      <text string according to encoding> $00 (00)
     *     Picture data     <binary data>
     */
    if (mimetype && data && size) {
        frame = writeFrameHeader(frame, FRAME_ID('A', 'P', 'I', 'C'), 4 + strlen(mimetype) + size, synchsafe);
        /* clear 1 encoding descriptor byte to indicate ISO-8859-1 format */
        *frame++ = 0;
        /* copy mime_type */
        while (*mimetype) {
            *frame++ = *mimetype++;
        }
        *frame++ = 0;
        /* set picture type to 0 */
        *frame++ = 0;
        /* empty description field */
        *frame++ = 0;
        /* copy the image data */
        while (size--) {
            *frame++ = *data++;
        }
    }
    return frame;
}

/*! Set an arbitrary ID3v2 text frame. */
/*!
  \ingroup api_tags
  Sets an ID3v2 frame that has no setter of its own. The argument is one
  string such as <tt>"TPE2=Various Artists"</tt>: a four-character ID3v2 frame
  identifier, an equals sign, and the text. It writes the frames that
  \c id3tag_set_textinfo_latin1() writes.

  The text is Latin-1. The \c _utf8, \c _utf16 and \c _ucs2 variants take the
  other encodings.

  \param gfp         the encoder instance.
  \param fieldvalue  the frame identifier, "=", and the text. NULL or empty is
                     accepted and does nothing.
  \return 0 on success. -1 if \a fieldvalue is too short to hold an identifier,
          or has no "=" in the fifth position. Otherwise the result of
          \c id3tag_set_textinfo_latin1().
*/
int
id3tag_set_fieldvalue(lame_t gfp, const char *fieldvalue)
{
    if (is_lame_internal_flags_null(gfp)) {
        return 0;
    }
    if (fieldvalue && *fieldvalue) {
        if (strlen(fieldvalue) < 5 || fieldvalue[4] != '=') {
            return -1;
        }
        return id3tag_set_textinfo_latin1(gfp, fieldvalue, &fieldvalue[5]);
    }
    return 0;
}

/*! Set an arbitrary ID3v2 text frame, taking UTF-16. */
/*!
  \ingroup api_tags
  \c id3tag_set_fieldvalue() for UTF-16 input. The whole string
  <tt>"TPE2=Various Artists"</tt>, with the identifier, is UTF-16. A byte
  order mark at the start is allowed and skipped.

  \param gfp         the encoder instance.
  \param fieldvalue  the frame identifier, "=", and the text, in UTF-16.
  \return 0 on success. -1 if the string is too short, has no "=" after the
          identifier, or names an identifier that is not valid. Unlike the
          Latin-1 version, a NULL or empty argument returns -1. Otherwise the
          result of \c id3tag_set_textinfo_utf16(). 0 if the instance is not
          usable.
*/
int
id3tag_set_fieldvalue_utf16(lame_t gfp, const unsigned short *fieldvalue)
{
    if (is_lame_internal_flags_null(gfp)) {
        return 0;
    }
    if (fieldvalue && *fieldvalue) {
        size_t dx = hasUcs2ByteOrderMarker(fieldvalue[0]);
        unsigned short const separator = fromLatin1Char(fieldvalue, '=');
        char fid[5] = {0,0,0,0,0};
        uint32_t const frame_id = toID3v2TagId_ucs2(fieldvalue);
        if (local_ucs2_strlen(fieldvalue) < (5+dx) || fieldvalue[4+dx] != separator) {
            return -1;
        }
        put_be32((unsigned char *) fid, frame_id);
        if (frame_id != 0) {
            unsigned short* txt = 0;
            int     rc;
            local_ucs2_substr(&txt, fieldvalue, dx+5, local_ucs2_strlen(fieldvalue));
            rc = id3tag_set_textinfo_utf16(gfp, fid, txt);
            free(txt);
            return rc;
        }
    }
    return -1;
}

/*! Set an arbitrary ID3v2 text frame, taking UCS-2. */
/*!
  \ingroup api_tags
  \deprecated Use \c id3tag_set_fieldvalue_utf16(). This function only calls
  it.

  \param gfp         the encoder instance.
  \param fieldvalue  the frame identifier, "=", and the text.
  \return as \c id3tag_set_fieldvalue_utf16().
*/
int
id3tag_set_fieldvalue_ucs2(lame_t gfp, const unsigned short *fieldvalue)
{
    if (is_lame_internal_flags_null(gfp)) {
        return 0;
    }
    return id3tag_set_fieldvalue_utf16(gfp, fieldvalue);
}

/*! Set an arbitrary ID3v2 text frame, taking UTF-8. */
/*!
  \ingroup api_tags
  \c id3tag_set_fieldvalue() for UTF-8 input. The frame identifier is ASCII in
  both cases, so the string is read the same way. Only the text differs.

  \param gfp         the encoder instance.
  \param fieldvalue  the frame identifier, "=", and the text, in UTF-8. NULL
                     or empty is accepted and does nothing.
  \return 0 on success. -1 if the string is too short or has no "=" in the
          fifth position. Otherwise the result of
          \c id3tag_set_textinfo_utf8().
*/
int
id3tag_set_fieldvalue_utf8(lame_t gfp, const char *fieldvalue)
{
    if (is_lame_internal_flags_null(gfp)) {
        return 0;
    }
    if (fieldvalue && *fieldvalue) {
        if (strlen(fieldvalue) < 5 || fieldvalue[4] != '=') {
            return -1;
        }
        return id3tag_set_textinfo_utf8(gfp, fieldvalue, &fieldvalue[5]);
    }
    return 0;
}

/*! Copy the ID3v2 tag into a buffer. */
/*!
  \ingroup api_tags
  Builds the tag as it would be written to the file. A caller that turned off
  the automatic tag writing with \c lame_set_write_id3tag_automatic() uses it
  to get the bytes and write them itself. Call it after
  \c lame_init_params().

  Call it once with a buffer that is too small, for example with a size of 0,
  to get the size. Then call it again with a buffer of that size. **Only a
  comparison with \a size tells the two cases apart.** A result larger than
  \a size means that nothing was copied, and the result is the required size.

  \param gfp     the encoder instance.
  \param buffer  where to copy the tag.
  \param size    how large \a buffer is.
  \return the number of bytes copied, or the number required if \a buffer is
          too small. 0 if there is no ID3v2 tag to write, because the caller
          asked for ID3v1 only or set nothing.
*/
size_t
lame_get_id3v2_tag(lame_t gfp, unsigned char *buffer, size_t size)
{
    lame_internal_flags *gfc = 0;

    if (is_lame_internal_flags_null(gfp)) {
        return 0;
    }
    gfc = gfp->internal_flags;
    if (test_tag_spec_flags(gfc, V1_ONLY_FLAG)) {
        return 0;
    }
    {
        int usev2 = test_tag_spec_flags(gfc, ADD_V2_FLAG | V2_ONLY_FLAG);
        /* calculate length of four fields which may not fit in verion 1 tag */
        size_t  title_length = gfc->tag_spec.title ? strlen(gfc->tag_spec.title) : 0;
        size_t  artist_length = gfc->tag_spec.artist ? strlen(gfc->tag_spec.artist) : 0;
        size_t  album_length = gfc->tag_spec.album ? strlen(gfc->tag_spec.album) : 0;
        size_t  comment_length = gfc->tag_spec.comment ? strlen(gfc->tag_spec.comment) : 0;
        /* write tag if explicitly requested or if fields overflow */
        if ((title_length > 30)
            || (artist_length > 30)
            || (album_length > 30)
            || (comment_length > 30)
            || (gfc->tag_spec.track_id3v1 && (comment_length > 28))) {
            usev2 = 1;
        }
        if (usev2) {
            size_t  tag_size;
            unsigned char *p;
            size_t  adjusted_tag_size;
            const char *albumart_mime = NULL;
            int const synchsafe = test_tag_spec_flags(gfc, V2_4_UTF8_FLAG);
            static const char *mime_jpeg = "image/jpeg";
            static const char *mime_png = "image/png";
            static const char *mime_gif = "image/gif";

            if (gfp->num_samples != MAX_U_32_NUM) {
                id3v2AddAudioDuration(gfp, gfp->num_samples);
            }

            /* calulate size of tag starting with 10-byte tag header */
            tag_size = 10;
            if (gfc->tag_spec.albumart && gfc->tag_spec.albumart_size) {
                switch (gfc->tag_spec.albumart_mimetype) {
                case MIMETYPE_JPEG:
                    albumart_mime = mime_jpeg;
                    break;
                case MIMETYPE_PNG:
                    albumart_mime = mime_png;
                    break;
                case MIMETYPE_GIF:
                    albumart_mime = mime_gif;
                    break;
                }
                if (albumart_mime) {
                    tag_size += 10 + 4 + strlen(albumart_mime) + gfc->tag_spec.albumart_size;
                }
            }
            {
                id3tag_spec *tag = &gfc->tag_spec;
                if (tag->v2_head != 0) {
                    FrameDataNode *node;
                    for (node = tag->v2_head; node != 0; node = node->nxt) {
                        switch (frame_kind(node)) {
                        case FRAME_KIND_COMMENT:
                            tag_size += sizeOfCommentNode(node);
                            break;
                        case FRAME_KIND_URL:
                            tag_size += sizeOfWxxxNode(node);
                            break;
                        case FRAME_KIND_TEXT:
                        default:
                            tag_size += sizeOfNode(node);
                            break;
                        }
                    }
                }
            }
            if (test_tag_spec_flags(gfc, PAD_V2_FLAG)) {
                /* add some bytes of padding */
                tag_size += gfc->tag_spec.padding_size;
            }
            /* The tag length goes into a 28-bit synchsafe field. A tag larger
             * than that cannot state its own size, so the four length bytes
             * would wrap and describe a shorter tag than was written, leaving
             * the frames after it unparseable. Produce no tag rather than a
             * corrupt one.
             */
            if (tag_size - 10 > 0x0FFFFFFFuL) {
                return 0;
            }
            if (size < tag_size) {
                return tag_size;
            }
            if (buffer == 0) {
                return 0;
            }
            p = buffer;
            /* set tag header starting with file identifier */
            *p++ = 'I';
            *p++ = 'D';
            *p++ = '3';
            
            /* set version number word */
            // set it according to whether V2_4_UTF8_FLAG is used
            if (test_tag_spec_flags(gfc, V2_4_UTF8_FLAG)) // if using ID3v2.4.0
            {
                *p++ = 4;
                *p++ = 0;
            } else { // if using ID3v2.3.0
                *p++ = 3;
                *p++ = 0;
            }

            /* clear flags byte */
            *p++ = 0;
            /* calculate and set tag size = total size - header size */
            adjusted_tag_size = tag_size - 10;
            p = set_frame_size(p, (uint32_t) adjusted_tag_size, 1);

            /*
             * NOTE: The remainder of the tag (frames and padding, if any)
             * are not "unsynchronized" to prevent false MPEG audio headers
             * from appearing in the bitstream.  Why?  Well, most players
             * and utilities know how to skip the ID3 version 2 tag by now
             * even if they don't read its contents, and it's actually
             * very unlikely that such a false "sync" pattern would occur
             * in just the simple text frames added here.
             */

            /* set each frame in tag */
            {
                id3tag_spec *tag = &gfc->tag_spec;
                if (tag->v2_head != 0) {
                    FrameDataNode *node;
                    for (node = tag->v2_head; node != 0; node = node->nxt) {
                        switch (frame_kind(node)) {
                        case FRAME_KIND_COMMENT:
                            p = set_frame_comment(p, node, synchsafe);
                            break;
                        case FRAME_KIND_URL:
                            p = set_frame_wxxx(p, node, synchsafe);
                            break;
                        case FRAME_KIND_TEXT:
                        default:
                            p = set_frame_custom2(p, node, synchsafe);
                            break;
                        }
                    }
                }
            }
            if (albumart_mime) {
                p = set_frame_apic(p, albumart_mime, gfc->tag_spec.albumart,
                                   gfc->tag_spec.albumart_size, synchsafe);
            }
            /* clear any padding bytes */
            memset(p, 0, tag_size - (p - buffer));
            return tag_size;
        }
    }
    return 0;
}

int
id3tag_write_v2(lame_t gfp)
{
    lame_internal_flags *gfc = 0;

    if (is_lame_internal_flags_null(gfp)) {
        return 0;
    }
    gfc = gfp->internal_flags;
    if (test_tag_spec_flags(gfc, V1_ONLY_FLAG)) {
        return 0;
    }
    if (test_tag_spec_flags(gfc, CHANGED_FLAG)) {
        unsigned char *tag = 0;
        size_t  tag_size, n;

        n = lame_get_id3v2_tag(gfp, 0, 0);
        tag = lame_calloc(unsigned char, n);
        if (tag == 0) {
            return -1;
        }
        tag_size = lame_get_id3v2_tag(gfp, tag, n);
        if (tag_size > n) {
            free(tag);
            return -1;
        }
        else {
            size_t  i;
            /* write tag directly into bitstream at current position */
            for (i = 0; i < tag_size; ++i) {
                add_dummy_byte(gfc, tag[i], 1);
            }
        }
        free(tag);
        return (int) tag_size; /* ok, tag should not exceed 2GB */
    }
    return 0;
}

static unsigned char *
set_text_field(unsigned char *field, const char *text, size_t size, int pad)
{
    while (size--) {
        if (text && *text) {
            *field++ = *text++;
        }
        else {
            *field++ = pad;
        }
    }
    return field;
}

/*! Copy the ID3v1 tag into a buffer. */
/*!
  \ingroup api_tags
  The same as \c lame_get_id3v2_tag(), for the ID3v1 tag, for a caller that
  writes the tags itself. An ID3v1 tag is always exactly 128 bytes, and it
  goes at the very end of the file.

  \param gfp     the encoder instance.
  \param buffer  where to copy the tag.
  \param size    how large \a buffer is.
  \return 128 if \a size is less than 128. Nothing is copied then.
          Otherwise 128 if the tag was copied, and 0 if there is no ID3v1 tag
          to write, because the caller asked for ID3v2 only or set no field.
          0 also if \a buffer is NULL.
*/
size_t
lame_get_id3v1_tag(lame_t gfp, unsigned char *buffer, size_t size)
{
    size_t const tag_size = 128;
    lame_internal_flags *gfc;

    if (gfp == 0) {
        return 0;
    }
    if (size < tag_size) {
        return tag_size;
    }
    gfc = gfp->internal_flags;
    if (gfc == 0) {
        return 0;
    }
    if (buffer == 0) {
        return 0;
    }
    if (test_tag_spec_flags(gfc, V2_ONLY_FLAG)) {
        return 0;
    }
    if (test_tag_spec_flags(gfc, CHANGED_FLAG)) {
        unsigned char *p = buffer;
        int     pad = test_tag_spec_flags(gfc, SPACE_V1_FLAG) ? ' ' : 0;
        char    year[5];

        /* set tag identifier */
        *p++ = 'T';
        *p++ = 'A';
        *p++ = 'G';
        /* set each field in tag */
        p = set_text_field(p, gfc->tag_spec.title, 30, pad);
        p = set_text_field(p, gfc->tag_spec.artist, 30, pad);
        p = set_text_field(p, gfc->tag_spec.album, 30, pad);
        snprintf(year, sizeof(year), "%d", gfc->tag_spec.year);
        p = set_text_field(p, gfc->tag_spec.year ? year : NULL, 4, pad);
        /* limit comment field to 28 bytes if a track is specified */
        p = set_text_field(p, gfc->tag_spec.comment, gfc->tag_spec.track_id3v1 ? 28 : 30, pad);
        if (gfc->tag_spec.track_id3v1) {
            /* clear the next byte to indicate a version 1.1 tag */
            *p++ = 0;
            *p++ = gfc->tag_spec.track_id3v1;
        }
        *p++ = gfc->tag_spec.genre_id3v1;
        return tag_size;
    }
    return 0;
}

int
id3tag_write_v1(lame_t gfp)
{
    lame_internal_flags* gfc = 0;
    size_t  i, n, m;
    unsigned char tag[128];

    if (is_lame_internal_flags_null(gfp)) {
        return 0;
    }
    gfc = gfp->internal_flags;

    m = sizeof(tag);
    n = lame_get_id3v1_tag(gfp, tag, m);
    if (n > m) {
        return 0;
    }
    /* write tag directly into bitstream at current position */
    for (i = 0; i < n; ++i) {
        add_dummy_byte(gfc, tag[i], 1);
    }
    return (int) n;     /* ok, tag has fixed size of 128 bytes, well below 2GB */
}
