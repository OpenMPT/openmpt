
/* C99 headers */

#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_LOCALE_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1

/* POSIX headers */

#define HAVE_SYS_TYPES_H 1

/* Windows headers */

#define HAVE_WINDOWS_H 1

/* C99 functions */

#define HAVE_ATOLL 1
#define HAVE_STRERROR 1
#define HAVE_SETLOCALE 1

/* yeah, POSIX ... */

#define SIZEOF_OFF_T 4

/* misc functions */

#define strcasecmp _strcmpi
#define strncasecmp _strnicmp

/* Features */

/* #define LFS_LARGEFILE_64 1 */

#define NO_CATCHSIGNAL

/* We want some frame index, eh? */
#define FRAME_INDEX 1
#define INDEX_SIZE 1000

/* also gapless playback! */
#define GAPLESS 1

/* new huffman decoding */
#define USE_NEW_HUFFTABLE 1

/* Debugging */

/* #define DEBUG */
/* #define EXTRA_DEBUG */

/* Precision */

/* use floating point */
#define REAL_IS_FLOAT 1

/* floating point is IEEE754 */
#define IEEE_FLOAT 1

/* use rounding instead of trunction */
#define ACCURATE_ROUNDING 1

/* Platform */

/* use the unicode support within libmpg123 */
#define WANT_WIN32_UNICODE 1
