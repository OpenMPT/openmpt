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

#if defined(_MSC_VER)

#pragma warning( disable : 4996 ) // 'foo': The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _foo. See online help for details.

#define MPT_WITH_FLAC
#define MPT_WITH_PORTAUDIO
#define MPT_WITH_ZLIB

#define FLAC__NO_DLL

#endif // _MSC_VER

#ifndef LIBOPENMPT_ALPHA_WARNING_SEEN_AND_I_KNOW_WHAT_I_AM_DOING
#define LIBOPENMPT_ALPHA_WARNING_SEEN_AND_I_KNOW_WHAT_I_AM_DOING
#endif

#define OPENMPT123_VERSION_STRING "0.1"

#endif // OPENMPT123_CONFIG_HPP
