
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

#define HAVE_STRINGS_H 1
#if !defined(__DJGPP__)
#define HAVE_SYS_SIGNAL_H 1
#endif
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1

/* C99 functions */

#define HAVE_ATOLL 1
#define HAVE_STRERROR 1

/* yeah, POSIX ... */

#if defined(__DJGPP__)
#define SIZEOF_OFF_T 4
#else
#define SIZEOF_OFF_T 8
#endif

/* Windows/DOS */

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

/* Debugging */

/* #define DEBUG */
/* #define EXTRA_DEBUG */

/* Precision */

/* use floating point */
#define REAL_IS_FLOAT 1

/* floating point is IEEE754 */
/* #define IEEE_FLOAT 1 */

/* use rounding instead of trunction */
#define ACCURATE_ROUNDING 1

/* Platform */
