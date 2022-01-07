/*
 * libopenmpt_stream_callbacks_file.h
 * ----------------------------------
 * Purpose: libopenmpt public c interface
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_STREAM_CALLBACKS_FILE_H
#define LIBOPENMPT_STREAM_CALLBACKS_FILE_H

#include "libopenmpt.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef _MSC_VER
#include <wchar.h> /* off_t */
#endif

/*! \addtogroup libopenmpt_c
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/* This stuff has to be in a header file because of possibly different MSVC CRTs which cause problems for FILE * crossing CRT boundaries. */

static size_t openmpt_stream_file_read_func( void * stream, void * dst, size_t bytes ) {
	FILE * f = 0;
	size_t retval = 0;
	f = (FILE*)stream;
	if ( !f ) {
		return 0;
	}
	retval = fread( dst, 1, bytes, f );
	if ( retval <= 0 ) {
		return 0;
	}
	return retval;
}

static int openmpt_stream_file_seek_func( void * stream, int64_t offset, int whence ) {
	FILE * f = 0;
	int fwhence = 0;
	f = (FILE*)stream;
	if ( !f ) {
		return -1;
	}
	switch ( whence ) {
#if defined(SEEK_SET)
		case OPENMPT_STREAM_SEEK_SET:
			fwhence = SEEK_SET;
			break;
#endif
#if defined(SEEK_CUR)
		case OPENMPT_STREAM_SEEK_CUR:
			fwhence = SEEK_CUR;
			break;
#endif
#if defined(SEEK_END)
		case OPENMPT_STREAM_SEEK_END:
			fwhence = SEEK_END;
			break;
#endif
		default:
			return -1;
			break;
	}
	#if defined(_MSC_VER)
		if ( (__int64)offset != offset ) {
			return -1;
		}
		return _fseeki64( f, (__int64)offset, fwhence ) ? -1 : 0;
	#elif defined(_POSIX_SOURCE) && (_POSIX_SOURCE == 1) 
		if ( (off_t)offset != offset ) {
			return -1;
		}
		return fseeko( f, (off_t)offset, fwhence ) ? -1 : 0;
	#else
		if ( (long)offset != offset ) {
			return -1;
		}
		return fseek( f, (long)offset, fwhence ) ? -1 : 0;
	#endif
}

static int64_t openmpt_stream_file_tell_func( void * stream ) {
	FILE * f = 0;
	#if defined(_MSC_VER)
		__int64 result = 0;
	#elif defined(_POSIX_SOURCE) && (_POSIX_SOURCE == 1)
		off_t result = 0;
	#else
		long result = 0;
	#endif
	int64_t retval = 0;
	f = (FILE*)stream;
	if ( !f ) {
		return -1;
	}
	#if defined(_MSC_VER)
		result = _ftelli64( f );
	#elif defined(_POSIX_SOURCE) && (_POSIX_SOURCE == 1) 
		result = ftello( f );
	#else
		result = ftell( f );
	#endif
	if ( (int64_t)result != result ) {
		return -1;
	}
	retval = (int64_t)result;
	if ( retval < 0 ) {
		return -1;
	}
	return retval;
}

/*! \brief Provide openmpt_stream_callbacks for standard C FILE objects
 *
 * Fills openmpt_stream_callbacks suitable for passing a standard C FILE object as a stream parameter to functions doing file input/output.
 *
 * \remarks The stream argument must be passed as `(void*)(FILE*)file`.
 * \sa \ref libopenmpt_c_fileio
 * \sa openmpt_stream_callbacks
 * \sa openmpt_could_open_probability2
 * \sa openmpt_probe_file_header_from_stream
 * \sa openmpt_module_create2
 */
static openmpt_stream_callbacks openmpt_stream_get_file_callbacks(void) {
	openmpt_stream_callbacks retval;
	memset( &retval, 0, sizeof( openmpt_stream_callbacks ) );
	retval.read = openmpt_stream_file_read_func;
	retval.seek = openmpt_stream_file_seek_func;
	retval.tell = openmpt_stream_file_tell_func;
	return retval;
}

#ifdef __cplusplus
}
#endif

/*!
 * @}
 */

#endif /* LIBOPENMPT_STREAM_CALLBACKS_FILE_H */

