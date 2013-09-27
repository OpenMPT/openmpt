/*
 * libopenmpt_version.h
 * --------------------
 * Purpose: libopenmpt public interface version
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_VERSION_H
#define LIBOPENMPT_VERSION_H

#define OPENMPT_API_VERSION_MAJOR 0
#define OPENMPT_API_VERSION_MINOR 0

#define OPENMPT_API_VERSION ((OPENMPT_API_VERSION_MAJOR<<24)|(OPENMPT_API_VERSION_MINOR<<16))

#define OPENMPT_API_VERSION_HELPER_STRINGIZE(x) #x
#define OPENMPT_API_VERSION_STRINGIZE(x) OPENMPT_API_VERSION_HELPER_STRINGIZE(x)
#define OPENMPT_API_VERSION_STRING OPENMPT_API_VERSION_STRINGIZE(OPENMPT_API_VERSION_MAJOR) "." OPENMPT_API_VERSION_STRINGIZE(OPENMPT_API_VERSION_MINOR)

#ifndef LIBOPENMPT_ALPHA_WARNING_SEEN_AND_I_KNOW_WHAT_I_AM_DOING
#ifdef __cplusplus
static_assert( false, "libopenmpt is currently in alpha stage, do not rely on ABI or even API compatibility for the next few months yet. #define LIBOPENMPT_ALPHA_WARNING_SEEN_AND_I_KNOW_WHAT_I_AM_DOING to acknowledge this warning." );
#else
typedef char OPENMPT_STATIC_ASSERT[-1]; /* libopenmpt is currently in alpha stage, do not rely on ABI or even API compatibility for the next few months yet. #define LIBOPENMPT_ALPHA_WARNING_SEEN_AND_I_KNOW_WHAT_I_AM_DOING to acknowledge this warning. */
#endif
#endif /* LIBOPENMPT_ALPHA_WARNING_SEEN_AND_I_KNOW_WHAT_I_AM_DOING */

#endif /* LIBOPENMPT_VERSION_H */
