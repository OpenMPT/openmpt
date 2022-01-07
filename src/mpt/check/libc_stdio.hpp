/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_CHECK_LIBC_STDIO_HPP
#define MPT_CHECK_LIBC_STDIO_HPP

#include "mpt/base/detect_libc.hpp"
#include "mpt/base/detect_os.hpp"
#include "mpt/check/libc.hpp"

#include <cstdio>

#if !MPT_LIBC_MS
#include <unistd.h>    // _POSIX_VERSION
#else                  // MPT_LIBC_MS
#include <sys/types.h> // off_t
#endif                 // !MPT_LIBC_MS

#ifndef MPT_CHECK_LIBC_IGNORE_SIZEOF_OFF_T
#if MPT_OS_DJGPP
// pure 32bit platform
#elif MPT_LIBC_MS
static_assert(true);
#elif MPT_LIBC_MINGW
static_assert(true);
#elif defined(_POSIX_VERSION) && (_POSIX_VERSION > 0) && defined(_LFS64_LARGEFILE) && (_LFS64_LARGEFILE == 1) && defined(_LFS64_STDIO) && (_LFS64_STDIO == 1)
static_assert(sizeof(off64_t) == 8, "C stdlib does not provide 64bit std::FILE access even though it claims to do so. You may #define MPT_CHECK_LIBC_IGNORE_SIZEOF_OFF_T to ignore this error.")
#elif defined(_POSIX_VERSION) && (_POSIX_VERSION > 0) && ((_POSIX_VERSION > 200112L) || (defined(_LFS_LARGEFILE) && (_LFS_LARGEFILE == 1)))
MPT_WARNING("C stdlib does not provide default 64bit std::FILE access. Please #define _FILE_OFFSET_BITS = 64. You may also #define MPT_CHECK_LIBC_IGNORE_SIZEOF_OFF_T to silence this warning.)
#elif MPT_LIBC_GLIBC
MPT_WARNING("C stdlib does not provide 64bit std::FILE access. Please #define _FILE_OFFSET_BITS=64.")
#else
MPT_WARNING("C stdlib may not provide 64bit std::FILE access. Please #define _FILE_OFFSET_BITS=64 or equivalent, or #define MPT_CHECK_LIBC_IGNORE_SIZEOF_OFF_T to silence this warning.")
#endif
#endif

#endif // MPT_CHECK_LIBC_STDIO_HPP
