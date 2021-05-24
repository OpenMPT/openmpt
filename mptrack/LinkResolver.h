/*
 * LinkResolver.h
 * --------------
 * Purpose: Resolve Windows shell link targets
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "../common/mptPathString.h"

#include <ShObjIdl.h>

OPENMPT_NAMESPACE_BEGIN

class LinkResolver
{
	IShellLink *psl = nullptr;
	IPersistFile *ppf = nullptr;

public:
	LinkResolver();
	~LinkResolver();


	mpt::PathString Resolve(const TCHAR *inPath);
};

OPENMPT_NAMESPACE_END
