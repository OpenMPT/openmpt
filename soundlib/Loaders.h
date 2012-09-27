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
#include "FileReader.h"
#include "Sndfile.h"
#include "SampleIO.h"

// Execute "action" if "request_bytes" bytes cannot be read from stream at position "position"
// DEPRECATED. Use FileReader instead.
#define ASSERT_CAN_READ_PROTOTYPE(position, length, request_bytes, action) \
	if((position) > (length) || (request_bytes) > (length) - (position)) action;

// "Default" macro for checking if x bytes can be read from stream.
// DEPRECATED. Use FileReader instead.
#define ASSERT_CAN_READ(x) ASSERT_CAN_READ_PROTOTYPE(dwMemPos, dwMemLength, x, return false);
