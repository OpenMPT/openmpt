/*
 * libopenmpt_example_c_mem.c
 * --------------------------
 * Purpose: libopenmpt C API example
 * Notes  : PortAudio is used for sound output.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

/*
 * Usage: libopenmpt_example_c_mem SOMEMODULE
 */

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libopenmpt/libopenmpt.h>

#include <portaudio.h>

#define BUFFERSIZE 480
#define SAMPLERATE 48000

static int16_t left[BUFFERSIZE];
static int16_t right[BUFFERSIZE];
static int16_t * const buffers[2] = { left, right };

static void libopenmpt_example_logfunc( const char * message, void * userdata ) {
	(void)userdata;

	if ( message ) {
		fprintf( stderr, "%s\n", message );
	}
}

typedef struct blob_t {
	size_t size;
	void * data;
} blob_t;

static void free_blob( blob_t * blob ) {
	if ( blob ) {
		if ( blob->data ) {
			free( blob->data );
			blob->data = 0;
		}
		blob->size = 0;
		free( blob );
	}
}

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
static blob_t * load_file( const wchar_t * filename ) {
#else
static blob_t * load_file( const char * filename ) {
#endif
	blob_t * result = 0;

	blob_t * blob = 0;
	FILE * file = 0;
	long tell_result = 0;

	blob = malloc( sizeof( blob_t ) );
	if ( !blob ) {
		goto fail;
	}
	memset( blob, 0, sizeof( blob_t ) );

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
	file = _wfopen( filename, L"rb" );
#else
	file = fopen( filename, "rb" );
#endif
	if ( !file ) {
		goto fail;
	}

	if ( fseek( file, 0, SEEK_END ) != 0 ) {
		goto fail;
	}

	tell_result = ftell( file );
	if ( tell_result < 0 ) {
		goto fail;
	}
	if ( (unsigned long)tell_result > SIZE_MAX ) {
		goto fail;
	}
	blob->size = (size_t)tell_result;

	if ( fseek( file, 0, SEEK_SET ) != 0 ) {
		goto fail;
	}

	blob->data = malloc( blob->size );
	if ( !blob->data ) {
		goto fail;
	}
	memset( blob->data, 0, blob->size );

	if ( fread( blob->data, 1, blob->size, file ) != blob->size ) {
		goto fail;
	}

	result = blob;
	blob = 0;
	goto cleanup;

fail:

	result = 0;

cleanup:

	if ( blob ) {
		free_blob( blob );
		blob = 0;
	}

	if ( file ) {
		fclose( file );
		file = 0;
	}

	return result;
}

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
int wmain( int argc, wchar_t * argv[] ) {
#else
int main( int argc, char * argv[] ) {
#endif

	int result = 0;
	blob_t * blob = 0;
	openmpt_module * mod = 0;
	size_t count = 0;
	PaError pa_error = paNoError;
	int pa_initialized = 0;
	PaStream * stream = 0;

	if ( argc != 2 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c_mem SOMEMODULE'." );
		goto fail;
	}

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
	if ( wcslen( argv[1] ) == 0 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c_mem SOMEMODULE'." );
		goto fail;
	}
#else
	if ( strlen( argv[1] ) == 0 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c_mem SOMEMODULE'." );
		goto fail;
	}
#endif
	blob = load_file( argv[1] );
	if ( !blob ) {
		fprintf( stderr, "Error: %s\n", "load_file() failed." );
		goto fail;
	}

	mod = openmpt_module_create_from_memory( blob->data, blob->size, &libopenmpt_example_logfunc, NULL, NULL );
	if ( !mod ) {
		fprintf( stderr, "Error: %s\n", "openmpt_module_from_memory() failed." );
		goto fail;
	}

	pa_error = Pa_Initialize();
	if ( pa_error != paNoError ) {
		fprintf( stderr, "Error: %s\n", "Pa_Initialize() failed." );
		goto fail;
	}
	pa_initialized = 1;

	pa_error = Pa_OpenDefaultStream( &stream, 0, 2, paInt16 | paNonInterleaved, SAMPLERATE, paFramesPerBufferUnspecified, NULL, NULL );
	if ( pa_error != paNoError ) {
		fprintf( stderr, "Error: %s\n", "Pa_OpenStream() failed." );
		goto fail;
	}
	if ( !stream ) {
		fprintf( stderr, "Error: %s\n", "Pa_OpenStream() failed." );
		goto fail;
	}

	pa_error = Pa_StartStream( stream );
	if ( pa_error != paNoError ) {
		fprintf( stderr, "Error: %s\n", "Pa_StartStream() failed." );
		goto fail;
	}

	while ( 1 ) {

		count = openmpt_module_read_stereo( mod, SAMPLERATE, BUFFERSIZE, left, right );
		if ( count == 0 ) {
			break;
		}

		pa_error = Pa_WriteStream( stream, buffers, (unsigned long)count );
		if ( pa_error == paOutputUnderflowed ) {
			pa_error = paNoError;
		}
		if ( pa_error != paNoError ) {
			fprintf( stderr, "Error: %s\n", "Pa_WriteStream() failed." );
			goto fail;
		}
	}

	result = 0;

	goto cleanup;

fail:

	result = 1;

cleanup:

	if ( stream ) {
		if ( Pa_IsStreamActive( stream ) == 1 ) {
			Pa_StopStream( stream );
		}
		Pa_CloseStream( stream );
		stream = 0;
	}

	if ( pa_initialized ) {
		Pa_Terminate();
		pa_initialized = 0;
	}

	if ( mod ) {
		openmpt_module_destroy( mod );
		mod = 0;
	}

	if ( blob ) {
		free_blob( blob );
		blob = 0;
	}

	return result;
}
