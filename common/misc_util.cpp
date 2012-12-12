/*
 * misc_util.cpp
 * -------------
 * Purpose: Various useful utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "misc_util.h"
#include <ctime>

#ifdef MODPLUG_TRACKER

/*
 * Loads resource.
 * [in] lpName and lpType: parameters passed to FindResource().
 * [out] pData: Pointer to loaded resource data, nullptr if load not successful.
 * [out] nSize: Size of the data in bytes, zero if load not succesfull.
 * [out] hglob: HGLOBAL returned by LoadResource-function.
 * Return: pData.
 */
LPCCH LoadResource(LPCTSTR lpName, LPCTSTR lpType, LPCCH& pData, size_t& nSize, HGLOBAL& hglob)
//---------------------------------------------------------------------------------------------
{
	pData = nullptr;
	nSize = 0;
	hglob = nullptr;
	HINSTANCE hInstance = AfxGetInstanceHandle();
	HRSRC hrsrc = FindResource(hInstance, lpName, lpType); 
	if (hrsrc != NULL)
	{
		hglob = LoadResource(hInstance, hrsrc);
		if (hglob != NULL)
		{
			pData = reinterpret_cast<const char*>(LockResource(hglob));
			nSize = SizeofResource(hInstance, hrsrc);
		}
	}
	return pData;
}


// Returns WinAPI error message corresponding to error code returned by GetLastError().
CString GetErrorMessage(DWORD nErrorCode)
//---------------------------------------
{
	LPVOID lpMsgBuf;

	FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					nErrorCode,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR)&lpMsgBuf,
					0,
					NULL );

	CString msg = (LPTSTR)lpMsgBuf;
	LocalFree(lpMsgBuf);

	return msg;
}
#endif // MODPLUG_TRACKER


std::basic_string<TCHAR> Util::sdTime::GetDateTimeStr()
{
	time_t t;
	std::time(&t);
	return _tctime(&t);
}

time_t Util::sdTime::MakeGmTime(tm& timeUtc)
{
#if (_MSC_VER < MSVC_VER_2005)
	// VC++ 2003 doesn't have _mkgmtime
	// This does not seem to work properly with DST time zones sometimes - if that's of any concern for you, please upgrade your compiler :)
	TIME_ZONE_INFORMATION tzi;
	GetTimeZoneInformation(&tzi);
	const time_t timeUtcTimeT = mktime(&timeUtc) - tzi.Bias * 60;
#else
	const time_t timeUtcTimeT = _mkgmtime(&timeUtc);
#endif
	return timeUtcTimeT;
}

