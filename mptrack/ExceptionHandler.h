/*
 * ExceptionHandler.h
 * ------------------
 * Purpose: Code for handling crashes (unhandled exceptions) in OpenMPT.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

//====================
class ExceptionHandler
//====================
{
public:
	
	// Call this in the main thread to activate unhandled exception filtering for this thread.
	static void RegisterMainThread() { ::SetUnhandledExceptionFilter(UnhandledExceptionFilterMain); };
	// Call this in the audio thread to activate unhandled exception filtering for this thread.
	static void RegisterAudioThread() { ::SetUnhandledExceptionFilter(UnhandledExceptionFilterAudio); };
	// Call this in the notification thread to activate unhandled exception filtering for this thread.
	static void RegisterNotifyThread() { ::SetUnhandledExceptionFilter(UnhandledExceptionFilterNotify); };

protected:

	static LONG WINAPI UnhandledExceptionFilterMain(_EXCEPTION_POINTERS *pExceptionInfo);
	static LONG WINAPI UnhandledExceptionFilterAudio(_EXCEPTION_POINTERS *pExceptionInfo);
	static LONG WINAPI UnhandledExceptionFilterNotify(_EXCEPTION_POINTERS *pExceptionInfo);
	static LONG UnhandledExceptionFilter(_EXCEPTION_POINTERS *pExceptionInfo, const CString threadName);

};
