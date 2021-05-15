/*
 * AudioSample.h
 * -------------
 * Purpose: Basic audio sample types.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "mptBuildSettings.h"

#include "../common/mptBaseTypes.h"

#include <type_traits>


OPENMPT_NAMESPACE_BEGIN


using AudioSampleInt = int16;
using AudioSampleFloat = nativefloat;

using AudioSample = std::conditional<mpt::float_traits<AudioSampleFloat>::is_hard, AudioSampleFloat, AudioSampleInt>::type;


OPENMPT_NAMESPACE_END
