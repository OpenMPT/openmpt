/*
 * This program is  free software; you can redistribute it  and modify it
 * under the terms of the GNU  General Public License as published by the
 * Free Software Foundation; either version 2  of the license or (at your
 * option) any later version.
 *
 * Authors: Rani Assaf <rani@magic.metawire.com>,
 *          Olivier Lapicque <olivierl@jps.net>
*/

#ifndef _STDAFX_H_
#define _STDAFX_H_


#ifdef WIN32

#pragma warning (disable:4201)
#pragma warning (disable:4514)
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

#else
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef char CHAR;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
typedef unsigned long UINT;
typedef unsigned long DWORD;
typedef long LONG;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef unsigned char * LPBYTE;
typedef bool BOOL;
typedef char * LPSTR;
typedef void *  LPVOID;
typedef long * LPLONG;
typedef unsigned long * LPDWORD;
typedef unsigned short * LPWORD;
typedef const char * LPCSTR;


inline LONG MulDiv (long a, long b, long c)
{
  // if (!c) return 0;
  return ((unsigned long long) a * (unsigned long long) b ) / c;
}

#define  GHND   0

inline char * GlobalAllocPtr(unsigned int, size_t size)
{ 
  char * p = (char *) malloc(size);

  if (p != NULL) memset(p, 0, size);
  return p;
}

#define GlobalFreePtr(p) free((void *)(p))

#define strnicmp(a,b,c)		strncasecmp(a,b,c)
#define wsprintf			sprintf

#ifndef FALSE
#define FALSE	false
#endif

#ifndef TRUE
#define TRUE	true
#endif

#endif // WIN32

#endif



