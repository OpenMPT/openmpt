/*
 * openmpt123_sdl.hpp
 * ------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_SDL_HPP
#define OPENMPT123_SDL_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

#if defined(MPT_WITH_SDL)

#include <SDL.h>
#undef main
#undef SDL_main

namespace openmpt123 {

struct sdl_exception : public exception {
	sdl_exception( int /*code*/ ) throw() : exception( "SDL error" ) { }
};

class sdl_stream_raii : public write_buffers_blocking_wrapper {
private:
	std::size_t channels;
protected:
	void check_sdl_error( int e ) {
		if ( e < 0 ) {
			throw sdl_exception( e );
			return;
		}
	}
public:
	sdl_stream_raii( const commandlineflags & flags )
		: write_buffers_blocking_wrapper(flags)
		, channels(flags.channels)
	{
		double bufferSeconds = flags.buffer * 0.001;
		check_sdl_error( SDL_InitSubSystem( SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_AUDIO ) );
		SDL_AudioSpec audiospec;
		std::memset( &audiospec, 0, sizeof( SDL_AudioSpec ) );
		audiospec.freq = flags.samplerate;
		audiospec.format = AUDIO_S16SYS;
		audiospec.channels = flags.channels;
		audiospec.silence = 0;
		audiospec.samples = bufferSeconds * 0.5 * flags.samplerate;
		audiospec.size = audiospec.samples * audiospec.channels * sizeof( std::int16_t );
		audiospec.callback = &sdl_callback_wrapper;
		audiospec.userdata = this;
		check_sdl_error( SDL_OpenAudio( &audiospec, NULL ) );
		set_queue_size_frames( static_cast<std::size_t>( bufferSeconds * 0.5 * flags.samplerate ) );
		SDL_PauseAudio( 0 );
	}
	~sdl_stream_raii() {
		SDL_PauseAudio( 1 );
		SDL_CloseAudio();
		SDL_QuitSubSystem( SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_AUDIO );
	}
private:
	static void sdl_callback_wrapper( void * userdata, Uint8 * stream, int len ) {
		return reinterpret_cast<sdl_stream_raii*>( userdata )->sdl_callback( stream, len );
	}
	void sdl_callback( Uint8 * stream, int len ) {
		fill_buffer( reinterpret_cast<std::int16_t*>( stream ), len / sizeof( std::int16_t ) / channels );
	}
public:
	bool pause() {
		SDL_PauseAudio( 1 );
		return true;
	}
	bool unpause() {
		SDL_PauseAudio( 0 );
		return true;
	}
	void lock() {
		SDL_LockAudio();
	}
	void unlock() {
		SDL_UnlockAudio();
	}
	bool sleep( int ms ) {
		SDL_Delay( ms );
		return true;
	}
};

} // namespace openmpt123

#endif // MPT_WITH_SDL

#endif // OPENMPT123_SDL_HPP

