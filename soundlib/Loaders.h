/*
 * Loaders.h
 * ---------
 * Purpose: Common functions for module loaders
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "../common/misc_util.h"
#include "../common/FileReader.h"
#include "Sndfile.h"
#include "SampleIO.h"

OPENMPT_NAMESPACE_BEGIN

// Macros to create magic bytes in little-endian format
#define MAGIC4LE(a, b, c, d)	static_cast<uint32>((static_cast<uint8>(d) << 24) | (static_cast<uint8>(c) << 16) | (static_cast<uint8>(b) << 8) | static_cast<uint8>(a))
#define MAGIC2LE(a, b)		static_cast<uint16>((static_cast<uint8>(b) << 8) | static_cast<uint8>(a))
// Macros to create magic bytes in big-endian format
#define MAGIC4BE(a, b, c, d)	static_cast<uint32>((static_cast<uint8>(a) << 24) | (static_cast<uint8>(b) << 16) | (static_cast<uint8>(c) << 8) | static_cast<uint8>(d))
#define MAGIC2BE(a, b)		static_cast<uint16>((static_cast<uint8>(a) << 8) | static_cast<uint8>(b))

OPENMPT_NAMESPACE_END
