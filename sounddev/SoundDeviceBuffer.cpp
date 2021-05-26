/* SPDX-License-Identifier: BSD-3-Clause */
/* SPDX-FileCopyrightText: OpenMPT Project Developers and Contributors */


#include "openmpt/all/BuildSettings.hpp"

#include "SoundDeviceBuffer.h"

#if defined(MODPLUG_TRACKER)
#include "mptBaseMacros.h"
#endif // MODPLUG_TRACKER

OPENMPT_NAMESPACE_BEGIN

namespace SoundDevice {

#if defined(MODPLUG_TRACKER)
MPT_MSVC_WORKAROUND_LNK4221(SoundDeviceBuffer)
#endif // MODPLUG_TRACKER

} // namespace SoundDevice

OPENMPT_NAMESPACE_END
