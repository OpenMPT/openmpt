/*
 * openmpt123_portaudio.hpp
 * ------------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_PORTAUDIO_HPP
#define OPENMPT123_PORTAUDIO_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

#if defined(MPT_WITH_PORTAUDIO)

#include <portaudio.h>

namespace openmpt123 {

struct portaudio_exception : public exception {
	portaudio_exception( PaError code ) throw() : exception( Pa_GetErrorText( code ) ) { }
};

typedef void (*PaUtilLogCallback ) (const char *log);
#ifdef _MSC_VER
extern "C" void PaUtil_SetDebugPrintFunction(PaUtilLogCallback  cb);
#endif

class portaudio_raii {
private:
	bool log_set;
	bool portaudio_initialized;
	static std::ostream * portaudio_log_stream;
private:
	static void portaudio_log_function( const char * log ) {
		if ( portaudio_log_stream ) {
			*portaudio_log_stream << "PortAudio: " << log;
		}
	}
protected:
	void check_portaudio_error( PaError e, std::ostream & log = std::cerr ) {
		if ( e == paNoError ) {
			return;
		}
		if ( e == paOutputUnderflowed ) {
			log << "PortAudio warning: " << Pa_GetErrorText( e ) << std::endl;
			return;
		}
		throw portaudio_exception( e );
	}
public:
	portaudio_raii( bool verbose, std::ostream & log = std::cerr ) : log_set(false), portaudio_initialized(false) {
		if ( verbose ) {
			portaudio_log_stream = &log;
		} else {
			portaudio_log_stream = 0;
		}
#ifdef _MSC_VER
		PaUtil_SetDebugPrintFunction( portaudio_log_function );
		log_set = true;
#endif
		check_portaudio_error( Pa_Initialize() );
		portaudio_initialized = true;
		if ( verbose ) {
			*portaudio_log_stream << std::endl;
		}
	}
	~portaudio_raii() {
		if ( portaudio_initialized ) {
			check_portaudio_error( Pa_Terminate() );
			portaudio_initialized = false;
		}
		if ( log_set ) {
#ifdef _MSC_VER
			PaUtil_SetDebugPrintFunction( NULL );
			log_set = false;
#endif
		}
		portaudio_log_stream = 0;
	}
};

std::ostream * portaudio_raii::portaudio_log_stream = 0;

class portaudio_stream_raii : public portaudio_raii, public write_buffers_interface {
private:
	PaStream * stream;
public:
	portaudio_stream_raii( const commandlineflags & flags, std::ostream & log = std::cerr ) : portaudio_raii(flags.verbose, log), stream(NULL) {
		PaStreamParameters streamparameters;
		std::memset( &streamparameters, 0, sizeof(PaStreamParameters) );
		streamparameters.device = ( flags.device == -1 ) ? Pa_GetDefaultOutputDevice() : flags.device;
		streamparameters.channelCount = flags.channels;
		streamparameters.sampleFormat = ( flags.use_float ? paFloat32 : paInt16 ) | paNonInterleaved;
		streamparameters.suggestedLatency = flags.buffer * 0.001;
		//check_portaudio_error( Pa_OpenStream( &stream, NULL, &streamparameters, flags.samplerate, flags.use_ui ? 20 * flags.samplerate / 1000 : paFramesPerBufferUnspecified, 0, NULL, NULL ) );
		check_portaudio_error( Pa_OpenStream( &stream, NULL, &streamparameters, flags.samplerate, paFramesPerBufferUnspecified, 0, NULL, NULL ) );
		check_portaudio_error( Pa_StartStream( stream ) );
		if ( flags.verbose ) {
			log << "PortAudio:" << std::endl;
			log << " device: "
				<< streamparameters.device
				<< " [ " << Pa_GetHostApiInfo( Pa_GetDeviceInfo( streamparameters.device )->hostApi )->name << " / " << Pa_GetDeviceInfo( streamparameters.device )->name << " ] "
				<< std::endl;
			log << " channels: " << streamparameters.channelCount << std::endl;
			log << " sampleformat: " << ( ( streamparameters.sampleFormat == ( paFloat32 | paNonInterleaved ) ) ? "paFloat32" : "paInt16" ) << std::endl;
			log << " latency: " << streamparameters.suggestedLatency << std::endl;
			log << " samplerate: " << flags.samplerate << std::endl;
			log << std::endl;
		}
	}
	~portaudio_stream_raii() {
		if ( stream ) {
			check_portaudio_error( Pa_StopStream( stream ) );
			check_portaudio_error( Pa_CloseStream( stream ) );
			stream = NULL;
		}
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) {
		while ( frames > 0 ) {
			unsigned long chunk_frames = static_cast<unsigned long>( std::min<std::size_t>( frames, std::numeric_limits<unsigned long>::max() ) );
			check_portaudio_error( Pa_WriteStream( stream, buffers.data(), chunk_frames ) );
			frames -= chunk_frames;
		}
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
		while ( frames > 0 ) {
			unsigned long chunk_frames = static_cast<unsigned long>( std::min<std::size_t>( frames, std::numeric_limits<unsigned long>::max() ) );
			check_portaudio_error( Pa_WriteStream( stream, buffers.data(), chunk_frames ) );
			frames -= chunk_frames;
		}
	}
};

static void show_audio_devices() {
	std::ostringstream devices;
	devices << " Available devices:" << std::endl;
	portaudio_raii portaudio( false );
	devices << "    " << "stdout" << ": " << "use standard output" << std::endl;
	devices << "    " << "default" << ": " << "default" << std::endl;
	for ( PaDeviceIndex i = 0; i < Pa_GetDeviceCount(); ++i ) {
		if ( Pa_GetDeviceInfo( i ) && Pa_GetDeviceInfo( i )->maxOutputChannels > 0 ) {
			devices << "    " << i << ": ";
			bool first = true;
			if ( Pa_GetHostApiInfo( Pa_GetDeviceInfo( i )->hostApi ) && Pa_GetHostApiInfo( Pa_GetDeviceInfo( i )->hostApi )->name ) {
				if ( first ) {
					first = false;
				} else {
					devices << " - ";
				}
				devices << Pa_GetHostApiInfo( Pa_GetDeviceInfo( i )->hostApi )->name;
			}
			if ( Pa_GetDeviceInfo( i )->name ) {
				if ( first ) {
					first = false;
				} else {
					devices << " - ";
				}
				devices << Pa_GetDeviceInfo( i )->name;
			}
			devices << std::endl;
		}
	}
	throw show_help_exception( devices.str() );
}

} // namespace openmpt123

#endif // MPT_WITH_PORTAUDIO

#endif // OPENMPT123_PORTAUDIO_HPP
