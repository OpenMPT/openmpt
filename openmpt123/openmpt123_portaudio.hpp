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

#if defined(MPT_PORTAUDIO_CALLBACK)

class write_buffers_blocking_wrapper : public write_buffers_interface {
protected:
	std::size_t channels;
	std::size_t sampleQueueMaxFrames;
	std::deque<float> sampleQueue;
private:
	template < typename Tsample > static Tsample convert_to( float val );
	template < > static float convert_to( float val ) {
		return val;
	}
	template < > static std::int16_t convert_to( float val ) {
		std::int32_t tmp = static_cast<std::int32_t>( val * 32768.0f );
		tmp = std::min( tmp, 32767 );
		tmp = std::max( tmp, -32768 );
		return static_cast<std::int16_t>( tmp );
	}
protected:
	write_buffers_blocking_wrapper( const commandlineflags & flags )
		: channels(flags.channels)
		, sampleQueueMaxFrames(0)
	{
		return;
	}
	void set_queue_size_frames( std::size_t frames ) {
		sampleQueueMaxFrames = frames;
	}
	template < typename Tsample >
	void fill_buffer( Tsample * buf, std::size_t framesToRender ) {
		lock();
		for ( std::size_t frame = 0; frame < framesToRender; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				float val = 0.0f;
				if ( !sampleQueue.empty() ) {
					val = sampleQueue.front();
					sampleQueue.pop_front();
				}
				*buf = convert_to<Tsample>( val );
				buf++;
			}
		}
		unlock();
	}
private:
	void wait_for_queue_space() {
		while ( sampleQueue.size() >= sampleQueueMaxFrames * channels ) {
			unlock();
			Pa_Sleep( 1 );
			lock();
		}
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) {
		lock();
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				wait_for_queue_space();
				sampleQueue.push_back( buffers[channel][frame] );
			}
		}
		unlock();
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
		lock();
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				wait_for_queue_space();
				sampleQueue.push_back( buffers[channel][frame] * (1.0f/32768.0f) );
			}
		}
		unlock();
	}
	virtual void lock() = 0;
	virtual void unlock() = 0;
};

namespace mpt {

#if defined(WIN32)

class mutex {
private:
	CRITICAL_SECTION impl;
public:
	mutex() { InitializeCriticalSection(&impl); }
	~mutex() { DeleteCriticalSection(&impl); }
	void lock() { EnterCriticalSection(&impl); }
	void unlock() { LeaveCriticalSection(&impl); }
};

#else

class mutex {
private:
	pthread_mutex_t impl;
public:
	mutex() {
		pthread_mutexattr_t attr;
		pthread_mutexattr_init( &attr );
		pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_NORMAL );
		pthread_mutex_init( &impl, &attr );
		pthread_mutexattr_destroy( &attr );
	}
	~mutex() { pthread_mutex_destroy( &impl ); }
	void lock() { pthread_mutex_lock( &impl ); }
	void unlock() { pthread_mutex_unlock( &impl ); }
};

#endif

} // namespace mpt

class portaudio_stream_callback_raii : public portaudio_raii, public write_buffers_blocking_wrapper {
private:
	PaStream * stream;
	mpt::mutex audioMutex;
	bool use_float;
public:
	portaudio_stream_callback_raii( const commandlineflags & flags, std::ostream & log = std::cerr )
		: portaudio_raii(flags.verbose, log)
		, write_buffers_blocking_wrapper(flags)
		, stream(NULL)
		, use_float(flags.use_float)
	{
		PaStreamParameters streamparameters;
		std::memset( &streamparameters, 0, sizeof(PaStreamParameters) );
		streamparameters.device = ( flags.device == -1 ) ? Pa_GetDefaultOutputDevice() : flags.device;
		streamparameters.channelCount = flags.channels;
		streamparameters.sampleFormat = ( flags.use_float ? paFloat32 : paInt16 );
		streamparameters.suggestedLatency = flags.buffer * 0.001;
		check_portaudio_error( Pa_OpenStream( &stream, NULL, &streamparameters, flags.samplerate, paFramesPerBufferUnspecified, 0, &portaudio_callback_wrapper, this ) );
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
		if ( bufferSeconds <= ( flags.ui_redraw_interval * 0.001 )  ) {
			bufferSeconds = flags.ui_redraw_interval * 0.001;
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
		return reinterpret_cast<portaudio_stream_threaded_raii*>( userData )->portaudio_callback( input, output, frameCount, timeInfo, statusFlags );
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
	void lock() {
		audioMutex.lock();
	}
	void unlock() {
		audioMutex.unlock();
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
	portaudio_stream_blocking_raii( const commandlineflags & flags, std::ostream & log = std::cerr )
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
		streamparameters.suggestedLatency = flags.buffer * 0.001;
		PaError e = PaError();
		e = Pa_OpenStream( &stream, NULL, &streamparameters, flags.samplerate, ( flags.mode == ModeUI ) ? flags.ui_redraw_interval * flags.samplerate / 1000 : paFramesPerBufferUnspecified, 0, NULL, NULL );
		if ( e != paNoError ) {
			// Non-interleaved failed, try interleaved next.
			// This might help broken portaudio on MacOS X.
			streamparameters.sampleFormat &= ~paNonInterleaved;
			e = Pa_OpenStream( &stream, NULL, &streamparameters, flags.samplerate, ( flags.mode == ModeUI ) ? flags.ui_redraw_interval * flags.samplerate / 1000 : paFramesPerBufferUnspecified, 0, NULL, NULL );
			if ( e == paNoError ) {
				interleaved = true;
			}
			check_portaudio_error( e );
		}
		check_portaudio_error( Pa_StartStream( stream ) );
		if ( flags.verbose ) {
			log << "PortAudio:" << std::endl;
			log << " device: "
				<< streamparameters.device
				<< " [ " << Pa_GetHostApiInfo( Pa_GetDeviceInfo( streamparameters.device )->hostApi )->name << " / " << Pa_GetDeviceInfo( streamparameters.device )->name << " ] "
				<< std::endl;
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
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) {
		if ( interleaved ) {
			sampleBufFloat.clear();
			for ( std::size_t frame = 0; frame < frames; ++frame ) {
				for ( std::size_t channel = 0; channel < channels; ++channel ) {
					sampleBufFloat.push_back( buffers[channel][frame] );
				}
			}
			Pa_WriteStream( stream, sampleBufFloat.data(), frames );
		} else {
			while ( frames > 0 ) {
				unsigned long chunk_frames = static_cast<unsigned long>( std::min<std::size_t>( frames, std::numeric_limits<unsigned long>::max() ) );
				check_portaudio_error( Pa_WriteStream( stream, buffers.data(), chunk_frames ) );
				frames -= chunk_frames;
			}
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
			Pa_WriteStream( stream, sampleBufInt.data(), frames );
		} else {
			while ( frames > 0 ) {
				unsigned long chunk_frames = static_cast<unsigned long>( std::min<std::size_t>( frames, std::numeric_limits<unsigned long>::max() ) );
				check_portaudio_error( Pa_WriteStream( stream, buffers.data(), chunk_frames ) );
				frames -= chunk_frames;
			}
		}
	}
};

#define portaudio_stream_raii portaudio_stream_blocking_raii

#endif

static void show_portaudio_devices() {
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
