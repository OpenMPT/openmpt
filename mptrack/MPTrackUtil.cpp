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


static bool CreateShellLink(const IID &type, const mpt::PathString &path, const mpt::PathString &target, const mpt::ustring &description)
{
	HRESULT hres = 0;
	IShellLink *psl = nullptr;
	hres = CoCreateInstance(type, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
	if(SUCCEEDED(hres))
	{
		IPersistFile *ppf = nullptr;
		psl->SetPath(target.AsNative().c_str()); 
		psl->SetDescription(mpt::ToWin(description).c_str());
		hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
		if(SUCCEEDED(hres))
		{
			hres = ppf->Save(path.ToWide().c_str(), TRUE);
			ppf->Release();
			ppf = nullptr;
		}
		psl->Release();
		psl = nullptr;
	}
	return SUCCEEDED(hres);
}


bool CreateShellFolderLink(const mpt::PathString &path, const mpt::PathString &target, const mpt::ustring &description)
{
	return CreateShellLink(CLSID_FolderShortcut, path, target, description);
}


bool CreateShellFileLink(const mpt::PathString &path, const mpt::PathString &target, const mpt::ustring &description)
{
	return CreateShellLink(CLSID_ShellLink, path, target, description);
}


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


CString LoadResourceString(UINT nID)
{
	CString str;
	BOOL resourceLoaded = str.LoadString(nID);
	MPT_ASSERT(resourceLoaded);
	if(!resourceLoaded)
	{
		return _T("");
	}
	return str;
}


OPENMPT_NAMESPACE_END
