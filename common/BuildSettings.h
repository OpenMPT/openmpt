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



// set windows version early so that we can deduce dependencies from SDK version

#if MPT_OS_WINDOWS

#if MPT_COMPILER_MSVC || MPT_COMPILER_MSVCCLANGC2

#if defined(MPT_BUILD_TARGET_2000)

#if defined(_M_X64)
#define _WIN32_WINNT 0x0502 // _WIN32_WINNT_WS03
#else // !_M_X64
#define _WIN32_WINNT 0x0500 // _WIN32_WINNT_WIN2000
#endif // _M_X64

#elif defined(MPT_BUILD_TARGET_XP)

#if defined(_M_X64)
#define _WIN32_WINNT 0x0502 // _WIN32_WINNT_WS03
#else // !_M_X64
#define _WIN32_WINNT 0x0501 // _WIN32_WINNT_WINXP
#endif // _M_X64

#else // MPT_BUILD_TARGET

#if MPT_MSVC_BEFORE(2010,0)
#define _WIN32_WINNT 0x0600 // _WIN32_WINNT_VISTA
#else
#define _WIN32_WINNT 0x0601 // _WIN32_WINNT_WIN7
#endif

#endif // MPT_BUILD_TARGET

#else // MPT_COMPILER

#define _WIN32_WINNT 0x0501 // _WIN32_WINNT_WINXP

#endif // MPT_COMPILER

#define WINVER       _WIN32_WINNT

#endif // MPT_OS_WINDOWS



#if defined(MODPLUG_TRACKER) && defined(LIBOPENMPT_BUILD)
#error "either MODPLUG_TRACKER or LIBOPENMPT_BUILD has to be defined"
#elif defined(MODPLUG_TRACKER)
// nothing
#elif defined(LIBOPENMPT_BUILD)
// nothing
#else
#error "either MODPLUG_TRACKER or LIBOPENMPT_BUILD has to be defined"
#endif // MODPLUG_TRACKER || LIBOPENMPT_BUILD



// wrapper for autoconf macros

#if defined(HAVE_CONFIG_H)

#include "config.h"

// Fixup dependencies which are currently not used in libopenmpt itself
#ifdef MPT_WITH_FLAC
#undef MPT_WITH_FLAC
#endif

#endif // HAVE_CONFIG_H



// Dependencies from the MSVC build system
#if MPT_COMPILER_MSVC || MPT_COMPILER_MSVCCLANGC2

// This section defines which dependencies are available when building with
// MSVC. Other build systems provide MPT_WITH_* macros via command-line or other
// means.
// OpenMPT and libopenmpt should compile and run successfully (albeit with
// reduced functionality) with any or all dependencies missing/disabled.
// The defaults match the bundled third-party libraries with the addition of
// ASIO and VST SDKs.

#if defined(MODPLUG_TRACKER)

// OpenMPT-only dependencies
#define MPT_WITH_ASIO
#define MPT_WITH_DSOUND
#define MPT_WITH_LHASA
#define MPT_WITH_MINIZIP
#define MPT_WITH_OPUS
#define MPT_WITH_OPUSFILE
#define MPT_WITH_PORTAUDIO
#define MPT_WITH_SMBPITCHSHIFT
#define MPT_WITH_UNRAR
#define MPT_WITH_VORBISENC

// OpenMPT and libopenmpt dependencies (not for openmp123, player plugins or examples)
//#define MPT_WITH_DL
#define MPT_WITH_FLAC
//#define MPT_WITH_ICONV
//#define MPT_WITH_LTDL
#if MPT_OS_WINDOWS
#if (_WIN32_WINNT >= 0x0601)
#define MPT_WITH_MEDIAFOUNDATION
#endif
#endif
//#define MPT_WITH_MINIMP3
//#define MPT_WITH_MINIZ
//#define MPT_WITH_MPG123
#define MPT_WITH_OGG
//#define MPT_WITH_STBVORBIS
//#define MPT_WITH_UNMO3
#define MPT_WITH_VORBIS
#define MPT_WITH_VORBISFILE
#define MPT_WITH_ZLIB

#endif // MODPLUG_TRACKER

#if defined(LIBOPENMPT_BUILD)

// OpenMPT and libopenmpt dependencies (not for openmp123, player plugins or examples)
#if defined(LIBOPENMPT_BUILD_SMALL)

//#define MPT_WITH_DL
//#define MPT_WITH_FLAC
//#define MPT_WITH_ICONV
//#define MPT_WITH_LTDL
#if MPT_OS_WINDOWS
#if (_WIN32_WINNT >= 0x0601)
#define MPT_WITH_MEDIAFOUNDATION
#endif
#endif
//#define MPT_WITH_MINIMP3
#define MPT_WITH_MINIZ
//#define MPT_WITH_MPG123
//#define MPT_WITH_OGG
#define MPT_WITH_STBVORBIS
//#define MPT_WITH_UNMO3
//#define MPT_WITH_VORBIS
//#define MPT_WITH_VORBISFILE
//#define MPT_WITH_ZLIB

#else // !LIBOPENMPT_BUILD_SMALL

//#define MPT_WITH_DL
//#define MPT_WITH_FLAC
//#define MPT_WITH_ICONV
//#define MPT_WITH_LTDL
#if MPT_OS_WINDOWS
#if (_WIN32_WINNT >= 0x0601)
#define MPT_WITH_MEDIAFOUNDATION
#endif
#endif
//#define MPT_WITH_MINIMP3
//#define MPT_WITH_MINIZ
//#define MPT_WITH_MPG123
#define MPT_WITH_OGG
//#define MPT_WITH_STBVORBIS
//#define MPT_WITH_UNMO3
#define MPT_WITH_VORBIS
#define MPT_WITH_VORBISFILE
#define MPT_WITH_ZLIB

#endif // LIBOPENMPT_BUILD_SMALL

#endif // LIBOPENMPT_BUILD

#endif // MPT_COMPILER_MSVC || MPT_COMPILER_MSVCCLANGC2



#if defined(MODPLUG_TRACKER)

// Enable built-in test suite.
#ifdef _DEBUG
#define ENABLE_TESTS
#endif

// Disable any file saving functionality (not really useful except for the player library)
//#define MODPLUG_NO_FILESAVE

// Disable any debug logging
//#define NO_LOGGING
#if !defined(_DEBUG) && !defined(MPT_BUILD_WINESUPPORT)
#define MPT_LOG_GLOBAL_LEVEL_STATIC
#define MPT_LOG_GLOBAL_LEVEL 0
#endif

// Disable all runtime asserts
#if !defined(_DEBUG) && !defined(MPT_BUILD_WINESUPPORT)
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

// Support mpt::ChartsetLocale
#define MPT_ENABLE_CHARSET_LOCALE

// Use inline assembly
#define ENABLE_ASM

// Disable unarchiving support
//#define NO_ARCHIVE_SUPPORT

// Disable the built-in reverb effect
//#define NO_REVERB

// Disable built-in miscellaneous DSP effects (surround, mega bass, noise reduction) 
//#define NO_DSP

// Disable the built-in equalizer.
//#define NO_EQ

// Disable the built-in automatic gain control
//#define NO_AGC

// Define to build without VST plugin support; makes build possible without VST SDK.
//#define NO_VST

// Define to build without DMO plugin support
//#define NO_DMO

// (HACK) Define to build without any plugin support
//#define NO_PLUGINS

// Enable dynamic loading of libmpg123
#define MPT_ENABLE_MPG123_DYNBIND

// Enable dynamic loading of un4seen unmo3
#define MPT_ENABLE_UNMO3_DYNBIND

// Enable built-in MO3 decoder
#define MPT_ENABLE_MO3_BUILTIN

// Do not build libopenmpt C api
#define NO_LIBOPENMPT_C

// Do not build libopenmpt C++ api
#define NO_LIBOPENMPT_CXX

#endif // MODPLUG_TRACKER



#if defined(LIBOPENMPT_BUILD)

#if (defined(_DEBUG) || defined(DEBUG)) && !defined(MPT_BUILD_DEBUG)
#define MPT_BUILD_DEBUG
#endif

#if defined(LIBOPENMPT_BUILD_TEST)
#define ENABLE_TESTS
#else
#define MODPLUG_NO_FILESAVE
#endif
#if defined(MPT_BUILD_ANALZYED) || defined(MPT_BUILD_CHECKED) || defined(ENABLE_TESTS)
// enable asserts
#else
#define NO_ASSERTS
#endif
//#define NO_LOGGING
#define MPT_FILEREADER_STD_ISTREAM
#define MPT_FILEREADER_STD_ISTREAM_SEEKABLE
#define MPT_FILEREADER_CALLBACK_STREAM
//#define MPT_EXTERNAL_SAMPLES
#if defined(ENABLE_TESTS) || defined(MPT_BUILD_HACK_ARCHIVE_SUPPORT)
#define MPT_ENABLE_CHARSET_LOCALE
#else
//#define MPT_ENABLE_CHARSET_LOCALE
#endif
// Do not use inline asm in library builds. There is just about no codepath which would use it anyway.
//#define ENABLE_ASM
#if defined(MPT_BUILD_HACK_ARCHIVE_SUPPORT)
//#define NO_ARCHIVE_SUPPORT
#else
#define NO_ARCHIVE_SUPPORT
#endif
#define NO_REVERB
#define NO_DSP
#define NO_EQ
#define NO_AGC
#define NO_VST
#if MPT_OS_WINDOWS && MPT_COMPILER_MSVC
//#define NO_DMO
#else
#define NO_DMO
#endif
//#define NO_PLUGINS
#if MPT_OS_WINDOWS || defined(MPT_WITH_LTDL) || defined(MPT_WITH_DL)
#if !defined(MPT_WITH_MPG123)
#define MPT_ENABLE_MPG123_DYNBIND
#endif
//#if !(defined(MPT_WITH_MPG123) || defined(MPT_ENABLE_MPG123_DYNBIND) || defined(MPT_WITH_MINIMP3)) || defined(MPT_WITH_MEDIAFOUNDATION)) || !((defined(MPT_WITH_OGG) && defined(MPT_WITH_VORBIS) && defined(MPT_WITH_VORBISFILE)) || defined(MPT_WITH_STBVORBIS))
#if !(defined(MPT_WITH_MPG123) || defined(MPT_WITH_MINIMP3)) || !((defined(MPT_WITH_OGG) && defined(MPT_WITH_VORBIS) && defined(MPT_WITH_VORBISFILE)) || defined(MPT_WITH_STBVORBIS))
#if !defined(MPT_WITH_UNMO3)
#define MPT_ENABLE_UNMO3_DYNBIND
#endif
#endif
#endif // MPT_OS_WINDOWS || defined(MPT_WITH_LTDL) || defined(MPT_WITH_DL)
#if ((defined(MPT_WITH_MPG123) || defined(MPT_ENABLE_MPG123_DYNBIND) || defined(MPT_WITH_MINIMP3)) || defined(MPT_WITH_MEDIAFOUNDATION)) && ((defined(MPT_WITH_OGG) && defined(MPT_WITH_VORBIS) && defined(MPT_WITH_VORBISFILE)) || defined(MPT_WITH_STBVORBIS))
#define MPT_ENABLE_MO3_BUILTIN
#endif
//#define NO_LIBOPENMPT_C
//#define NO_LIBOPENMPT_CXX

#endif // LIBOPENMPT_BUILD



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
	//#ifndef MPT_LOCALE_ASSUME_CHARSET
	//#define MPT_LOCALE_ASSUME_CHARSET CharsetUTF8
	//#endif

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
	#define MPT_ENABLE_U8STRING 1

#else

	#define MPT_ENABLE_U8STRING 0

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

#if defined(MPT_BUILD_TARGET_XP)
// Also support Wine 1.6 in addiion to Windows XP
#ifndef MPT_QUIRK_NO_CPP_THREAD
#define MPT_QUIRK_NO_CPP_THREAD
#endif
#endif

#if defined(MPT_BUILD_ANALYZED) || defined(MPT_BUILD_CHECKED) 
#ifdef NO_ASSERTS
#undef NO_ASSERTS // static or dynamic analyzers want assertions on
#endif
#endif

#if !MPT_COMPILER_MSVC && defined(ENABLE_ASM)
#undef ENABLE_ASM // inline assembly requires MSVC compiler
#endif

#if defined(ENABLE_ASM)
#if MPT_COMPILER_MSVC && defined(_M_IX86)

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

#elif MPT_COMPILER_MSVC && defined(_M_X64)

// Generate general x64 inline assembly / intrinsics.
#define ENABLE_X64
// Generate inline assembly using SSE2 instructions (only used when the CPU supports it).
#define ENABLE_SSE2
// Generate inline assembly using SSE3 instructions (only used when the CPU supports it).
#define ENABLE_SSE3
// Generate inline assembly using SSE4 instructions (only used when the CPU supports it).
#define ENABLE_SSE4

#endif // arch
#endif // ENABLE_ASM

#if !defined(ENABLE_MMX) && !defined(NO_REVERB)
#define NO_REVERB // reverb requires mmx
#endif

#if !defined(ENABLE_X86) && !defined(NO_DSP)
#define NO_DSP // DSP requires x86 inline asm
#endif

#if defined(ENABLE_TESTS) && defined(MODPLUG_NO_FILESAVE)
#undef MODPLUG_NO_FILESAVE // tests recommend file saving
#endif

#if defined(MPT_WITH_ZLIB) && defined(MPT_WITH_MINIZ)
// Only one deflate implementation should be used. Prefer zlib.
#undef MPT_WITH_MINIZ
#endif

#if !MPT_OS_WINDOWS && defined(MPT_WITH_MEDIAFOUNDATION)
#undef MPT_WITH_MEDIAFOUNDATION // MediaFoundation requires Windows
#endif

#if MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0) && defined(MPT_WITH_MEDIAFOUNDATION)
#undef MPT_WITH_MEDIAFOUNDATION // MediaFoundation requires a modern SDK
#endif

#if defined(MPT_WITH_MEDIAFOUNDATION) && !defined(MPT_ENABLE_TEMPFILE)
#define MPT_ENABLE_TEMPFILE
#endif

#if defined(MODPLUG_TRACKER) && !defined(MPT_ENABLE_TEMPFILE)
#define MPT_ENABLE_TEMPFILE
#endif

#if !defined(MPT_CHARSET_WIN32) && !defined(MPT_CHARSET_ICONV) && !defined(MPT_CHARSET_CODECVTUTF8) && !defined(MPT_CHARSET_INTERNAL)
#define MPT_CHARSET_INTERNAL
#endif

#if defined(MODPLUG_TRACKER) && !defined(MPT_ENABLE_DYNBIND)
#define MPT_ENABLE_DYNBIND // Tracker requires dynamic library loading for export codecs
#endif

#if defined(MPT_ENABLE_MPG123_DYNBIND) && !defined(MPT_ENABLE_DYNBIND)
#define MPT_ENABLE_DYNBIND // mpg123 is loaded dynamically
#endif

#if (defined(MPT_WITH_MPG123) || defined(MPT_ENABLE_MPG123_DYNBIND) || defined(MPT_WITH_MINIMP3)) && !defined(MPT_ENABLE_MP3_SAMPLES)
#define MPT_ENABLE_MP3_SAMPLES
#endif

#if defined(MPT_WITH_UNMO3) || defined(MPT_ENABLE_UNMO3_DYNBIND) || defined(MPT_ENABLE_MO3_BUILTIN)
#define MPT_ENABLE_MO3
#endif

#if defined(MPT_ENABLE_UNMO3_DYNBIND) && !defined(MPT_ENABLE_DYNBIND)
#define MPT_ENABLE_DYNBIND // unmo3 is loaded dynamically
#endif

#if defined(ENABLE_TESTS)
#define MPT_ENABLE_FILEIO // Test suite requires PathString for file loading.
#endif

#if !MPT_OS_WINDOWS && !defined(MPT_FILEREADER_STD_ISTREAM)
#define MPT_FILEREADER_STD_ISTREAM // MMAP is only supported on Windows
#endif

#if defined(MODPLUG_TRACKER) && !defined(MPT_ENABLE_FILEIO)
#define MPT_ENABLE_FILEIO // Tracker requires disk file io
#endif

#if defined(MODPLUG_TRACKER) && !defined(MPT_ENABLE_THREAD)
#define MPT_ENABLE_THREAD // Tracker requires threads
#endif

#if defined(MODPLUG_TRACKER) && !defined(MPT_ENABLE_ATOMIC)
#define MPT_ENABLE_ATOMIC // Tracker requires threads
#endif

#if defined(MPT_EXTERNAL_SAMPLES) && !defined(MPT_ENABLE_FILEIO)
#define MPT_ENABLE_FILEIO // External samples require disk file io
#endif

#if !defined(MODPLUG_NO_FILESAVE) && !defined(MPT_ENABLE_FILEIO_STDIO)
#define MPT_ENABLE_FILEIO_STDIO // file saving requires FILE*
#endif

#if defined(NO_PLUGINS)
// Any plugin type requires NO_PLUGINS to not be defined.
#define NO_VST
#define NO_DMO
#endif



#if defined(MODPLUG_TRACKER) && !defined(MPT_BUILD_WINESUPPORT) && !defined(MPT_BUILD_WINESUPPORT_WRAPPER)
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



// platform configuration

#if MPT_OS_WINDOWS

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

#endif // MPT_OS_WINDOWS



// stdlib configuration

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#define _USE_MATH_DEFINES

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif



// compiler configuration

#if MPT_COMPILER_MSVC

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS		// Define to disable the "This function or variable may be unsafe" warnings.
#endif
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES			1
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT	1
#ifndef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif

#ifndef NO_WARN_MBCS_MFC_DEPRECATION
#define NO_WARN_MBCS_MFC_DEPRECATION
#endif

#pragma warning(disable:4355) // 'this' : used in base member initializer list

// happens for immutable classes (i.e. classes containing const members)
#pragma warning(disable:4512) // assignment operator could not be generated

#if MPT_MSVC_AT_LEAST(2012,0) && MPT_MSVC_BEFORE(2013,0)
#pragma warning(disable:4250) // 'mpt::fstream' : inherits 'std::basic_istream<_Elem,_Traits>::std::basic_istream<_Elem,_Traits>::_Add_vtordisp1' via dominance
#endif

#pragma warning(error:4309) // Treat "truncation of constant value"-warning as error.

#ifdef MPT_BUILD_ANALYZED
// Disable Visual Studio static analyzer warnings that generate too many false positives in VS2010.
//#pragma warning(disable:6246)
//#pragma warning(disable:6262)
#pragma warning(disable:6326) // Potential comparison of a constant with another constant
//#pragma warning(disable:6385)
//#pragma warning(disable:6386)
#endif // MPT_BUILD_ANALYZED

#endif // MPT_COMPILER_MSVC

#if MPT_COMPILER_MSVCCLANGC2

#if MPT_OS_WINDOWS
// As Clang defines __STDC__ 1, Windows headers will use named union fields. The MediaFoundation headers do not support this, though.
// Clang supports nameless union fields just fine, and luckily there is a way to override the Windows headers behaviour.
#define _FORCENAMELESSUNION
#endif // MPT_OS_WINDOWS

#endif // MPT_COMPILER_MSVCCLANGC2



// third-party library configuration

#ifdef MPT_WITH_FLAC
#ifdef MPT_BUILD_MSVC_STATIC
#define FLAC__NO_DLL
#endif
#endif

#ifdef MPT_WITH_MINIZ
#define MINIZ_HEADER_FILE_ONLY
#endif

#ifdef MPT_WITH_SMBPITCHSHIFT
#ifdef MPT_BUILD_MSVC_SHARED
#define SMBPITCHSHIFT_USE_DLL
#endif
#endif

#ifdef MPT_WITH_STBVORBIS
#define STB_VORBIS_HEADER_ONLY
#ifndef STB_VORBIS_NO_PULLDATA_API
#define STB_VORBIS_NO_PULLDATA_API
#endif
#ifndef STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_STDIO
#endif
#endif

#ifdef MPT_WITH_VORBISFILE
#ifndef OV_EXCLUDE_STATIC_CALLBACKS
#define OV_EXCLUDE_STATIC_CALLBACKS
#endif
#endif

#ifdef MPT_WITH_ZLIB
#ifdef MPT_BUILD_MSVC_SHARED
#define ZLIB_DLL
#endif
#endif

