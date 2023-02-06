
/* C99 headers */

#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1

/* POSIX headers */

#define HAVE_DIRENT_H 1
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

#define OFF_MAX ((off_t)((sizeof(off_t) == 4) ? ((uint32_t)-1/2) : (sizeof(off_t) == 8) ? ((uint64_t)-1/2) : 0))

/* Features */

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
