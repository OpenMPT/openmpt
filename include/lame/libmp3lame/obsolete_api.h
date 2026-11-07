/*
 *      Prototypes of the obsolete functions that libmp3lame still exports
 *
 *      Copyright (c) 2026 The LAME project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * \file
 * \internal
 * \brief Prototypes of the obsolete functions that the library still exports.
 *
 * Programs built against an older release still link against these
 * functions. lame.h declares them only when DEPRECATED_OR_OBSOLETE_CODE_REMOVED
 * is 0, so the files that define them take the prototypes from here.
 */

#ifndef LAME_OBSOLETE_API_H
#define LAME_OBSOLETE_API_H

#include "lame.h"

#if defined(__cplusplus)
extern  "C" {
#endif

/* set_get.c */
int CDECL lame_set_ogg(lame_global_flags *, int);
int CDECL lame_get_ogg(const lame_global_flags *);
int CDECL lame_set_mode_automs(lame_global_flags *, int);
int CDECL lame_get_mode_automs(const lame_global_flags *);
int CDECL lame_set_ReplayGain_input(lame_global_flags *, int);
int CDECL lame_get_ReplayGain_input(const lame_global_flags *);
int CDECL lame_set_ReplayGain_decode(lame_global_flags *, int);
int CDECL lame_get_ReplayGain_decode(const lame_global_flags *);
int CDECL lame_set_findPeakSample(lame_global_flags *, int);
int CDECL lame_get_findPeakSample(const lame_global_flags *);
int CDECL lame_set_padding_type(lame_global_flags *, Padding_type);
Padding_type CDECL lame_get_padding_type(const lame_global_flags *);
int CDECL lame_set_athaa_loudapprox(lame_global_flags * gfp, int athaa_loudapprox);
int CDECL lame_get_athaa_loudapprox(const lame_global_flags * gfp);
int     lame_set_cwlimit(lame_global_flags * gfp, int cwlimit);
int     lame_get_cwlimit(const lame_global_flags * gfp);
int CDECL lame_set_preset_expopts(lame_global_flags *, int);

/* lame.c */
int CDECL lame_encode_finish(lame_global_flags * gfp, unsigned char *mp3buffer, int mp3buffer_size);

/* mpglib_interface.c */
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

/* id3tag.c */
int     id3tag_set_textinfo_ucs2(lame_t gfp, char const *id, unsigned short const *text);
int     id3tag_set_comment_ucs2(lame_t gfp, char const *lang, unsigned short const *desc,
                                unsigned short const *text);
int     id3tag_set_fieldvalue_ucs2(lame_t gfp, const unsigned short *fieldvalue);

#if defined(__cplusplus)
}
#endif

#endif /* LAME_OBSOLETE_API_H */
