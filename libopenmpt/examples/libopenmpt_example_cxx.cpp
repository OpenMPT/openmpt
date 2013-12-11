/*
 * libopenmpt_example_cxx.cpp
 * --------------------------
 * Purpose: libopenmpt C++ API simple example
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include <fstream>
#include <vector>

#include <cstring>

#include <libopenmpt/libopenmpt.hpp>

#include <portaudio.h>

int main( int /*argc*/, char * argv [] ) {
	const std::size_t buffersize = 480;
	const std::int32_t samplerate = 48000;
	std::vector<float> left( buffersize );
	std::vector<float> right( buffersize );
	std::ifstream file( argv[1], std::ios::binary );
	openmpt::module mod( file );
	Pa_Initialize();
	PaStream * stream = 0;
	PaStreamParameters streamparameters;
	std::memset( &streamparameters, 0, sizeof(PaStreamParameters) );
	streamparameters.device = Pa_GetDefaultOutputDevice();
	streamparameters.channelCount = 2;
	streamparameters.sampleFormat = paFloat32 | paNonInterleaved;
	streamparameters.suggestedLatency = Pa_GetDeviceInfo( streamparameters.device )->defaultHighOutputLatency;
	Pa_OpenStream( &stream, NULL, &streamparameters, samplerate, paFramesPerBufferUnspecified, 0, NULL, NULL );
	Pa_StartStream( stream );
	while ( true ) {
		std::size_t count = mod.read( samplerate, buffersize, left.data(), right.data() );
		if ( count == 0 ) {
			break;
		}
		const float * const buffers [2] = { left.data(), right.data() };
		Pa_WriteStream( stream, buffers, count );
	}
	Pa_StopStream( stream );
	Pa_CloseStream( stream );
	Pa_Terminate();
	return 0;
}
