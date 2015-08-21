/*
 * libopenmpt_example_c.c
 * ----------------------
 * Purpose: libopenmpt C API simple example
 * Notes  : This simple example does no error cheking at all.
 *          PortAudio is used for sound output.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

/*
 * Usage example:
 *
 *     bin/libopenmpt_example_c SOMEMODULE
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

#if (defined(_WIN32) || defined(WIN32)) && (defined(_UNICODE) || defined(UNICODE))
int wmain( int argc, wchar_t * argv[] ) {
#else
int main( int argc, char * argv[] ) {
#endif
	FILE * file = 0;
	openmpt_module * mod = 0;
	size_t count = 0;
	PaStream * stream = 0;
	PaStreamParameters streamparameters;
	memset( &streamparameters, 0, sizeof( PaStreamParameters ) );
	(void)argc;
#if (defined(_WIN32) || defined(WIN32)) && (defined(_UNICODE) || defined(UNICODE))
	file = _wfopen( argv[1], L"rb" );
#else
	file = fopen( argv[1], "rb" );
#endif
	mod = openmpt_module_create( openmpt_stream_get_file_callbacks(), file, NULL, NULL, NULL );
	fclose( file );
	Pa_Initialize();
	streamparameters.device = Pa_GetDefaultOutputDevice();
	streamparameters.channelCount = 2;
	streamparameters.sampleFormat = paInt16 | paNonInterleaved;
	streamparameters.suggestedLatency = Pa_GetDeviceInfo( streamparameters.device )->defaultHighOutputLatency;
	Pa_OpenStream( &stream, NULL, &streamparameters, SAMPLERATE, paFramesPerBufferUnspecified, 0, NULL, NULL );
	Pa_StartStream( stream );
	while ( 1 ) {
		count = openmpt_module_read_stereo( mod, SAMPLERATE, BUFFERSIZE, left, right );
		if ( count == 0 ) {
			break;
		}
		Pa_WriteStream( stream, buffers, (unsigned long)count );
	}
	Pa_StopStream( stream );
	Pa_CloseStream( stream );
	Pa_Terminate();
	openmpt_module_destroy( mod );
	return 0;
}
