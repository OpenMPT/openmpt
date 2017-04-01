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
	static bool delegateToWindowsHandler;
	static bool debugExceptionHandler;

	// these are expected to be set on startup and never changed again
	static bool useAnyCrashHandler;
	static bool useImplicitFallbackSEH;
	static bool useExplicitSEH;
	static bool handleStdTerminate;
	static bool handleStdUnexpected;
	static bool handleMfcExceptions;

	// Call this to activate unhandled exception filtering
	// and register a std::terminate_handler.
	static void Register();
	static void ConfigureSystemHandler();
	static void UnconfigureSystemHandler();
	static void Unregister();

public:

	static LONG WINAPI UnhandledExceptionFilterContinue(_EXCEPTION_POINTERS *pExceptionInfo);
	static LONG WINAPI ExceptionFilter(_EXCEPTION_POINTERS *pExceptionInfo);

	static void UnhandledMFCException(CException * e, const MSG * pMsg);

};

void DebugInjectCrash();

void DebugTraceDump();

OPENMPT_NAMESPACE_END
