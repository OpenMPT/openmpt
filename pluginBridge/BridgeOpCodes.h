/*
 * BridgeOpCodes.h
 * ---------------
 * Purpose: Various dispatch opcodes for communication between OpenMPT and its plugin bridge.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "../mptrack/plugins/VstDefinitions.h"

OPENMPT_NAMESPACE_BEGIN

enum VendorSpecificOpCodes : int32
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
	// Cache parameter information - ptr points to 2x int32 containing the start and end parameter index - caches info in range [start, end[
	kCacheParameterInfo,
	// Cache parameter values
	kBeginGetProgram,
	// Stop using parameter value cache
	kEndGetProgram,

	// Constant for identifying our vendor-specific opcodes
	kVendorOpenMPT = Vst::FourCC("OMPT"),

	// Maximum length of cached program names
	kCachedProgramNameLength = 256,
};

OPENMPT_NAMESPACE_END
