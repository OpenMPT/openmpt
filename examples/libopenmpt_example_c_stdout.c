/*
 * libopenmpt_example_c_stdout.c
 * -----------------------------
 * Purpose: libopenmpt C API simple example.
 *          This examples demonstrates how to play a module file in C.
 *          The module file to play must be specified as a command line parameter.
 *          If no parameter is given, a built-in module is loaded from memory instead.
 *          The rendered audio data is send to stdout, from where it can be piped
 *          into a file or program (see below for more details).
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

/*
 * This example writes raw 48000Hz / stereo / 16bit native endian PCM data to stdout.
 *
 * Use for example (on little endian platforms):
 *
 *     bin/libopenmpt_example_c_stdout SOMEMODULE | aplay --file-type raw --format=dat
 *
 */

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>

#include <libopenmpt/libopenmpt.h>
#include <libopenmpt/libopenmpt_stream_callbacks_file.h>
#include "chipsim.h"

#define BUFFERSIZE 480
#define SAMPLERATE 48000

static ssize_t xwrite( int fd, const void * buffer, size_t size ) {
	size_t written = 0;
	ssize_t retval = 0;
	while ( written < size ) {
		retval = write( fd, (const char *)buffer + written, size - written );
		if ( retval < 0 ) {
			if ( errno != EINTR ) {
				break;
			}
			retval = 0;
		}
		written += retval;
	}
	return written;
}

static int16_t buffer[BUFFERSIZE * 2];

#if (defined(_WIN32) || defined(WIN32)) && (defined(_UNICODE) || defined(UNICODE))
int wmain( int argc, wchar_t * argv[] ) {
#else
int main( int argc, char * argv[] ) {
#endif
	FILE * file = 0;
	openmpt_module * mod = 0;
	size_t count = 0;
	
	if ( argc > 1 ) {
#if (defined(_WIN32) || defined(WIN32)) && (defined(_UNICODE) || defined(UNICODE))
		file = _wfopen( argv[1], L"rb" );
#else
		file = fopen( argv[1], "rb" );
#endif
		mod = openmpt_module_create( openmpt_stream_get_file_callbacks(), file, NULL, NULL, NULL );
	} else {
		/* no command line parameter specified - play an embedded example module. */
		mod = openmpt_module_create_from_memory( chipsim_it, sizeof( chipsim_it), NULL, NULL, NULL );
	}
	
	if ( argc > 1 ) {
		fclose( file );
	}

	while ( 1 ) {
		count = openmpt_module_read_interleaved_stereo( mod, SAMPLERATE, BUFFERSIZE, buffer );
		if ( count == 0 ) {
			break;
		}
		xwrite( STDOUT_FILENO, buffer, count * 2 * sizeof( int16_t ) );
	}
	openmpt_module_destroy( mod );
	return 0;
}
