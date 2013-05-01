/*
 * BuildSettings.h
 * ---------------
 * Purpose: Global, user settable compile time flags (and some global system header configuration)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once



// Use inline assembly at all
#define ENABLE_ASM

// inline assembly requires MSVC compiler
#if defined(ENABLE_ASM) && defined(_MSC_VER)

// Generate general x86 inline assembly.
#define ENABLE_X86

// Generate inline assembly using MMX instructions (only used when the CPU supports it).
#define ENABLE_MMX

// Generate inline assembly using 3DNOW instructions (only used when the CPU supports it).
#define ENABLE_3DNOW

// Generate inline assembly using SSE instructions (only used when the CPU supports it).
#define ENABLE_SSE

#endif // ENABLE_ASM

// Disable any debug logging
//#define NO_LOGGING

// Disable unarchiving support
//#define NO_ARCHIVE_SUPPORT

// Disable std::istream support in class FileReader (this is generally not needed for the tracker, local files can easily be mmapped as they have been before introducing std::istream support)
#define NO_FILEREADER_STD_ISTREAM

// Disable the built-in reverb effect
//#define NO_REVERB

// Disable built-in miscellaneous DSP effects (surround, mega bass, noise reduction) 
//#define NO_DSP

// Disable the built-in equalizer.
//#define NO_EQ

// Disable the built-in automatic gain control
//#define NO_AGC

// Define to build without ASIO support; makes build possible without ASIO SDK.
//#define NO_ASIO 

// (HACK) Define to build without VST support; makes build possible without VST SDK.
//#define NO_VST

// Define to build without portaudio.
//#define NO_PORTAUDIO

// Define to build without MO3 support.
//#define NO_MO3

// Define to build without DirectSound support.
//#define NO_DSOUND

// Define to build without FLAC support
//#define NO_FLAC

// Define to build without MP3 import support (via mpg123)
//#define NO_MP3_SAMPLES



#ifdef _MSC_VER
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#ifdef _WIN32

#define _WIN32_WINNT	0x0500	// 0x0500 = Windows 2000

// no stupid min/max macros
#define NOMINMAX
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

#endif

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS		// Define to disable the "This function or variable may be unsafe" warnings.
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES			1
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT	1
#define _SCL_SECURE_NO_WARNINGS
#endif
