/*
 * libopenmpt_example_c_pipe.c
 * ---------------------------
 * Purpose: libopenmpt C API simple pipe example
 * Notes  : This example writes raw 48000Hz / stereo / 16bit native endian PCM data to stdout.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

/*
 * Usage: cat SOMEMODULE | libopenmpt_example_c_pipe | aplay --file-type raw --format=dat
 */

#include <memory.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>

#include <libopenmpt/libopenmpt.h>
#include <libopenmpt/libopenmpt_stream_callbacks_fd.h>

#define BUFFERSIZE 480
#define SAMPLERATE 48000

static void libopenmpt_example_logfunc( const char * message, void * userdata ) {
	(void)userdata;

	if ( message ) {
		fprintf( stderr, "%s\n", message );
	}
}

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

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
int wmain( int argc, wchar_t * argv[] ) {
#else
int main( int argc, char * argv[] ) {
#endif

	int result = 0;
	openmpt_module * mod = 0;
	size_t count = 0;
	size_t written = 0;
	(void)argv;

	if ( argc != 1 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c_pipe' and connect stdin and stdout." );
		goto fail;
	}

	mod = openmpt_module_create( openmpt_stream_get_fd_callbacks(), (void*)(uintptr_t)STDIN_FILENO, &libopenmpt_example_logfunc, NULL, NULL );
	if ( !mod ) {
		fprintf( stderr, "Error: %s\n", "openmpt_module_create() failed." );
		goto fail;
	}

	while ( 1 ) {

		count = openmpt_module_read_interleaved_stereo( mod, SAMPLERATE, BUFFERSIZE, buffer );
		if ( count == 0 ) {
			break;
		}

		written = xwrite( STDOUT_FILENO, buffer, count * 2 * sizeof( int16_t ) );
		if ( written == 0 ) {
			fprintf( stderr, "Error: %s\n", "write() failed." );
			goto fail;
		}
	}

	result = 0;

	goto cleanup;

fail:

	result = 1;

cleanup:

	if ( mod ) {
		openmpt_module_destroy( mod );
		mod = 0;
	}

	return result;
}
