// stdafx.cpp : source file that includes just the standard includes
//	mptrack.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#if (_MSC_VER == MSVC_VER_2008)

// Fix for VS2008 SP1 bloatage (http://tedwvc.wordpress.com/2011/04/16/static-mfc-code-bloat-problem-from-vc2010-is-now-in-vc2008-sp1security-fix/):

// this is our own local copy of the AfxLoadSystemLibraryUsingFullPath function
HMODULE AfxLoadSystemLibraryUsingFullPath(const WCHAR *pszLibrary)
{
	WCHAR wszLoadPath[MAX_PATH+1];
	if (::GetSystemDirectoryW(wszLoadPath, _countof(wszLoadPath)) == 0)
	{
		return NULL;
	}

	if (wszLoadPath[wcslen(wszLoadPath)-1] != L'\\')
	{
		if (wcscat_s(wszLoadPath, _countof(wszLoadPath), L"\\") != 0)
		{
			return NULL;
		}
	}

	if (wcscat_s(wszLoadPath, _countof(wszLoadPath), pszLibrary) != 0)
	{
		return NULL;
	}

	return(::AfxCtxLoadLibraryW(wszLoadPath));
}

#endif
