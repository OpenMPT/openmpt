
#ifndef LAME_ID3_H
#define LAME_ID3_H

#include "lame.h"
#include "machine.h"


#define CHANGED_FLAG    (1U << 0)
#define ADD_V2_FLAG     (1U << 1)
#define V1_ONLY_FLAG    (1U << 2)
#define V2_ONLY_FLAG    (1U << 3)
#define SPACE_V1_FLAG   (1U << 4)
#define PAD_V2_FLAG     (1U << 5)
// flag for enabling writing ID3v2.4 with UTF-8 characters
#define V2_4_UTF8_FLAG  (1U << 6)

enum {
    MIMETYPE_NONE = 0,
    MIMETYPE_JPEG,
    MIMETYPE_PNG,
    MIMETYPE_GIF
};

/** \internal The text encoding byte of an ID3v2 frame. */
typedef enum id3v2_text_encoding {
    ID3V2_ENC_LATIN1 = 0,    /**< ISO-8859-1 */
    ID3V2_ENC_UCS2 = 1,      /**< UCS-2 with a byte order mark */
    ID3V2_ENC_UTF8 = 3       /**< UTF-8, ID3v2.4 only */
} id3v2_text_encoding;

/** \internal A string in an ID3v2 frame: the description or the text. */
typedef struct FrameString {
    union {
        char   *l;           /* ptr to Latin-1 chars             */
        unsigned short *u;   /* ptr to UCS-2 text                */
        unsigned char *b;    /* ptr to raw bytes                 */
    } ptr;
    size_t  dim;
    id3v2_text_encoding enc;
} FrameString;

typedef struct FrameDataNode {
    struct FrameDataNode *nxt;
    uint32_t fid;             /* Frame Identifier                 */
    char    lng[4];          /* 3-character language descriptor  */
    FrameString dsc, txt;
} FrameDataNode;


typedef struct id3tag_spec {
    /* private data members */
    unsigned int flags;
    int     year;
    char   *title;
    char   *artist;
    char   *album;
    char   *comment;
    int     track_id3v1;
    int     genre_id3v1;
    unsigned char *albumart;
    unsigned int albumart_size;
    unsigned int padding_size;
    int     albumart_mimetype;
    char    language[4]; /* the language of the frame's content, according to ISO-639-2 */
    FrameDataNode *v2_head, *v2_tail;
} id3tag_spec;


/* write tag into stream at current position */
extern int id3tag_write_v2(lame_global_flags * gfp);
extern int id3tag_write_v1(lame_global_flags * gfp);
/*
 * NOTE: A version 2 tag will NOT be added unless one of the text fields won't
 * fit in a version 1 tag (e.g. the title string is longer than 30 characters),
 * or the "id3tag_add_v2", "id3tag_add_v2_4_UTF8", "id3tag_v2_only", or
 * "id3tag_v2_4_UTF8_only" functions are used.
 */

#endif
