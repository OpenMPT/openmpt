/*
 * openmpt123_config.hpp
 * ---------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_CONFIG_HPP
#define OPENMPT123_CONFIG_HPP

#if defined(_WIN32)
#ifndef WIN32
#define WIN32
#endif
#endif // _WIN32

#if defined(WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#endif // WIN32

#if defined(WIN32)
#define MPT_WITH_MMIO
#endif // WIN32

#if defined(_MSC_VER)

#pragma warning( disable : 4996 ) // 'foo': The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _foo. See online help for details.

#define MPT_WITH_FLAC
#define MPT_WITH_PORTAUDIO
#define MPT_WITH_ZLIB

#define FLAC__NO_DLL

#endif // _MSC_VER

#if defined(MPT_WITH_PORTAUDIO) && defined(MPT_PORTAUDIO_CALLBACK)
#ifndef MPT_NEEDS_THREADS
#define MPT_NEEDS_THREADS
#endif
#endif

#if defined(MPT_WITH_SDL)
#ifndef MPT_NEEDS_THREADS
#define MPT_NEEDS_THREADS
#endif
#endif

#define OPENMPT123_VERSION_STRING "0.2"

#endif // OPENMPT123_CONFIG_HPP
