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

#if defined(LIBOPENMPT_BUILD_DLL) || defined(LIBOPENMPT_USE_DLL)
#if defined(_MSC_VER) || defined(_WIN32)
#define LIBOPENMPT_DLL_HELPER_EXPORT __declspec(dllexport)
#define LIBOPENMPT_DLL_HELPER_IMPORT __declspec(dllimport)
#define LIBOPENMPT_DLL_HELPER_LOCAL  
#elif defined(__GNUC__)
#define LIBOPENMPT_DLL_HELPER_EXPORT __attribute__((visibility("default")))
#define LIBOPENMPT_DLL_HELPER_IMPORT __attribute__((visibility("default")))
#define LIBOPENMPT_DLL_HELPER_LOCAL  __attribute__((visibility("hidden")))
#else
#define LIBOPENMPT_DLL_HELPER_EXPORT 
#define LIBOPENMPT_DLL_HELPER_IMPORT
#define LIBOPENMPT_DLL_HELPER_LOCAL
#endif
#else
#define LIBOPENMPT_DLL_HELPER_EXPORT
#define LIBOPENMPT_DLL_HELPER_IMPORT
#define LIBOPENMPT_DLL_HELPER_LOCAL
#endif

#if defined(LIBOPENMPT_BUILD_DLL)
#define LIBOPENMPT_API     LIBOPENMPT_DLL_HELPER_EXPORT
#ifdef __cplusplus
#define LIBOPENMPT_CXX_API LIBOPENMPT_DLL_HELPER_EXPORT
#endif
#elif defined(LIBOPENMPT_USE_DLL)
#define LIBOPENMPT_API     LIBOPENMPT_DLL_HELPER_IMPORT
#ifdef __cplusplus
#define LIBOPENMPT_CXX_API LIBOPENMPT_DLL_HELPER_IMPORT
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
#define LIBOPENMPT_CXX_API LIBOPENMPT_DLL_HELPER_LOCAL
#endif
#endif
#endif

#include "libopenmpt_version.h"

#endif /* LIBOPENMPT_CONFIG_H */
