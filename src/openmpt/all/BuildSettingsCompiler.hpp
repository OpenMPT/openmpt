/* SPDX-License-Identifier: BSD-3-Clause */
/* SPDX-FileCopyrightText: OpenMPT Project Developers and Contributors */


#pragma once



#include "openmpt/all/BuildSettings.hpp"



#include "mpt/base/detect_compiler.hpp"



#if MPT_COMPILER_GCC

#ifdef MPT_COMPILER_SETTING_QUIRK_GCC_BROKEN_IPA
// See <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115049>.
#if MPT_GCC_BEFORE(9, 0, 0)
// Earlier GCC get confused about setting a global function attribute.
// We need to check for 9.0 instead of 9.1 because of
// <https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=1028580>.
// It also gets confused when setting global optimization -O1,
// so we have no way of fixing GCC 8 or earlier.
//#pragma GCC optimize("O1")
#else
#pragma GCC optimize("no-ipa-ra")
#endif
#endif  // MPT_COMPILER_SETTING_QUIRK_GCC_BROKEN_IPA

#endif  // MPT_COMPILER_GCC
