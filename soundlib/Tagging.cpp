/*
 * tagging.cpp
 * -----------
 * Purpose: Structure holding a superset of tags for all supported output sample or stream files or types.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Tagging.h"
#include "../common/version.h"

#ifndef MODPLUG_NO_FILESAVE

FileTags::FileTags()
//------------------
{
	encoder = mpt::ToWide(mpt::CharsetASCII, MptVersion::GetOpenMPTVersionStr());
}

#endif // MODPLUG_NO_FILESAVE
