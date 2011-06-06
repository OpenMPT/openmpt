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
	TIME_ZONE_INFORMATION tzi;
	GetTimeZoneInformation(&tzi);
	const time_t timeUtcTimeT = mktime(&timeUtc) - 60 * tzi.Bias;
#if (_MSC_VER >= MSVC_VER_2005) && defined(_DEBUG)
	ASSERT(timeUtcTimeT < 0 || timeUtcTimeT == _mkgmtime(&timeUtc));
#endif
	return timeUtcTimeT;
}

