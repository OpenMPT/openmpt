/*
 * Loaders.h
 * ---------
 * Purpose: Provide common functions for module loaders
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 *
 */

#include "Sndfile.h"

// Execute "action" if "request_bytes" bytes cannot be read from stream at position "position"
#define ASSERT_CAN_READ_PROTOTYPE(position, length, request_bytes, action) \
	if( position > length || request_bytes > length - position) action;

// "Default" macro for checking if x bytes can be read from stream.
#define ASSERT_CAN_READ(x) ASSERT_CAN_READ_PROTOTYPE(dwMemPos, dwMemLength, x, return false);
