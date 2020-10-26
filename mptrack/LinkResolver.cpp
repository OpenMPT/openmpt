/*
 * LinkResolver.cpp
 * ----------------
 * Purpose: Resolve Windows shell link targets
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "LinkResolver.h"

#include <atlconv.h>

OPENMPT_NAMESPACE_BEGIN

LinkResolver::LinkResolver()
{
	HRESULT result = CoCreateInstance(CLSID_ShellLink,
	                                  0,
	                                  CLSCTX_INPROC_SERVER,
	                                  IID_IShellLink,
	                                  reinterpret_cast<LPVOID *>(&psl));
	if(SUCCEEDED(result) && psl != nullptr)
	{
		psl->QueryInterface(IID_IPersistFile, reinterpret_cast<LPVOID *>(&ppf));
	}
}

LinkResolver::~LinkResolver()
{
	if(ppf != nullptr)
		ppf->Release();
	if(psl != nullptr)
		psl->Release();
}

mpt::PathString LinkResolver::Resolve(const TCHAR *inPath)
{
	if(ppf == nullptr)
		return {};

	SHFILEINFO info;
	Clear(info);
	if((SHGetFileInfo(inPath, 0, &info, sizeof(info), SHGFI_ATTRIBUTES) == 0) || !(info.dwAttributes & SFGAO_LINK))
		return {};

	USES_CONVERSION;  // T2COLE needs this
	if(ppf != nullptr && SUCCEEDED(ppf->Load(T2COLE(inPath), STGM_READ)))
	{
		if(SUCCEEDED(psl->Resolve(AfxGetMainWnd()->m_hWnd, MAKELONG(SLR_ANY_MATCH | SLR_NO_UI | SLR_NOSEARCH, 100))))
		{
			TCHAR outPath[MAX_PATH];
			psl->GetPath(outPath, mpt::saturate_cast<int>(std::size(outPath)), nullptr, 0);
			return mpt::PathString::FromNative(outPath);
		}
	}
	return {};
}

OPENMPT_NAMESPACE_END
