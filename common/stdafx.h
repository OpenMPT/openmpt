/*
 * StdAfx.h
 * --------
 * Purpose: Include file for standard system include files, or project specific include files that are used frequently, but are changed infrequently
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#define _WIN32_WINNT	0x0500	// 0x0500 = Windows 2000

// windows excludes
#define NOMCX
// mmreg excludes
#define NOMMIDS
#define NOJPEGDIB
#define NONEWIC
#define NOBITMAP
// mmsystem excludes
#define MMNODRV
#define MMNOMCI

#define _CRT_SECURE_NO_WARNINGS		// Define to disable the "This function or variable may be unsafe" warnings.
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES			1
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT	1


#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>			// MFC support for Windows Common Controls
#include <afxcview.h>
#include <afxole.h>
#include <windowsx.h>

#pragma warning(disable:4201)
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <afxdlgs.h>
//#include <afxdhtml.h>
#pragma warning(default:4201)

#include <string>
#include <fstream>
#include <strstream>

#ifndef OFN_FORCESHOWHIDDEN
#define OFN_FORCESHOWHIDDEN		0x10000000
#endif

#ifndef _WAVEFORMATEXTENSIBLE_
#define _WAVEFORMATEXTENSIBLE_

typedef struct {
    WAVEFORMATEX    Format;
    union {
        WORD wValidBitsPerSample;       /* bits of precision  */
        WORD wSamplesPerBlock;          /* valid if wBitsPerSample==0 */
        WORD wReserved;                 /* If neither applies, set to zero. */
    } Samples;
    DWORD           dwChannelMask;      /* which channels are */
                                        /* present in stream  */
    GUID            SubFormat;
} WAVEFORMATEXTENSIBLE, *PWAVEFORMATEXTENSIBLE;

#endif // !_WAVEFORMATEXTENSIBLE_

#if !defined(WAVE_FORMAT_EXTENSIBLE)
#define  WAVE_FORMAT_EXTENSIBLE                 0xFFFE
#endif // !defined(WAVE_FORMAT_EXTENSIBLE)

// Define to build without ASIO support; makes build possible without ASIO SDK.
//#define NO_ASIO 

// (HACK) Define to build without VST support; makes build possible without VST SDK.
//#define NO_VST

// Define to build without MO3 support.
//#define NO_MO3_SUPPORT
				  
// Define to build without DirectSound support.
//#define NO_DSOUND

// Define to build without FLAC support
//#define NO_FLAC

// Define to build without MP3 import support (via mpg123)
//#define NO_MP3_SAMPLES


void Log(LPCSTR format,...);

#include "../common/typedefs.h"

// Exception type that is used to catch "operator new" exceptions.
//typedef std::bad_alloc & MPTMemoryException;
typedef CMemoryException * MPTMemoryException;

//To mark string that should be translated in case of multilingual version.
#define GetStrI18N(x)	(x)

#pragma warning(error : 4309) // Treat "truncation of constant value"-warning as error.


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
