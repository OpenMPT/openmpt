/*
 * This source code is public domain.
 *
 * Authors: Rani Assaf <rani@magic.metawire.com>,
 *          Olivier Lapicque <olivierl@jps.net>,
 *          Adam Goode       <adam@evdebs.org> (endian and char fixes for PPC)
 */

#ifndef MODPLUG_STDAFX_H  /* OpenMPT */
#define MODPLUG_STDAFX_H  /* OpenMPT */

/* Autoconf detection of stdint/inttypes */
#if defined(HAVE_CONFIG_H) && !defined(CONFIG_H_INCLUDED)
# include "config.h"
# define CONFIG_H_INCLUDED 1
#endif
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif

/* disable AGC and FILESAVE for all targets for uniformity. */
#define NO_AGC
#define MODPLUG_NO_FILESAVE

#ifdef _WIN32

#ifdef _MSC_VER
#pragma warning (disable:4201)
#pragma warning (disable:4514)
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h> /* for WAVE_FORMAT_PCM */
#include <stdio.h>
#include <malloc.h>
#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef signed char    int8_t;
typedef signed short   int16_t;
typedef signed int     int32_t;
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef __int64        int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#define sleep(_ms)      Sleep(_ms * 1000)

inline void ProcessPlugins(int n) { (void)n; }

#undef strcasecmp
#undef strncasecmp
#define strcasecmp(a,b)     _stricmp(a,b)
#define strncasecmp(a,b,c)  _strnicmp(a,b,c)

#if defined(_MSC_VER) || defined(__MINGW32__)
#define HAVE_SINF 1
#endif

#ifndef isblank
#define isblank(c) ((c) == ' ' || (c) == '\t')
#endif

#else

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

typedef int8_t CHAR;
typedef uint8_t UCHAR;
typedef uint8_t* PUCHAR;
typedef uint16_t USHORT;
typedef uint32_t ULONG;
typedef uint32_t UINT;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef int64_t LONGLONG;
typedef int32_t* LPLONG;
typedef uint32_t* LPDWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef uint8_t* LPBYTE;
typedef bool BOOL; /* FIXME: must be 'int' */
typedef char* LPSTR;
typedef void* LPVOID;
typedef uint16_t* LPWORD;
typedef const char* LPCSTR;
typedef void* PVOID;
typedef void VOID;

#define lstrcpynA strncpy
#define lstrcpyA strcpy
#define wsprintfA sprintf

#define WAVE_FORMAT_PCM 1

inline void ProcessPlugins(int n) { (void)n; }

#ifndef FALSE
#define FALSE	false
#endif

#ifndef TRUE
#define TRUE	true
#endif

#endif /* _WIN32 */

#if defined(_WIN32) || defined(__CYGWIN__)
# if defined(MODPLUG_BUILD) && defined(DLL_EXPORT)	/* building libmodplug as a dll for windows */
#   define MODPLUG_EXPORT __declspec(dllexport)
# elif defined(MODPLUG_BUILD) || defined(MODPLUG_STATIC)	/* building or using static libmodplug for windows */
#   define MODPLUG_EXPORT
# else
#   define MODPLUG_EXPORT __declspec(dllimport)			/* using libmodplug dll for windows */
# endif
#elif defined(__OS2__) && defined(__WATCOMC__)
# if defined(MODPLUG_BUILD) && defined(__SW_BD)		/* building libmodplug as a dll for os/2 */
#   define MODPLUG_EXPORT __declspec(dllexport)
# else
#   define MODPLUG_EXPORT					/* using dll or static libmodplug for os/2 */
# endif
#elif defined(MODPLUG_BUILD) && defined(SYM_VISIBILITY)
#   define MODPLUG_EXPORT __attribute__((visibility("default")))
#else
#define MODPLUG_EXPORT
#endif

#endif
