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

enum VendorSpecificOpCodes
{
	kUpdateEffectStruct = 0,
	kUpdateProcessingBuffer,
	kGetWrapperPointer,
	kUpdateEventMemName,
	kCloseOldProcessingMemory,

	kVendorOpenMPT = CCONST('O', 'M', 'P', 'T'),
};
