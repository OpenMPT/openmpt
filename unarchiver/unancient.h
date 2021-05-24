/*
 * ununancient.h
 * -------------
 * Purpose: Header file extracting modules from compressed files supported by libancient
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "archive.h"

OPENMPT_NAMESPACE_BEGIN

#ifdef MPT_WITH_ANCIENT

class CAncientArchive
	: public ArchiveBase
{
public:
	CAncientArchive(FileReader &file);
	virtual ~CAncientArchive();
public:
	bool ExtractFile(std::size_t index) override;
};

#endif // MPT_WITH_ANCIENT

OPENMPT_NAMESPACE_END
