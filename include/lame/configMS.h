#ifndef CONFIGMS_H_INCLUDED
#define CONFIGMS_H_INCLUDED

/* Silence the MSVC CRT deprecation warning (C4996) for the standard ISO C
   library functions LAME uses.  The "secure" _s replacements MSVC suggests are
   not portable, so the code stays on the standard names.  This header is the
   first include in every translation unit, so the definition takes effect
   before any CRT header is parsed.  Guarded so a build that already defines it
   on the command line does not trigger a redefinition warning. */
#ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS 1
#endif

/* The same for the POSIX names MSVC deprecates in favour of an underscored
   spelling of its own (strdup -> _strdup).  The POSIX name is the portable
   one, so the code keeps it. */
#ifndef _CRT_NONSTDC_NO_WARNINGS
# define _CRT_NONSTDC_NO_WARNINGS 1
#endif

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H

/* Name of package */
#define PACKAGE "lame"

/* Define if compiler has function prototypes */
#define PROTOTYPES 1

/* faster log implementation with less but enough precission */
#define USE_FAST_LOG 1

#define HAVE_STRCHR
#define HAVE_MEMCPY

/* Define if you have the <stdint.h> header file.  */
#define HAVE_STDINT_H 1

/* Define if you have the <sys/utime.h> header file and utime().  The Windows
   compilers put utime() there rather than in <utime.h>; the frontend needs
   both answers for --preserve-modtime.  */
#define HAVE_SYS_UTIME_H 1
#define HAVE_UTIME 1

/* <fpu_control.h> is a glibc header; the Windows compilers have their own way
   of reaching the floating point control word, which util.c uses instead, so
   this answer is no rather than absent.  */
/* #undef HAVE_FPU_CONTROL_H */

#if defined(_MSC_VER) || defined(__BORLANDC__)
#pragma warning( disable : 4305 )
#endif

typedef double      ieee754_float64_t;
typedef float       ieee754_float32_t;

#define LAME_LIBRARY_BUILD


/* The vector routines are available on every x86 target here: unlike GCC and
 * Clang, which reject the intrinsics unless the function opts into the
 * instruction set, this compiler accepts them whatever /arch is in force.  So
 * the routines are always built and the CPU decides at run time whether they
 * are used, exactly as on the Autotools side.
 *
 * That holds for the 512-bit registers too - verified rather than assumed,
 * since it decides whether a stock binary would fault on a CPU without them:
 * the kernels compile with no /arch:AVX512 and the object carries the wide
 * registers, the mask registers and the gathers (build/avx512-probe/).  What
 * confines that tier to alpha versions is the version test in
 * libmp3lame/vector/lame_intrin.h, which is where the Autotools side gates it
 * as well.
 */
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) \
 || (defined(__ICL) && (__ICL >= 450))
    #define HAVE_SSE2_INTRINSICS
#if !defined(_M_ARM64EC)  /* OpenMPT */
    #define HAVE_AVX2_INTRINSICS
    #define HAVE_AVX512_INTRINSICS
#endif  /* OpenMPT */
#endif

#if defined(_M_ARM) || defined(_M_ARM64) || defined(_M_ARM64EC)  /* OpenMPT */
    #define HAVE_NEON_INTRINSICS  /* OpenMPT */
#endif  /* OpenMPT */

/* mp3rtp offers IPv6 destinations as well as IPv4 ones. This is the same
 * switch the Autotools side spells --disable-ipv6, and it selects the same
 * thing: the address family the resolver is asked for. Comment it out for a
 * build that accepts IPv4 destinations only. Every Windows this project
 * supports has the types and socket options it needs.
 */
#define IPV6 1

#endif
