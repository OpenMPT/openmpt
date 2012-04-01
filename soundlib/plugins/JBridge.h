/*
 * JBridge.h
 * ---------
 * Purpose: Native support for the JBridge VST plugin bridge.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

namespace JBridge
{
	AEffect *LoadBridgedPlugin(audioMasterCallback audioMaster, const char *pluginPath);
}
