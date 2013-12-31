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

/*! \addtogroup libopenmpt
  @{
*/

/*! \brief libopenmpt major version number */
#define OPENMPT_API_VERSION_MAJOR 0
/*! \brief libopenmpt minor version number */
#define OPENMPT_API_VERSION_MINOR 2

/*! \brief libopenmpt API version number */
#define OPENMPT_API_VERSION ((OPENMPT_API_VERSION_MAJOR<<24)|(OPENMPT_API_VERSION_MINOR<<16))

#define OPENMPT_API_VERSION_HELPER_STRINGIZE(x) #x
#define OPENMPT_API_VERSION_STRINGIZE(x) OPENMPT_API_VERSION_HELPER_STRINGIZE(x)
#define OPENMPT_API_VERSION_STRING OPENMPT_API_VERSION_STRINGIZE(OPENMPT_API_VERSION_MAJOR) "." OPENMPT_API_VERSION_STRINGIZE(OPENMPT_API_VERSION_MINOR)

/*!
  @}
*/

#endif /* LIBOPENMPT_VERSION_H */
