/*
 * libopenmpt_example_c.c
 * ----------------------
 * Purpose: libopenmpt C API simple example
 * Notes  : PortAudio is used for sound output.
 *          This example does full error checking.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

/*
 * Usage example:
 *
 *     bin/libopenmpt_example_c_safe SOMEMODULE
 */

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libopenmpt/libopenmpt.h>
#include <libopenmpt/libopenmpt_stream_callbacks_file.h>

#include <portaudio.h>

#define BUFFERSIZE 480
#define SAMPLERATE 48000

static int16_t left[BUFFERSIZE];
static int16_t right[BUFFERSIZE];
static int16_t * const buffers[2] = { left, right };

static void libopenmpt_example_logfunc( const char * message, void * user ) {
	(void)user;
	if ( message ) {
		fprintf( stderr, "%s\n", message );
	}
}

#if (defined(_WIN32) || defined(WIN32)) && (defined(_UNICODE) || defined(UNICODE))
int wmain( int argc, wchar_t * argv[] ) {
#else
int main( int argc, char * argv[] ) {
#endif
	int result = 0;
	FILE * file = 0;
	openmpt_module * mod = 0;
	size_t count = 0;
	PaError pa_error = paNoError;
	int pa_initialized = 0;
	PaStream * stream = 0;
	PaStreamParameters streamparameters;
	memset( &streamparameters, 0, sizeof( PaStreamParameters ) );
	if ( argc != 2 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'bin/libopenmpt_example_c_safe SOMEMODULE'." );
		goto fail;
	}
#if (defined(_WIN32) || defined(WIN32)) && (defined(_UNICODE) || defined(UNICODE))
	if ( wcslen( argv[1] ) == 0 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'bin/libopenmpt_example_c_safe SOMEMODULE'." );
		goto fail;
	}
	file = _wfopen( argv[1], L"rb" );
#else
	if ( strlen( argv[1] ) == 0 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'bin/libopenmpt_example_c_safe SOMEMODULE'." );
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
	pa_error = Pa_Initialize();
	if ( pa_error != paNoError ) {
		fprintf( stderr, "Error: %s\n", "Pa_Initialize() failed." );
		goto fail;
	}
	pa_error = paNoError;
	pa_initialized = 1;
	streamparameters.device = Pa_GetDefaultOutputDevice();
	if ( streamparameters.device == paNoDevice ) {
		fprintf( stderr, "Error: %s\n", "Pa_GetDefaultOutputDevice() failed." );
		goto fail;
	}
	streamparameters.channelCount = 2;
	streamparameters.sampleFormat = paInt16 | paNonInterleaved;
	if ( !Pa_GetDeviceInfo( streamparameters.device ) ) {
		fprintf( stderr, "Error: %s\n", "Pa_GetDeviceInfo() failed." );
		goto fail;
	}
	streamparameters.suggestedLatency = Pa_GetDeviceInfo( streamparameters.device )->defaultHighOutputLatency;
	pa_error = Pa_OpenStream( &stream, NULL, &streamparameters, SAMPLERATE, paFramesPerBufferUnspecified, 0, NULL, NULL );
	if ( pa_error != paNoError ) {
		fprintf( stderr, "Error: %s\n", "Pa_OpenStream() failed." );
		goto fail;
	}
	pa_error = paNoError;
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
		if ( pa_error != paNoError ) {
			fprintf( stderr, "Error: %s\n", "Pa_WriteStream() failed." );
			goto fail;
		}
		pa_error = paNoError;
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
	memset( &streamparameters, 0, sizeof( PaStreamParameters ) );
	if ( pa_initialized ) {
		Pa_Terminate();
		pa_initialized = 0;
	}
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
