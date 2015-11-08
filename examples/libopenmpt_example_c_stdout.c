/*
 * libopenmpt_example_c_stdout.c
 * -----------------------------
 * Purpose: libopenmpt C API simple example
 * Notes  : This example writes raw 48000Hz / stereo / 16bit native endian PCM data to stdout.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

/*
 * Usage: libopenmpt_example_c_stdout SOMEMODULE | aplay --file-type raw --format=dat
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
	FILE * file = 0;
	openmpt_module * mod = 0;
	size_t count = 0;
	size_t written = 0;

	if ( argc != 2 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c_stdout SOMEMODULE'." );
		goto fail;
	}

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
	if ( wcslen( argv[1] ) == 0 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c_stdout SOMEMODULE'." );
		goto fail;
	}
	file = _wfopen( argv[1], L"rb" );
#else
	if ( strlen( argv[1] ) == 0 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c_stdout SOMEMODULE'." );
		goto fail;
	}
	file = fopen( argv[1], "rb" );
#endif
	if ( !file ) {
		fprintf( stderr, "Error: %s\n", "fopen() failed." );
		goto fail;
	}

	mod = openmpt_module_create( openmpt_stream_get_file_callbacks(), file, &libopenmpt_example_logfunc, NULL, NULL );
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

	if ( file ) {
		fclose( file );
		file = 0;
	}

	return result;
}
