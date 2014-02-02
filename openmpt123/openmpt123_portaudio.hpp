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
	std::ostream & log;
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
	void check_portaudio_error( PaError e ) {
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
	portaudio_raii( bool verbose, std::ostream & log ) : log(log), log_set(false), portaudio_initialized(false) {
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

#if defined(MPT_PORTAUDIO_CALLBACK)

class portaudio_stream_callback_raii : public portaudio_raii, public write_buffers_blocking_wrapper {
private:
	PaStream * stream;
	openmpt123::mutex audioMutex;
	bool use_float;
public:
	portaudio_stream_callback_raii( commandlineflags & flags, std::ostream & log )
		: portaudio_raii(flags.verbose, log)
		, write_buffers_blocking_wrapper(flags)
		, stream(NULL)
		, use_float(flags.use_float)
	{
		flags.apply_default_buffer_sizes();
		PaStreamParameters streamparameters;
		std::memset( &streamparameters, 0, sizeof(PaStreamParameters) );
		streamparameters.device = ( flags.device == -1 ) ? Pa_GetDefaultOutputDevice() : flags.device;
		streamparameters.channelCount = flags.channels;
		streamparameters.sampleFormat = ( flags.use_float ? paFloat32 : paInt16 );
		streamparameters.suggestedLatency = flags.buffer * 0.001;
		check_portaudio_error( Pa_OpenStream( &stream, NULL, &streamparameters, flags.samplerate, paFramesPerBufferUnspecified, ( flags.dither > 0 ) ? paNoFlag : paDitherOff, &portaudio_callback_wrapper, this ) );
		if ( flags.verbose ) {
			log << "PortAudio (Callback):" << std::endl;
			log << " device: "
				<< streamparameters.device
				<< " [ " << Pa_GetHostApiInfo( Pa_GetDeviceInfo( streamparameters.device )->hostApi )->name << " / " << Pa_GetDeviceInfo( streamparameters.device )->name << " ] "
				<< std::endl;
			log << " channels: " << streamparameters.channelCount << std::endl;
			log << " sampleformat: " << ( ( streamparameters.sampleFormat == paFloat32 ) ? "paFloat32" : "paInt16" ) << std::endl;
			log << " latency: " << Pa_GetStreamInfo( stream )->outputLatency << std::endl;
			log << " samplerate: " << Pa_GetStreamInfo( stream )->sampleRate << std::endl;
			log << std::endl;
		}
		double bufferSeconds = ( flags.buffer * 0.001 ) - Pa_GetStreamInfo( stream )->outputLatency;
		if ( bufferSeconds <= ( flags.period * 0.001 * 2.0 )  ) {
			bufferSeconds = flags.period * 0.001 * 2.0;
		}
		set_queue_size_frames( static_cast<std::size_t>( bufferSeconds * Pa_GetStreamInfo( stream )->sampleRate ) );
		check_portaudio_error( Pa_StartStream( stream ) );
	}
	~portaudio_stream_callback_raii() {
		if ( stream ) {
			check_portaudio_error( Pa_StopStream( stream ) );
			check_portaudio_error( Pa_CloseStream( stream ) );
			stream = NULL;
		}
	}
private:
	static int portaudio_callback_wrapper( const void * input, void * output, unsigned long frameCount, const PaStreamCallbackTimeInfo * timeInfo, PaStreamCallbackFlags statusFlags, void * userData ) {
		return reinterpret_cast<portaudio_stream_callback_raii*>( userData )->portaudio_callback( input, output, frameCount, timeInfo, statusFlags );
	}
	int portaudio_callback( const void * input, void * output, unsigned long frameCount, const PaStreamCallbackTimeInfo * timeInfo, PaStreamCallbackFlags statusFlags ) {
		lock();
		if ( use_float ) {
			fill_buffer( reinterpret_cast<float*>( output ), frameCount );
		} else {
			fill_buffer( reinterpret_cast<std::int16_t*>( output ), frameCount );
		}
		unlock();
		return paContinue;
	}
public:
	bool unpause() {
		check_portaudio_error( Pa_StartStream( stream ) );
		return true;
	}
	bool pause() {
		check_portaudio_error( Pa_StopStream( stream ) );
		return true;
	}
	void lock() {
		audioMutex.lock();
	}
	void unlock() {
		audioMutex.unlock();
	}
	bool sleep( int ms ) {
		Pa_Sleep( ms );
		return true;
	}
};

#define portaudio_stream_raii portaudio_stream_callback_raii

#else

class portaudio_stream_blocking_raii : public portaudio_raii, public write_buffers_interface {
private:
	PaStream * stream;
	bool interleaved;
	std::size_t channels;
	std::vector<float> sampleBufFloat;
	std::vector<std::int16_t> sampleBufInt;
public:
	portaudio_stream_blocking_raii( commandlineflags & flags, std::ostream & log )
		: portaudio_raii(flags.verbose, log)
		, stream(NULL)
		, interleaved(false)
		, channels(flags.channels)
	{
		PaStreamParameters streamparameters;
		std::memset( &streamparameters, 0, sizeof(PaStreamParameters) );
		streamparameters.device = ( flags.device == -1 ) ? Pa_GetDefaultOutputDevice() : flags.device;
		streamparameters.channelCount = flags.channels;
		streamparameters.sampleFormat = ( flags.use_float ? paFloat32 : paInt16 ) | paNonInterleaved;
		if ( flags.buffer == default_high ) {
			streamparameters.suggestedLatency = Pa_GetDeviceInfo( streamparameters.device )->defaultHighOutputLatency;
			flags.buffer = static_cast<std::int32_t>( Pa_GetDeviceInfo( streamparameters.device )->defaultHighOutputLatency * 1000.0 );
		} else if ( flags.buffer == default_low ) {
			streamparameters.suggestedLatency = Pa_GetDeviceInfo( streamparameters.device )->defaultLowOutputLatency;
			flags.buffer = static_cast<std::int32_t>( Pa_GetDeviceInfo( streamparameters.device )->defaultLowOutputLatency * 1000.0 );
		} else {
			streamparameters.suggestedLatency = flags.buffer * 0.001;
		}
		unsigned long framesperbuffer = 0;
		if ( flags.mode != ModeUI ) {
			framesperbuffer = paFramesPerBufferUnspecified;
			flags.period = 50;
			flags.period = std::min<std::int32_t>( flags.period, flags.buffer / 3 );
		} else if ( flags.period == default_high ) {
			framesperbuffer = paFramesPerBufferUnspecified;
			flags.period = 50;
			flags.period = std::min<std::int32_t>( flags.period, flags.buffer / 3 );
		} else if ( flags.period == default_low ) {
			framesperbuffer = paFramesPerBufferUnspecified;
			flags.period = 10;
			flags.period = std::min<std::int32_t>( flags.period, flags.buffer / 3 );
		} else {
			framesperbuffer = flags.period * flags.samplerate / 1000;
		}
		if ( flags.period <= 0 ) {
			flags.period = 1;
		}
		flags.apply_default_buffer_sizes();
		if ( flags.verbose ) {
			log << "PortAudio:" << std::endl;
			log << " device: "
				<< streamparameters.device
				<< " [ " << Pa_GetHostApiInfo( Pa_GetDeviceInfo( streamparameters.device )->hostApi )->name << " / " << Pa_GetDeviceInfo( streamparameters.device )->name << " ] "
				<< std::endl;
			log << " low latency: " << Pa_GetDeviceInfo( streamparameters.device )->defaultLowOutputLatency << std::endl;
			log << " high latency: " << Pa_GetDeviceInfo( streamparameters.device )->defaultHighOutputLatency << std::endl;
			log << " suggested latency: " << streamparameters.suggestedLatency << std::endl;
			log << " frames per buffer: " << framesperbuffer << std::endl;
			log << " ui redraw: " << flags.period << std::endl;
		}
		PaError e = PaError();
		e = Pa_OpenStream( &stream, NULL, &streamparameters, flags.samplerate, framesperbuffer, ( flags.dither > 0 ) ? paNoFlag : paDitherOff, NULL, NULL );
		if ( e != paNoError ) {
			// Non-interleaved failed, try interleaved next.
			// This might help broken portaudio on MacOS X.
			streamparameters.sampleFormat &= ~paNonInterleaved;
			e = Pa_OpenStream( &stream, NULL, &streamparameters, flags.samplerate, framesperbuffer, ( flags.dither > 0 ) ? paNoFlag : paDitherOff, NULL, NULL );
			if ( e == paNoError ) {
				interleaved = true;
			}
			check_portaudio_error( e );
		}
		check_portaudio_error( Pa_StartStream( stream ) );
		if ( flags.verbose ) {
			log << " channels: " << streamparameters.channelCount << std::endl;
			log << " sampleformat: " << ( ( ( streamparameters.sampleFormat & ~paNonInterleaved ) == paFloat32 ) ? "paFloat32" : "paInt16" ) << std::endl;
			log << " latency: " << Pa_GetStreamInfo( stream )->outputLatency << std::endl;
			log << " samplerate: " << Pa_GetStreamInfo( stream )->sampleRate << std::endl;
			log << std::endl;
		}
	}
	~portaudio_stream_blocking_raii() {
		if ( stream ) {
			check_portaudio_error( Pa_StopStream( stream ) );
			check_portaudio_error( Pa_CloseStream( stream ) );
			stream = NULL;
		}
	}
private:
	template<typename Tsample>
	void write_frames( const Tsample * buffer, std::size_t frames ) {
		while ( frames > 0 ) {
			unsigned long chunk_frames = static_cast<unsigned long>( std::min<std::size_t>( frames, std::numeric_limits<unsigned long>::max() ) );
			check_portaudio_error( Pa_WriteStream( stream, buffer, chunk_frames ) );
			buffer += chunk_frames * channels;
			frames -= chunk_frames;
		}
	}
	template<typename Tsample>
	void write_frames( std::vector<Tsample*> buffers, std::size_t frames ) {
		while ( frames > 0 ) {
			unsigned long chunk_frames = static_cast<unsigned long>( std::min<std::size_t>( frames, std::numeric_limits<unsigned long>::max() ) );
			check_portaudio_error( Pa_WriteStream( stream, buffers.data(), chunk_frames ) );
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				buffers[channel] += chunk_frames;
			}
			frames -= chunk_frames;
		}
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) {
		if ( interleaved ) {
			sampleBufFloat.clear();
			for ( std::size_t frame = 0; frame < frames; ++frame ) {
				for ( std::size_t channel = 0; channel < channels; ++channel ) {
					sampleBufFloat.push_back( buffers[channel][frame] );
				}
			}
			write_frames( sampleBufFloat.data(), frames );
		} else {
			write_frames( buffers, frames );
		}
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
		if ( interleaved ) {
			sampleBufInt.clear();
			for ( std::size_t frame = 0; frame < frames; ++frame ) {
				for ( std::size_t channel = 0; channel < channels; ++channel ) {
					sampleBufInt.push_back( buffers[channel][frame] );
				}
			}
			write_frames( sampleBufInt.data(), frames );
		} else {
			write_frames( buffers, frames );
		}
	}
	bool unpause() {
		check_portaudio_error( Pa_StartStream( stream ) );
		return true;
	}
	bool pause() {
		check_portaudio_error( Pa_StopStream( stream ) );
		return true;
	}
	bool sleep( int ms ) {
		Pa_Sleep( ms );
		return true;
	}
};

#define portaudio_stream_raii portaudio_stream_blocking_raii

#endif

static std::string show_portaudio_devices( std::ostream & log ) {
	std::ostringstream devices;
	devices << " portaudio:" << std::endl;
	portaudio_raii portaudio( false, log );
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
			if ( Pa_GetHostApiInfo( Pa_GetDeviceInfo( i )->hostApi ) ) {
				if ( i == Pa_GetHostApiInfo( Pa_GetDeviceInfo( i )->hostApi )->defaultOutputDevice ) {
					devices << " (default)";
				}
			}
			if ( Pa_GetDeviceInfo( i )->name ) {
				if ( first ) {
					first = false;
				} else {
					devices << " - ";
				}
				devices << Pa_GetDeviceInfo( i )->name;
			}
			devices << " (";
			devices << "high latency: " << Pa_GetDeviceInfo( i )->defaultHighOutputLatency;
			devices << ", ";
			devices << "low latency: " << Pa_GetDeviceInfo( i )->defaultLowOutputLatency;
			devices << ")";
			devices << std::endl;
		}
	}
	return devices.str();
}

} // namespace openmpt123

#endif // MPT_WITH_PORTAUDIO

#endif // OPENMPT123_PORTAUDIO_HPP
