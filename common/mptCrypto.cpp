/*
 * mptCrypto.cpp
 * -------------
 * Purpose: .
 * Notes  : (currently none)
 * Authors: Joern Heusipp
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "stdafx.h"
#include "mptCrypto.h"

#include "mptAssert.h"
#include "mptBaseMacros.h"
#include "mptBaseTypes.h"
#include "mptBaseUtils.h"
#include "mptException.h"
#include "mptSpan.h"

#include <algorithm>
#include <vector>

#include <cstddef>
#include <cstdint>

#ifdef MODPLUG_TRACKER
#if MPT_OS_WINDOWS
#include <windows.h>
#include <bcrypt.h>
#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_BEGIN


namespace mpt
{


MPT_MSVC_WORKAROUND_LNK4221(mptCrypto)


} // namespace mpt


OPENMPT_NAMESPACE_END
