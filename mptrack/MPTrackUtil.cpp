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
#include "Mptrack.h"
#include "../common/misc_util.h"

#include <io.h> // for _taccess
#include <time.h>


OPENMPT_NAMESPACE_BEGIN

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
	LPTSTR lpMsgBuf = NULL;

	FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					nErrorCode,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR)&lpMsgBuf,
					0,
					NULL );

	CString msg = lpMsgBuf;
	LocalFree(lpMsgBuf);

	return msg;
}


OPENMPT_NAMESPACE_END
