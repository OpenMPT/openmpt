/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_CHECK_LIBCXX_HPP
#define MPT_CHECK_LIBCXX_HPP

#include "mpt/base/detect_libcxx.hpp"
#include "mpt/base/detect_os.hpp"
#include "mpt/base/detect_quirks.hpp"

#ifndef MPT_CHECK_LIBCXX_IGNORE_WARNING_NO_THREADS
#if MPT_OS_WINDOWS && MPT_WIN_BEFORE(MPT_WIN_VISTA) && MPT_LIBCXX_GNU_AT_LEAST(13) && !defined(_GLIBCXX_HAS_GTHREADS)
#error "GNU libstdc++ is compiled without gthreads support (likely due to using Win32 threading model as opposed to POSIX threading model. This a severely crippled C++11 implementation and no is no longer supported for targetting Windows before Vista as of release 13 because non-threading standard library headers fail to compile."
#endif
#endif

#endif // MPT_CHECK_LIBCXX_HPP
