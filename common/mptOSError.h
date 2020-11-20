/*
 * mptOSError.h
 * ------------
 * Purpose: OS-specific error message handling.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"



#include "mptBaseMacros.h"
#include "mptException.h"
#include "mptString.h"
#include "mptStringFormat.h"

#if defined(MODPLUG_TRACKER)
#if MPT_OS_WINDOWS
#include <stdexcept>
#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER

#if defined(MODPLUG_TRACKER)
#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER



OPENMPT_NAMESPACE_BEGIN


#if defined(MODPLUG_TRACKER)
#if MPT_OS_WINDOWS

namespace mpt
{

namespace Windows
{


inline mpt::ustring GetErrorMessage(DWORD errorCode, HANDLE hModule = NULL)
{
	mpt::ustring message;
	void *lpMsgBuf = nullptr;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | (hModule ? FORMAT_MESSAGE_FROM_HMODULE : 0) | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		hModule,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);
	if(!lpMsgBuf)
	{
		DWORD e = GetLastError();
		if((e == ERROR_NOT_ENOUGH_MEMORY) || (e == ERROR_OUTOFMEMORY))
		{
			mpt::throw_out_of_memory();
		}
		return {};
	}
	try
	{
		message = mpt::ToUnicode(mpt::winstring((LPTSTR)lpMsgBuf));
	} catch(mpt::out_of_memory e)
	{
		LocalFree(lpMsgBuf);
		mpt::rethrow_out_of_memory(e);
	}
	LocalFree(lpMsgBuf);
	return message;
}


class Error
	: public std::runtime_error
{
public:
	Error(DWORD errorCode, HANDLE hModule = NULL)
		: std::runtime_error(mpt::ToCharset(mpt::CharsetException, MPT_UFORMAT("Windows Error: 0x{}: {}")(mpt::ufmt::hex0<8>(errorCode), GetErrorMessage(errorCode, hModule))))
	{
		return;
	}
};


inline HANDLE CheckFileHANDLE(HANDLE handle)
{
	if(handle == INVALID_HANDLE_VALUE)
	{
		DWORD err = ::GetLastError();
		if((err == ERROR_NOT_ENOUGH_MEMORY) || (err == ERROR_OUTOFMEMORY))
		{
			mpt::throw_out_of_memory();
		}
		throw Windows::Error(err);
	}
	return handle;
}


inline HANDLE CheckHANDLE(HANDLE handle)
{
	if(handle == NULL)
	{
		DWORD err = ::GetLastError();
		if((err == ERROR_NOT_ENOUGH_MEMORY) || (err == ERROR_OUTOFMEMORY))
		{
			mpt::throw_out_of_memory();
		}
		throw Windows::Error(err);
	}
	return handle;
}


inline void CheckBOOL(BOOL result)
{
	if(result == FALSE)
	{
		DWORD err = ::GetLastError();
		if((err == ERROR_NOT_ENOUGH_MEMORY) || (err == ERROR_OUTOFMEMORY))
		{
			mpt::throw_out_of_memory();
		}
		throw Windows::Error(err);
	}
}


} // namespace Windows

} // namespace mpt

#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER



OPENMPT_NAMESPACE_END
