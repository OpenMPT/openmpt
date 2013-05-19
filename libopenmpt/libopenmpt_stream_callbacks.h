/*
 * libopenmpt_stream_callbacks.h
 * -----------------------------
 * Purpose: libopenmpt public c interface
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_STREAM_CALLBACKS_H
#define LIBOPENMPT_STREAM_CALLBACKS_H

#include "libopenmpt.h"

#ifdef _MSC_VER
#include <io.h>
#endif
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#ifdef _MSC_VER
#include <wchar.h> // off_t
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* This stuff has to be in a header file because of possibly different MSVC CRTs which cause problems for FILE * crossing CRT boundaries. */

static size_t openmpt_stream_fd_read_func( void * stream, void * dst, size_t bytes ) {
	int fd = (int)(uintptr_t)stream;
	if ( fd < 0 ) {
		return 0;
	}
	#if defined(_MSC_VER)
		size_t retval = 0;
		while ( bytes > 0 ) {
			int to_read = 0;
			if ( bytes < (size_t)INT_MAX ) {
				to_read = (int)bytes;
			} else {
				to_read = INT_MAX;
			}
			int ret_read = _read( fd, dst, to_read );
			if ( ret_read <= 0 ) {
				return retval;
			}
			bytes -= ret_read;
			retval += ret_read;
		}
	#else
		ssize_t retval = read( fd, dst, bytes );
	#endif
	if ( retval <= 0 ) {
		return 0;
	}
	return retval;
}

static size_t openmpt_stream_file_read_func( void * stream, void * dst, size_t bytes ) {
	FILE * f = (FILE*)stream;
	if ( !f ) {
		return 0;
	}
	size_t retval = fread( dst, 1, bytes, f );
	if ( retval <= 0 ) {
		return 0;
	}
	return retval;
}

static int openmpt_stream_file_seek_func( void * stream, int64_t offset, int whence ) {
	FILE * f = (FILE*)stream;
	if ( !f ) {
		return -1;
	}
	#if defined(_MSC_VER)
		return _fseeki64( f, offset, whence ) ? -1 : 0;
	#elif defined(_POSIX_SOURCE) && (_POSIX_SOURCE == 1) 
		return fseeko( f, offset, whence ) ? -1 : 0;
	#else
		return fseek( f, offset, whence ) ? -1 : 0;
	#endif
}

static int64_t openmpt_stream_file_tell_func( void * stream ) {
	FILE * f = (FILE*)stream;
	if ( !f ) {
		return -1;
	}
	#if defined(_MSC_VER)
		int64_t retval = _ftelli64( f );
	#elif defined(_POSIX_SOURCE) && (_POSIX_SOURCE == 1) 
		int64_t retval = ftello( f );
	#else
		int64_t retval = ftell( f );
	#endif
	if ( retval < 0 ) {
		return -1;
	}
	return retval;
}

static inline openmpt_stream_callbacks openmpt_stream_get_fd_callbacks(void) {
	openmpt_stream_callbacks retval;
	memset( &retval, 0, sizeof( openmpt_stream_callbacks ) );
	retval.read = openmpt_stream_fd_read_func;
	return retval;
}

static inline openmpt_stream_callbacks openmpt_stream_get_file_callbacks(void) {
	openmpt_stream_callbacks retval;
	memset( &retval, 0, sizeof( openmpt_stream_callbacks ) );
	retval.read = openmpt_stream_file_read_func;
	retval.seek = openmpt_stream_file_seek_func;
	retval.tell = openmpt_stream_file_tell_func;
	return retval;
}

#ifdef __cplusplus
};
#endif

#endif /* LIBOPENMPT_STREAM_CALLBACKS_H */
