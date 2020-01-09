/*
 * MPTrackUtil.cpp
 * ---------------
 * Purpose: Various useful utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "MPTrackUtil.h"

OPENMPT_NAMESPACE_BEGIN


mpt::const_byte_span GetResource(LPCTSTR lpName, LPCTSTR lpType)
{
	HINSTANCE hInstance = AfxGetInstanceHandle();
	HRSRC hRsrc = FindResource(hInstance, lpName, lpType); 
	if(hRsrc == NULL)
	{
		return mpt::const_byte_span();
	}
	HGLOBAL hGlob = LoadResource(hInstance, hRsrc);
	if(hGlob == NULL)
	{
		return mpt::const_byte_span();
	}
	return mpt::const_byte_span(mpt::void_cast<const std::byte *>(LockResource(hGlob)), SizeofResource(hInstance, hRsrc));
	// no need to call FreeResource(hGlob) or free hRsrc, according to MSDN
}


// Returns WinAPI error message corresponding to error code returned by GetLastError().
CString GetErrorMessage(DWORD nErrorCode)
{
	LPTSTR lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		nErrorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);
	if(!lpMsgBuf)
	{
		if(GetLastError() == ERROR_NOT_ENOUGH_MEMORY)
		{
			MPT_EXCEPTION_THROW_OUT_OF_MEMORY();
		}
		return {};
	}
	CString msg;
	try
	{
		msg = lpMsgBuf;
	} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
	{
		LocalFree(lpMsgBuf);
		MPT_EXCEPTION_RETHROW_OUT_OF_MEMORY(e);
	}
	LocalFree(lpMsgBuf);
	return msg;
}


OPENMPT_NAMESPACE_END
