/*
 * Container.h
 * -----------
 * Purpose: General interface for MDO container and/or packers.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../common/FileReader.h"

#include <vector>

OPENMPT_NAMESPACE_BEGIN


struct ContainerItem
{
	mpt::ustring name;
	FileReader file;
	std::unique_ptr<std::vector<char> > data_cache; // may be empty
};


OPENMPT_NAMESPACE_END
