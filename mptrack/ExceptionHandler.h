/*
 *
 * ExceptionHandler.h
 * ------------------
 * Purpose: Header file for crash handler.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */


#pragma once
#ifndef EXCEPTIONHANDLER_H
#define EXCEPTIONHANDLER_H

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

#endif // EXCEPTIONHANDLER_H
