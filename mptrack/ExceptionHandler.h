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
	
	static void RegisterMainThread();
	static void RegisterAudioThread();

protected:

	static LONG WINAPI UnhandledExceptionFilterMain(_EXCEPTION_POINTERS *pExceptionInfo);
	static LONG WINAPI UnhandledExceptionFilterAudio(_EXCEPTION_POINTERS *pExceptionInfo);
	static LONG UnhandledExceptionFilter(_EXCEPTION_POINTERS *pExceptionInfo, const CString threadName);

};

#endif // EXCEPTIONHANDLER_H
