/*
 * JBridge.h
 * ---------
 * Purpose: Native support for the JBridge VST plugin bridge.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#if !defined(NO_VST) && (defined(WIN32) || (defined(WINDOWS) && WINDOWS == 1))
#define ENABLE_JBRIDGE
#endif

namespace JBridge
{
#ifdef ENABLE_JBRIDGE
	AEffect *LoadBridgedPlugin(audioMasterCallback audioMaster, const char *pluginPath);
#else
	inline void *LoadBridgedPlugin(void *, const char *) { return nullptr; }
#endif // ENABLE_JBRIDGE
}
