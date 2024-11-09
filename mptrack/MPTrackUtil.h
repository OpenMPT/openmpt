/*
 * MPTrackUtil.h
 * -------------
 * Purpose: Various useful utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"


#include <string>


OPENMPT_NAMESPACE_BEGIN


bool CreateShellFolderLink(const mpt::PathString &path, const mpt::PathString &target, const mpt::ustring &description = mpt::ustring());

bool CreateShellFileLink(const mpt::PathString &path, const mpt::PathString &target, const mpt::ustring &description = mpt::ustring());


/*
 * Gets resource as raw byte data.
 * [in] lpName and lpType: parameters passed to FindResource().
 * Return: span representing the resource data, valid as long as hInstance is valid.
 */
mpt::const_byte_span GetResource(LPCTSTR lpName, LPCTSTR lpType);


CString LoadResourceString(UINT nID);


namespace Util
{
	inline DWORD64 GetTickCount64()
	{
#if MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
		return ::GetTickCount64();
#else
		return ::GetTickCount();
#endif
	}
}


OPENMPT_NAMESPACE_END
