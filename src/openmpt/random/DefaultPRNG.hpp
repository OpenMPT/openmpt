/* SPDX-License-Identifier: BSD-3-Clause */
/* SPDX-FileCopyrightText: OpenMPT Project Developers and Contributors */

#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/random/default_engines.hpp"


OPENMPT_NAMESPACE_BEGIN


namespace mpt
{


#if defined(MODPLUG_TRACKER) || defined(LIBOPENMPT_BUILD)

#ifdef MPT_BUILD_FUZZER

// Use fast PRNGs in order to not waste time fuzzing more complex PRNG
// implementations.
using fast_prng = deterministic_fast_engine;
using good_prng = deterministic_good_engine;

#else  // !MPT_BUILD_FUZZER

// We cannot use std::minstd_rand here because it has not a power-of-2 sized
// output domain which we rely upon.
using fast_prng = fast_engine;  // about 3 ALU operations, ~32bit of state, suited for inner loops
using good_prng = good_engine;

#endif  // MPT_BUILD_FUZZER

#else  // !(MODPLUG_TRACKER || LIBOPENMPT_BUILD)

using fast_prng = fast_engine;
using good_prng = good_engine;

#endif  // MODPLUG_TRACKER || LIBOPENMPT_BUILD


}  // namespace mpt


OPENMPT_NAMESPACE_END
