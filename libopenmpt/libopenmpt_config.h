/*
 * libopenmpt_config.h
 * -------------------
 * Purpose: libopenmpt public interface configuration
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_CONFIG_H
#define LIBOPENMPT_CONFIG_H

#if defined(LIBOPENMPT_USE_DLL)
#define LIBOPENMPT_API     __declspec(dllimport)
#ifdef __cplusplus
#define LIBOPENMPT_CXX_API __declspec(dllimport)
#endif
#else
#define LIBOPENMPT_API     extern
#ifdef __cplusplus
#define LIBOPENMPT_CXX_API 
#endif
#endif

#ifdef __cplusplus
#if defined(LIBOPENMPT_USE_DLL)
#if defined(_MSC_VER) && !defined(_DLL)
#error "C++ interface is disabled if libopenmpt is built as a DLL and the runtime is statically linked. This is not supported by microsoft and cannot possibly work. Ever."
#undef LIBOPENMPT_CXX_API
#define LIBOPENMPT_CXX_API
#endif
#endif
#endif

#include "libopenmpt_version.h"

#endif /* LIBOPENMPT_CONFIG_H */
