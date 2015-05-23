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

OPENMPT_NAMESPACE_BEGIN

#ifndef MODPLUG_NO_FILESAVE

FileTags::FileTags()
//------------------
{
	encoder = mpt::ToUnicode(mpt::CharsetASCII, MptVersion::GetOpenMPTVersionStr());
}

#else // MODPLUG_NO_FILESAVE

MPT_MSVC_WORKAROUND_LNK4221(Tagging)

#endif // MODPLUG_NO_FILESAVE


OPENMPT_NAMESPACE_END
