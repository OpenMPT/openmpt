/*
 * mptUUIDNamespace.h
 * ------------------
 * Purpose: UUID utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "../common/mptString.h"
#include "../common/mptUUID.h"


OPENMPT_NAMESPACE_BEGIN


namespace mpt {

// Create a RFC4122 Version 3 namespace UUID
UUID UUIDRFC4122NamespaceV3(const UUID &ns, const mpt::ustring &name);
// Create a RFC4122 Version 5 namespace UUID
UUID UUIDRFC4122NamespaceV5(const UUID &ns, const mpt::ustring &name);

} // namespace mpt


OPENMPT_NAMESPACE_END
