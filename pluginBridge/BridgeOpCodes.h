/*
 * BridgeOpCodes.h
 * ---------------
 * Purpose: Various dispatch opcodes for communication between OpenMPT and its plugin bridge.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#define VST_FORCE_DEPRECATED 0
#include "../include/vstsdk2.4/pluginterfaces/vst2.x/aeffectx.h"

OPENMPT_NAMESPACE_BEGIN

enum VendorSpecificOpCodes
{
	// Explicitely tell the plugin bridge to update the AEffect struct cache
	kUpdateEffectStruct = 0,
	// Notify the host that it needs to resize its processing buffers
	kUpdateProcessingBuffer,
	// Returns the pointer to the bridge wrapper object for the current plugin
	kGetWrapperPointer,
	// Sets name for new shared memory object - name in ptr
	kUpdateEventMemName,
	kCloseOldProcessingMemory,
	// Cache program names - ptr points to 2x int32 containing the start and end program index - caches names in range [start, end[
	kCacheProgramNames,

	// Constant for identifying our vendor-specific opcodes
	kVendorOpenMPT = CCONST('O', 'M', 'P', 'T'),

	// Maximum length of cached program names
	kCachedProgramNameLength = 256,
};

OPENMPT_NAMESPACE_END
