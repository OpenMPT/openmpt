/*
 * JBridge.cpp
 * -----------
 * Purpose: Native support for the JBridge VST plugin bridge.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 *          Bridge code borrowed from http://jstuff.wordpress.com/jbridge/how-to-add-direct-support-for-jbridge-in-your-host/
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include <pluginterfaces/vst2.x/aeffectx.h>
#include "JBridge.h"

#ifdef ENABLE_JBRIDGE

namespace JBridge
{

// Name of the proxy DLL to load
static const char *proxyRegKey = "Software\\JBridge";

#ifdef _M_X64
static const char *proxyRegVal = "Proxy64";	//use this for x64 builds
static_assert(sizeof(void *) == 8, "Wrong platform!");
#else
static const char *proxyRegVal = "Proxy32";	//use this for x86 builds
static_assert(sizeof(void *) == 4, "Wrong platform!");
#endif

// Typedef for BridgeMain proc
typedef AEffect * (*PFNBRIDGEMAIN)(audioMasterCallback audioMaster, const char *pluginPath);


//Check if it's a plugin_name.xx.dll
bool IsBootStrapDll(const char *path)
//-----------------------------------
{
	bool ret = false;

	HMODULE hModule = LoadLibrary(path);
	if(!hModule)
	{
		// Some error...
		return ret;
	}

	//Exported dummy function to identify this as a bootstrap dll.
	if(GetProcAddress(hModule, "JBridgeBootstrap"))
	{
		//it's a bootstrap dll
		ret = true;
	}

	FreeLibrary(hModule);

	return ret;
}


AEffect *LoadBridgedPlugin(audioMasterCallback audioMaster, const char *pluginPath)
//---------------------------------------------------------------------------------
{
	// Ignore JBridge bootstrap DLLs
	if(IsBootStrapDll(pluginPath))
	{
		return nullptr;
	}

	// Get path to JBridge proxy
	char proxyPath[MAX_PATH];
	proxyPath[0] = '\0';
	HKEY hKey;
	if(RegOpenKey(HKEY_LOCAL_MACHINE, proxyRegKey, &hKey) == ERROR_SUCCESS)
	{
		DWORD dw = sizeof(proxyPath);
		RegQueryValueEx(hKey, proxyRegVal, nullptr, nullptr, reinterpret_cast<LPBYTE>(proxyPath), &dw);
		RegCloseKey(hKey);
	}

	// Check key found and file exists
	if(proxyPath[0] == '\0')
	{
		//MessageBox(GetActiveWindow(), "Unable to locate proxy DLL", "Warning", MB_OK | MB_ICONHAND);
		return nullptr;
	}

	// Load proxy DLL
	HMODULE hModuleProxy = LoadLibrary(proxyPath);
	if(!hModuleProxy)
	{
		//MessageBox(GetActiveWindow(), "Failed to load proxy", "Warning", MB_OK | MB_ICONHAND);
		return nullptr;
	}

	// Get entry point
	PFNBRIDGEMAIN pfnBridgeMain = (PFNBRIDGEMAIN)GetProcAddress(hModuleProxy, "BridgeMain");
	if(pfnBridgeMain == nullptr)
	{
		FreeLibrary(hModuleProxy);
		//MessageBox(GetActiveWindow(), "BridgeMain entry point not found", "JBridge", MB_OK | MB_ICONHAND);
		return nullptr;
	}

	return pfnBridgeMain(audioMaster, pluginPath);
}

}

#endif // ENABLE_JBRIDGE
