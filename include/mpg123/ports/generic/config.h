
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

/* libmpg123 does not care about strtok */
#define NO_STRTOK

/* We want some frame index, eh? */
#define FRAME_INDEX 1
#define INDEX_SIZE 1000

/* also gapless playback! */
#define GAPLESS 1

/* Debugging */

/* #define DEBUG */
/* #define EXTRA_DEBUG */

/* CPU Features */

#if defined(__GNUC__) || defined(__clang__)

#if defined(_SOFT_FLOAT)
#define MPT123_NOFPU
#endif

#if defined(__amd64__) || defined(__x86_64__)

#elif defined(__i386__) || defined(_X86_)

#ifndef MPG123_ISA_X86_386
#define MPG123_ISA_X86_386
#endif

#if defined(__i486__)
#ifndef MPG123_ISA_X86_486
#define MPG123_ISA_X86_486
#endif
#endif

#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)
#ifndef MPG123_ISA_X86_486
#define MPG123_ISA_X86_486
#endif
#ifndef MPG123_ISA_X86_586
#define MPG123_ISA_X86_586
#endif
#endif

#if defined(__tune_i386__)
#define MPG123_TUNE_386
#endif
#if defined(__tune_i486__)
#define MPG123_TUNE_486
#endif

#elif defined(__arm__)

#if defined(__SOFTFP__)
#define MPT123_NOFPU
#endif

#elif defined(__mips__)

#if defined(__mips_soft_float)
#define MPT123_NOFPU
#endif

#endif

#endif

#if defined(MPG123_ISA_X86_386) && defined(MPG123_TUNE_386) && defined(MPT123_NOFPU)

#ifndef OPT_I386
#define OPT_I386
#endif
/* do not use floating point */
#define REAL_IS_FIXED 1
#define NO_SYNTH32 1

#elif defined(MPT123_NOFPU)

#ifndef OPT_GENERIC
#define OPT_GENERIC
#endif
/* do not use floating point */
#define REAL_IS_FIXED 1
#define NO_SYNTH32 1

#elif defined(MPG123_ISA_X86_386) && defined(MPG123_TUNE_386)

#ifndef OPT_I386
#define OPT_I386
#endif
/* use floating point */
#define REAL_IS_FLOAT 1

#elif defined(MPG123_ISA_X86_486) && defined(MPG123_TUNE_486)

#ifndef OPT_I486
#define OPT_I486
#endif
/* use floating point */
#define REAL_IS_FLOAT 1

#else

#ifndef OPT_MULTI
#define OPT_MULTI
#endif
#ifndef OPT_GENERIC
#define OPT_GENERIC
#endif
#ifndef OPT_GENERIC_DITHER
#define OPT_GENERIC_DITHER
#endif
/* use floating point */
#define REAL_IS_FLOAT 1
/* use rounding instead of trunction */
#define ACCURATE_ROUNDING 1
/* new huffman decoding */
#define USE_NEW_HUFFTABLE 1

#endif

/* Endian */
#if defined(_MSC_VER)
#define WORDS_LITTLEENDIAN 1
#elif defined(__GNUC__) || defined(__clang__)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define WORDS_BIGENDIAN 1
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define WORDS_LITTLEENDIAN 1
#endif
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && defined(__ORDER_LITTLE_ENDIAN__)
#if __ORDER_BIG_ENDIAN__ != __ORDER_LITTLE_ENDIAN__
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define WORDS_BIGENDIAN 1
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define WORDS_LITTLEENDIAN 1
#endif
#endif
#endif
/* fallback */
#if !defined(WORDS_BIGENDIAN) && !defined(WORDS_LITTLEENDIAN)
#if (defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)) || (defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)) || (defined(_STLP_BIG_ENDIAN) && !defined(_STLP_LITTLE_ENDIAN))
#define WORDS_BIGENDIAN 1
#elif (defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)) || (defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)) || (defined(_STLP_LITTLE_ENDIAN) && !defined(_STLP_BIG_ENDIAN))
#define WORDS_LITTLEENDIAN 1
#elif defined(__hpux) || defined(__hppa) || defined(_MIPSEB) || defined(__s390__)
#define WORDS_BIGENDIAN 1
#elif defined(__i386__) || defined(_M_IX86) || defined(__amd64) || defined(__amd64__) || defined(_M_AMD64) || defined(__x86_64) || defined(__x86_64__) || defined(_M_X64) || defined(__bfin__)
#define WORDS_LITTLEENDIAN 1
#endif
#endif

/* floating point is IEEE754 */
#if defined(_WIN32)
#ifndef IEEE_FLOAT
#define IEEE_FLOAT 1
#endif
#endif
#if defined(__STDC_VERSION__)
#if (__STDC_VERSION__ >= 199901L)
#if defined(__STDC_IEC_559__)
#if (__STDC_IEC_559__ == 1)
#ifndef IEEE_FLOAT
#define IEEE_FLOAT 1
#endif
#endif
#endif
#endif
#if (__STDC_VERSION__ >= 202311L)
#if defined(__STDC_IEC_60559_BFP__)
#if (__STDC_IEC_60559_BFP__ >= 202311L)
#ifndef IEEE_FLOAT
#define IEEE_FLOAT 1
#endif
#endif
#endif
#endif
#endif

/* Platform */

/* use the unicode support within libmpg123 */
#if defined(_WIN32)
#if defined(UNICODE)
#define WANT_WIN32_UNICODE 1
#endif
#endif
