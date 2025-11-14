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



#include "openmpt/all/BuildSettingsCompiler.hpp"



// compiler configuration

#if MPT_COMPILER_MSVC

// happens for every useful alignas() use
#pragma warning(disable:4324) // structure was padded due to alignment specifier

#pragma warning(disable:4355) // 'this' : used in base member initializer list

// happens for immutable classes (i.e. classes containing const members)
#pragma warning(disable:4512) // assignment operator could not be generated

#pragma warning(error:4309) // Treat "truncation of constant value"-warning as error.
#pragma warning(error:4463) // Treat overflow; assigning value to bit-field that can only hold values from low_value to high_value"-warning as error.

#ifdef MPT_BUILD_ANALYZED
// Disable Visual Studio static analyzer warnings that generate too many false positives.
#pragma warning(disable:6297) // 32-bit value is shifted, then cast to 64-bit value.  Results might not be an expected value. 
#pragma warning(disable:6326) // Potential comparison of a constant with another constant
#endif // MPT_BUILD_ANALYZED

#endif // MPT_COMPILER_MSVC

#if MPT_COMPILER_CLANG

#if defined(MPT_BUILD_MSVC)
#pragma clang diagnostic warning "-Wimplicit-fallthrough"
#endif // MPT_BUILD_MSVC

#if defined(MODPLUG_TRACKER)
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif // MODPLUG_TRACKER

#endif // MPT_COMPILER_CLANG
