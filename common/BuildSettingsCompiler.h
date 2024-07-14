/*
 * BuildSettingsCompiler.h
 * -----------------------
 * Purpose: Global compiler setting overrides
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once



#include "mpt/base/detect_compiler.hpp"



// compiler configuration

#if MPT_COMPILER_MSVC

#pragma warning(default:4800) // Implicit conversion from 'int' to bool. Possible information loss

#pragma warning(disable:4355) // 'this' : used in base member initializer list

// happens for immutable classes (i.e. classes containing const members)
#pragma warning(disable:4512) // assignment operator could not be generated

#pragma warning(error:4309) // Treat "truncation of constant value"-warning as error.
#pragma warning(error:4463) // Treat overflow; assigning value to bit-field that can only hold values from low_value to high_value"-warning as error.

#ifdef MPT_BUILD_ANALYZED
// Disable Visual Studio static analyzer warnings that generate too many false positives in VS2010.
//#pragma warning(disable:6246)
//#pragma warning(disable:6262)
#pragma warning(disable:6297) // 32-bit value is shifted, then cast to 64-bit value.  Results might not be an expected value. 
#pragma warning(disable:6326) // Potential comparison of a constant with another constant
//#pragma warning(disable:6385)
//#pragma warning(disable:6386)
#endif // MPT_BUILD_ANALYZED

#endif // MPT_COMPILER_MSVC

#if MPT_COMPILER_GCC

#ifdef MPT_COMPILER_SETTING_QUIRK_GCC_NO_IPA_RA
// See <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115049>.
#pragma GCC optimize("no-ipa-ra")
#endif // MPT_COMPILER_SETTING_QUIRK_GCC_NO_IPA_RA

#endif // MPT_COMPILER_GCC

#if MPT_COMPILER_CLANG

#if defined(MPT_BUILD_MSVC)
#pragma clang diagnostic warning "-Wimplicit-fallthrough"
#endif // MPT_BUILD_MSVC

#if defined(MODPLUG_TRACKER)
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif // MODPLUG_TRACKER

#endif // MPT_COMPILER_CLANG
