#include "stdafx.h"
#include "misc_util.h"
#include <ctime>

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


// Returns error message corresponding to error code returned by GetLastError().
CString GetErrorMessage(DWORD nErrorCode)
//---------------------------------------
{
	const size_t nBufferSize = 256;
	CString sMsg;
	LPTSTR pszBuf = sMsg.GetBuffer(nBufferSize);

    FormatMessage(  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					nErrorCode,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					pszBuf,
					nBufferSize,
					NULL );

	sMsg.ReleaseBuffer();

	return sMsg;
}


std::basic_string<TCHAR> Util::sdTime::GetDateTimeStr()
{
	time_t t;
	std::time(&t);
	return _tctime(&t);
}

time_t Util::sdTime::MakeGmTime(tm& timeUtc)
{
#if MSVC_VER_2003
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

