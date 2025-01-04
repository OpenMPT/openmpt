
#include <stddef.h>

#if defined(__MINGW32__) || defined(__MINGW64__)
#define HAVE_MINGW_CRT 1
#elif defined(_MSC_VER) || defined(_UCRT)
#define HAVE_MS_CRT 1
#else
#define HAVE_POSIX_CRT 1
#endif

/* C99 headers */

#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_LOCALE_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1

/* Windows headers */

#if defined(_WIN32)
#define HAVE_WINDOWS_H 1
#endif

/* POSIX headers */

#if defined(HAVE_POSIX_CRT) || defined(HAVE_MINGW_CRT)
#define HAVE_STRINGS_H 1
#endif
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#if defined(HAVE_POSIX_CRT) || defined(HAVE_MINGW_CRT)
#define HAVE_UNISTD_H 1
#endif

/* C99 functions */

#define HAVE_ATOLL 1
#define HAVE_STRERROR 1

/* POSIX functions */

#if defined(HAVE_MS_CRT)
#define strcasecmp _strcmpi
#define strncasecmp _strnicmp
#endif

/* yeah, POSIX ... */

#if defined(HAVE_POSIX_CRT)
#if defined(__DJGPP__)
#define SIZEOF_OFF_T 4
#elif defined(_FILE_OFFSET_BITS)
#if (_FILE_OFFSET_BITS == 64)
#define SIZEOF_OFF_T 8
#elif (_FILE_OFFSET_BITS == 32)
#define SIZEOF_OFF_T 4
#else
#define SIZEOF_OFF_T 4
#endif
#else
#define SIZEOF_OFF_T 8
#endif
#elif defined(HAVE_MINGW_CRT)
#if defined(_FILE_OFFSET_BITS)
#if (_FILE_OFFSET_BITS == 64)
#define SIZEOF_OFF_T 8
#elif (_FILE_OFFSET_BITS == 32)
#define SIZEOF_OFF_T 4
#else
#define SIZEOF_OFF_T 4
#endif
#else
#define SIZEOF_OFF_T 4
#endif
#elif defined(HAVE_MS_CRT)
#define SIZEOF_OFF_T 4
#else
#define SIZEOF_OFF_T 8
#endif

/* Features */

/* #define LFS_LARGEFILE_64 1 */

/* libmpg123 does not care about signals */
#define NO_CATCHSIGNAL

/* libmpg123 does not care about directories */
#define NO_DIR

/* libmpg123 does not care about environment variables */
#define NO_ENV

/* libmpg123 does not care about file mode */
#define NO_FILEMODE

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
#if defined(_WIN32)
#define IEEE_FLOAT 1
#endif

/* use rounding instead of trunction */
#define ACCURATE_ROUNDING 1

/* Platform */

/* use the unicode support within libmpg123 */
#if defined(_WIN32)
#if defined(UNICODE)
#define WANT_WIN32_UNICODE 1
#endif
#endif
