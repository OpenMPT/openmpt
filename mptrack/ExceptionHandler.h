/*
 * ExceptionHandler.h
 * ------------------
 * Purpose: Code for handling crashes (unhandled exceptions) in OpenMPT.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN

//====================
class ExceptionHandler
//====================
{
public:
	static bool fullMemDump;
	static bool stopSoundDeviceOnCrash;
	static bool stopSoundDeviceBeforeDump;

	// Call this to activate unhandled exception filtering.
	static void Register() { ::SetUnhandledExceptionFilter(UnhandledExceptionFilter); };

protected:

	static LONG WINAPI UnhandledExceptionFilter(_EXCEPTION_POINTERS *pExceptionInfo);

};

void DebugInjectCrash();

void DebugTraceDump();

OPENMPT_NAMESPACE_END
