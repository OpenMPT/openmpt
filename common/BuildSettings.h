/*
 * BuildSettings.h
 * ---------------
 * Purpose: Global, user settable compile time flags (and some global system header configuration)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once



#include "CompilerDetect.h"



#ifdef MODPLUG_TRACKER

// Use inline assembly at all
#define ENABLE_ASM

#else

// Do not use inline asm in library builds. There is just about no codepath which would use it anyway.
//#define ENABLE_ASM

#endif



// inline assembly requires MSVC compiler
#if defined(ENABLE_ASM) && MPT_COMPILER_MSVC

#ifdef _M_IX86

// Generate general x86 inline assembly / intrinsics.
#define ENABLE_X86

// Generate inline assembly using MMX instructions (only used when the CPU supports it).
#define ENABLE_MMX

// Generate inline assembly using SSE instructions (only used when the CPU supports it).
#define ENABLE_SSE

// Generate inline assembly using SSE2 instructions (only used when the CPU supports it).
#define ENABLE_SSE2

// Generate inline assembly using SSE3 instructions (only used when the CPU supports it).
#define ENABLE_SSE3

// Generate inline assembly using SSE4 instructions (only used when the CPU supports it).
#define ENABLE_SSE4

// Generate inline assembly using AMD specific instruction set extensions (only used when the CPU supports it).
#define ENABLE_X86_AMD

#elif defined(_M_X64)

// Generate general x64 inline assembly / intrinsics.
#define ENABLE_X64

// Generate inline assembly using SSE2 instructions (only used when the CPU supports it).
#define ENABLE_SSE2

// Generate inline assembly using SSE3 instructions (only used when the CPU supports it).
#define ENABLE_SSE3

// Generate inline assembly using SSE4 instructions (only used when the CPU supports it).
#define ENABLE_SSE4

#endif

#endif // ENABLE_ASM



#if defined(MODPLUG_TRACKER) && defined(LIBOPENMPT_BUILD)

#error "either MODPLUG_TRACKER or LIBOPENMPT_BUILD has to be defined"

#elif defined(MODPLUG_TRACKER)

// Enable built-in test suite.
#ifdef _DEBUG
#define ENABLE_TESTS
#endif

// Disable any file saving functionality (not really useful except for the player library)
//#define MODPLUG_NO_FILESAVE

// Disable any debug logging
//#define NO_LOGGING
#ifndef _DEBUG
#define MPT_LOG_GLOBAL_LEVEL_STATIC
#define MPT_LOG_GLOBAL_LEVEL 0
#endif

// Disable all runtime asserts
#ifndef _DEBUG
#define NO_ASSERTS
#endif

// Enable std::istream support in class FileReader (this is generally not needed for the tracker, local files can easily be mmapped as they have been before introducing std::istream support)
//#define MPT_FILEREADER_STD_ISTREAM

// Enable performance optimizations for seekable std::istream. Note that these probably will only payoff once FileReader::GetRawData() usage gets further reduced.
//#define MPT_FILEREADER_STD_ISTREAM_SEEKABLE

// Enable callback stream wrapper for FileReader (required by libopenmpt C API).
//#define MPT_FILEREADER_CALLBACK_STREAM

// Support for externally linked samples e.g. in MPTM files
#define MPT_EXTERNAL_SAMPLES

// Disable unarchiving support
//#define NO_ARCHIVE_SUPPORT
#define UNGZIP_SUPPORT
#define ZIPPED_MOD_SUPPORT
#define UNLHA_SUPPORT
#define UNRAR_SUPPORT

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

// Define to build without VST plugin support; makes build possible without VST SDK.
//#define NO_VST

// Define to build without DMO plugin support
//#define NO_DMO

// (HACK) Define to build without any plugin support
//#define NO_PLUGINS

// Define to build without portaudio.
//#define NO_PORTAUDIO

// Define to build without MO3 support.
//#define NO_MO3

// Define to build without DirectSound support.
//#define NO_DSOUND

// Define to build without OGG support
//#define NO_OGG

// Define to build without FLAC support
//#define NO_FLAC

// Define to build without zlib support
//#define NO_ZLIB

// Define to build without miniz support
#define NO_MINIZ

// Define to build without MP3 import support (via mpg123)
//#define NO_MP3_SAMPLES

// Define to build without MediaFoundation codec support
//#define NO_MEDIAFOUNDATION

// Do not build libopenmpt C api
#define NO_LIBOPENMPT_C

// Do not build libopenmpt C++ api
#define NO_LIBOPENMPT_CXX

#elif defined(LIBOPENMPT_BUILD)

#if defined(HAVE_CONFIG_H)
// wrapper for autoconf macros
#include "config.h"
#endif // HAVE_CONFIG_H

#if (defined(_DEBUG) || defined(DEBUG)) && !defined(MPT_BUILD_DEBUG)
#define MPT_BUILD_DEBUG
#endif

#if defined(LIBOPENMPT_BUILD_TEST)
#define ENABLE_TESTS
#else
#define MODPLUG_NO_FILESAVE
#endif
#if defined(MPT_BUILD_CHECKED) || defined(ENABLE_TESTS)
// enable asserts
#else
#define NO_ASSERTS
#endif
//#define NO_LOGGING
#define MPT_FILEREADER_STD_ISTREAM
#define MPT_FILEREADER_STD_ISTREAM_SEEKABLE
#define MPT_FILEREADER_CALLBACK_STREAM
//#define MPT_EXTERNAL_SAMPLES
#define NO_ARCHIVE_SUPPORT
//#define UNGZIP_SUPPORT
//#define ZIPPED_MOD_SUPPORT
//#define UNLHA_SUPPORT
//#define UNRAR_SUPPORT
#define NO_REVERB
#define NO_DSP
#define NO_EQ
#define NO_AGC
#define NO_ASIO
#define NO_VST
#define NO_DMO
#define NO_PLUGINS
#define NO_PORTAUDIO
#if !defined(MPT_WITH_MO3) && !(MPT_COMPILER_MSVC)
#ifndef NO_MO3
#define NO_MO3
#endif
#endif
#define NO_DSOUND
#define NO_OGG
#define NO_FLAC
#if !defined(MPT_WITH_ZLIB)
#ifndef NO_ZLIB
#define NO_ZLIB
#endif
#endif
//#define NO_MINIZ
#if !defined(MPT_WITH_MPG123)
#define NO_MP3_SAMPLES
#endif
#define NO_MEDIAFOUNDATION
//#define NO_LIBOPENMPT_C
//#define NO_LIBOPENMPT_CXX

#else

#error "either MODPLUG_TRACKER or LIBOPENMPT_BUILD has to be defined"

#endif



#if MPT_OS_WINDOWS

	#define MPT_CHARSET_WIN32

#elif MPT_OS_LINUX

	#define MPT_CHARSET_ICONV

#elif MPT_OS_ANDROID

	#define MPT_CHARSET_INTERNAL

#elif MPT_OS_EMSCRIPTEN

	#define MPT_CHARSET_CODECVTUTF8
	#ifndef MPT_LOCALE_ASSUME_CHARSET
	#define MPT_LOCALE_ASSUME_CHARSET CharsetUTF8
	#endif

#elif MPT_OS_MACOSX_OR_IOS

	#if defined(MPT_WITH_ICONV)
	#define MPT_CHARSET_ICONV
	#ifndef MPT_ICONV_NO_WCHAR
	#define MPT_ICONV_NO_WCHAR
	#endif
	#else
	#define MPT_CHARSET_CODECVTUTF8
	#endif

#elif defined(MPT_WITH_ICONV)

	#define MPT_CHARSET_ICONV

#endif



#if MPT_COMPILER_MSVC

	// Use wide strings for MSVC because this is the native encoding on 
	// microsoft platforms.
	#define MPT_USTRING_MODE_WIDE 1
	#define MPT_USTRING_MODE_UTF8 0

#else // !MPT_COMPILER_MSVC

	#define MPT_USTRING_MODE_WIDE 0
	#define MPT_USTRING_MODE_UTF8 1

#endif // MPT_COMPILER_MSVC

#if MPT_USTRING_MODE_UTF8

	// MPT_USTRING_MODE_UTF8 mpt::ustring is implemented via mpt::u8string
	#define MPT_WITH_U8STRING 1

#else

	#define MPT_WITH_U8STRING 0

#endif

#if defined(MODPLUG_TRACKER) || MPT_USTRING_MODE_WIDE

	// mpt::ToWString, mpt::wfmt, ConvertStrTo<std::wstring>
	// Required by the tracker to ease interfacing with WinAPI.
	// Required by MPT_USTRING_MODE_WIDE to ease type tunneling in mpt::String::Print.
	#define MPT_WSTRING_FORMAT 1

#else

	#define MPT_WSTRING_FORMAT 0

#endif

#if MPT_OS_WINDOWS || MPT_USTRING_MODE_WIDE || MPT_WSTRING_FORMAT

	// mpt::ToWide
	// Required on Windows by mpt::PathString.
	// Required by MPT_USTRING_MODE_WIDE as they share the conversion functions.
	// Required by MPT_WSTRING_FORMAT because of std::string<->std::wstring conversion in mpt::ToString and mpt::ToWString.
	#define MPT_WSTRING_CONVERT 1

#else

	#define MPT_WSTRING_CONVERT 0

#endif



// fixing stuff up

#if !defined(ENABLE_MMX) && !defined(NO_REVERB)
#define NO_REVERB // reverb requires mmx
#endif

#if !defined(ENABLE_X86) && !defined(NO_DSP)
#define NO_DSP // DSP requires x86 inline asm
#endif

#if defined(ENABLE_TESTS) && defined(MODPLUG_NO_FILESAVE)
#undef MODPLUG_NO_FILESAVE // tests recommend file saving
#endif

#if !defined(NO_ZLIB) && !defined(NO_MINIZ)
// Only one deflate implementation should be used. Prefer zlib.
#define NO_MINIZ
#endif

#if !MPT_OS_WINDOWS && !defined(NO_ASIO)
#define NO_ASIO // ASIO requires Windows
#endif

#if !MPT_OS_WINDOWS && !defined(NO_DSOUND)
#define NO_DSOUND // DirectSound requires Windows
#endif

#if !MPT_OS_WINDOWS && !defined(NO_MEDIAFOUNDATION)
#define NO_MEDIAFOUNDATION // MediaFoundation requires Windows
#endif

#if MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0) && !defined(NO_MEDIAFOUNDATION)
#define NO_MEDIAFOUNDATION // MediaFoundation requires a modern SDK
#endif

#if !defined(MPT_CHARSET_WIN32) && !defined(MPT_CHARSET_ICONV) && !defined(MPT_CHARSET_CODECVTUTF8) && !defined(MPT_CHARSET_INTERNAL)
#define MPT_CHARSET_INTERNAL
#endif

#if defined(MODPLUG_TRACKER) && !defined(MPT_WITH_DYNBIND)
#define MPT_WITH_DYNBIND // Tracker requires dynamic library loading for export codecs
#endif

#if !defined(MPT_WITH_MPG123) && !defined(NO_MP3_SAMPLES) && !defined(MPT_WITH_DYNBIND)
#define MPT_WITH_DYNBIND // mpg123 is loaded dynamically
#endif

#if !defined(NO_MO3) && !defined(MPT_WITH_DYNBIND)
#define MPT_WITH_DYNBIND // unmo3 is loaded dynamically
#endif

#if defined(ENABLE_TESTS) && !defined(MPT_WITH_PATHSTRING)
#define MPT_WITH_FILEIO // Test suite requires PathString for file loading.
#endif

#if !MPT_OS_WINDOWS && !defined(MPT_FILEREADER_STD_ISTREAM)
#define MPT_FILEREADER_STD_ISTREAM // MMAP is only supported on Windows
#endif

#if defined(MODPLUG_TRACKER) && !defined(MPT_WITH_FILEIO)
#define MPT_WITH_FILEIO // Tracker requires disk file io
#endif

#if defined(MODPLUG_TRACKER) && !defined(MPT_WITH_THREAD)
#define MPT_WITH_THREAD // Tracker requires threads
#endif

#if defined(MODPLUG_TRACKER) && !defined(MPT_WITH_ATOMIC)
#define MPT_WITH_ATOMIC // Tracker requires threads
#endif

#if defined(MPT_EXTERNAL_SAMPLES) && !defined(MPT_WITH_FILEIO)
#define MPT_WITH_FILEIO // External samples require disk file io
#endif

#if !defined(NO_ARCHIVE_SUPPORT) && !defined(MPT_WITH_PATHSTRING)
#define MPT_WITH_PATHSTRING // archive requires PathString
#endif

#if defined(MPT_WITH_FILEIO) && !defined(MPT_WITH_PATHSTRING)
#define MPT_WITH_PATHSTRING // disk file io requires PathString
#endif

#if defined(MPT_WITH_DYNBIND) && !defined(MPT_WITH_PATHSTRING)
#define MPT_WITH_PATHSTRING // dynamic library loading requires PathString
#endif

#if !defined(MODPLUG_NO_FILESAVE) && !defined(MPT_WITH_PATHSTRING)
#define MPT_WITH_PATHSTRING // file saving requires PathString
#endif

#if !defined(MODPLUG_NO_FILESAVE) && !defined(MPT_WITH_FILEIO_STDIO)
#define MPT_WITH_FILEIO_STDIO // file saving requires FILE*
#endif

#if defined(MPT_WITH_PATHSTRING) && !defined(MPT_WITH_CHARSET_LOCALE)
#define MPT_WITH_CHARSET_LOCALE // PathString requires locale charset
#endif

#if defined(NO_PLUGINS)
// Any plugin type requires NO_PLUGINS to not be defined.
#define NO_VST
#define NO_DMO
#endif



#if defined(MODPLUG_TRACKER)
#ifndef MPT_NO_NAMESPACE
#define MPT_NO_NAMESPACE
#endif
#endif

#if defined(MPT_NO_NAMESPACE)

#ifdef OPENMPT_NAMESPACE
#undef OPENMPT_NAMESPACE
#endif
#define OPENMPT_NAMESPACE

#ifdef OPENMPT_NAMESPACE_BEGIN
#undef OPENMPT_NAMESPACE_BEGIN
#endif
#define OPENMPT_NAMESPACE_BEGIN

#ifdef OPENMPT_NAMESPACE_END
#undef OPENMPT_NAMESPACE_END
#endif
#define OPENMPT_NAMESPACE_END

#else

#ifndef OPENMPT_NAMESPACE
#define OPENMPT_NAMESPACE OpenMPT
#endif

#ifndef OPENMPT_NAMESPACE_BEGIN
#define OPENMPT_NAMESPACE_BEGIN namespace OPENMPT_NAMESPACE {
#endif
#ifndef OPENMPT_NAMESPACE_END
#define OPENMPT_NAMESPACE_END   }
#endif

#endif



#if MPT_COMPILER_MSVC
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#if defined(_WIN32)

#if MPT_COMPILER_MSVC && MPT_MSVC_AT_LEAST(2010,0)
#define _WIN32_WINNT        0x0601 // _WIN32_WINNT_WIN7
#else
#define _WIN32_WINNT        0x0500 // _WIN32_WINNT_WIN2000
#endif
#define WINVER              _WIN32_WINNT
#define WIN32_LEAN_AND_MEAN

// windows.h excludes
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMINMAX          // Macros min(a,b) and max(a,b)
#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOPROFILER        // Profiler interface.
#define NOMCX             // Modem Configuration Extensions

// mmsystem.h excludes
#define MMNODRV
//#define MMNOSOUND
//#define MMNOWAVE
//#define MMNOMIDI
#define MMNOAUX
#define MMNOMIXER
//#define MMNOTIMER
#define MMNOJOY
#define MMNOMCI
//#define MMNOMMIO
//#define MMNOMMSYSTEM

// mmreg.h excludes
#define NOMMIDS
//#define NONEWWAVE
#define NONEWRIFF
#define NOJPEGDIB
#define NONEWIC
#define NOBITMAP

#endif

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#define _USE_MATH_DEFINES

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#if MPT_COMPILER_MSVC
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS		// Define to disable the "This function or variable may be unsafe" warnings.
#endif
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES			1
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT	1
#ifndef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif
#endif



#if MPT_COMPILER_MSVC

#pragma warning(disable:4355) // 'this' : used in base member initializer list

// happens for immutable classes (i.e. classes containing const members)
#pragma warning(disable:4512) // assignment operator could not be generated

#if MPT_MSVC_AT_LEAST(2012,0) && MPT_MSVC_BEFORE(2013,0)
#pragma warning(disable:4250) // 'mpt::fstream' : inherits 'std::basic_istream<_Elem,_Traits>::std::basic_istream<_Elem,_Traits>::_Add_vtordisp1' via dominance
#endif

#pragma warning(error:4309) // Treat "truncation of constant value"-warning as error.

#endif // MPT_COMPILER_MSVC



#if MPT_OS_WINDOWS
#if (_WIN32_WINNT < 0x0601) && !defined(NO_MEDIAFOUNDATION)
#define NO_MEDIAFOUNDATION // MediaFoundation requires Windows 7
#endif
#endif

