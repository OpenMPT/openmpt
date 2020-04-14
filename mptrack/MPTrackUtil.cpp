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


OPENMPT_NAMESPACE_END
