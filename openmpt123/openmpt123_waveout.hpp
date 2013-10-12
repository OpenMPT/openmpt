/*
 * openmpt123_waveout.hpp
 * ------------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_WAVEOUT_HPP
#define OPENMPT123_WAVEOUT_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

#if defined(WIN32)

namespace openmpt123 {

struct waveout_exception : public exception {
	waveout_exception() throw() : exception( "waveout" ) { }
};

class waveout_stream_raii : public write_buffers_interface {
private:
	HWAVEOUT waveout;
	std::size_t num_channels;
	std::size_t num_chunks;
	std::size_t frames_per_chunk;
	std::vector<WAVEHDR> waveheaders;
	std::vector<std::vector<char> > wavebuffers;
	std::deque<std::int16_t> sample_queue_int;
	std::deque<float> sample_queue_float;
public:
	waveout_stream_raii( const commandlineflags & flags )
		: waveout(NULL)
		, num_channels(0)
		, num_chunks(0)
		, frames_per_chunk(0)
	{
		WAVEFORMATEX wfx;
		ZeroMemory( &wfx, sizeof( wfx ) );
		wfx.wFormatTag = flags.use_float ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
		wfx.nChannels = flags.channels;
		wfx.nSamplesPerSec = flags.samplerate;
		wfx.wBitsPerSample = flags.use_float ? 32 : 16;
		wfx.nBlockAlign = ( wfx.wBitsPerSample / 8 ) * wfx.nChannels;
		wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
		wfx.cbSize = 0;
		waveOutOpen( &waveout, flags.device == -1 ? WAVE_MAPPER : flags.device, &wfx, 0, 0, CALLBACK_NULL );
		num_channels = flags.channels;
		std::size_t frames_per_buffer = flags.samplerate * flags.buffer / 1000;
		num_chunks = ( flags.buffer + 9 ) / 10;
		if ( num_chunks < 2 ) {
			num_chunks = 2;
		}
		frames_per_chunk = ( frames_per_buffer + num_chunks - 1 ) / num_chunks;
		waveheaders.resize( num_chunks );
		wavebuffers.resize( num_chunks );
		for ( std::size_t i = 0; i < num_chunks; ++i ) {
			wavebuffers[i].resize( wfx.nBlockAlign * frames_per_chunk );
			waveheaders[i] = WAVEHDR();
			waveheaders[i].lpData = wavebuffers[i].data();
			waveheaders[i].dwBufferLength = wavebuffers[i].size();
			waveheaders[i].dwFlags = 0;
			waveOutPrepareHeader( waveout, &waveheaders[i], sizeof( WAVEHDR ) );
		}
	}
	~waveout_stream_raii() {
		if ( waveout ) {
			waveOutReset( waveout );
			for ( std::size_t i = 0; i < num_chunks; ++i ) {
				waveheaders[i].dwBufferLength = wavebuffers[i].size();
				waveOutUnprepareHeader( waveout, &waveheaders[i], sizeof( WAVEHDR ) );
			}
			wavebuffers.clear();
			waveheaders.clear();
			frames_per_chunk = 0;
			num_chunks = 0;
			waveOutClose( waveout );
			waveout = NULL;
		}
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) {
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < buffers.size(); ++channel ) {
				sample_queue_float.push_back( buffers[channel][frame] );
			}
		}
		while ( sample_queue_float.size() / num_channels >= frames_per_chunk ) {
			bool chunk_found = false;
			std::size_t this_chunk = 0;
			for ( std::size_t chunk = 0; chunk < num_chunks; ++chunk ) {
				DWORD flags = waveheaders[chunk].dwFlags;
				if ( !(flags & WHDR_INQUEUE) || (flags & WHDR_DONE) ) {
					this_chunk = chunk;
					chunk_found = true;
					break;
				}
			}
			if ( !chunk_found ) {
				Sleep( 1 );
			} else {
				for ( std::size_t sample = 0; sample < frames_per_chunk * num_channels; ++ sample ) {
					float val = sample_queue_float.front();
					sample_queue_float.pop_front();
					std::memcpy( wavebuffers[this_chunk].data() + ( sample * sizeof( float ) ), &val, sizeof( float ) );
				}
				waveOutWrite( waveout, &waveheaders[this_chunk], sizeof( WAVEHDR ) );
			}
		}
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < buffers.size(); ++channel ) {
				sample_queue_int.push_back( buffers[channel][frame] );
			}
		}
		while ( sample_queue_float.size() / num_channels >= frames_per_chunk ) {
			bool chunk_found = false;
			std::size_t this_chunk = 0;
			for ( std::size_t chunk = 0; chunk < num_chunks; ++chunk ) {
				DWORD flags = waveheaders[chunk].dwFlags;
				if ( !(flags & WHDR_INQUEUE) || (flags & WHDR_DONE) ) {
					this_chunk = chunk;
					chunk_found = true;
					break;
				}
			}
			if ( !chunk_found ) {
				Sleep( 1 );
			} else {
				for ( std::size_t sample = 0; sample < frames_per_chunk * num_channels; ++ sample ) {
					std::int16_t val = sample_queue_int.front();
					sample_queue_int.pop_front();
					std::memcpy( wavebuffers[this_chunk].data() + ( sample * sizeof( std::int16_t ) ), &val, sizeof( std::int16_t ) );
				}
				waveOutWrite( waveout, &waveheaders[this_chunk], sizeof( WAVEHDR ) );
			}
		}
	}
};

static void show_waveout_devices() {
	std::ostringstream devices;
	devices << " Available devices:" << std::endl;
	devices << "    " << "stdout" << ": " << "use standard output" << std::endl;
	devices << "    " << "default" << ": " << "default" << std::endl;
	for ( UINT i = 0; i < waveOutGetNumDevs(); ++i ) {
		devices << "    " << i << ": ";
		WAVEOUTCAPSW caps;
		ZeroMemory( &caps, sizeof( caps ) );
		waveOutGetDevCapsW( i, &caps, sizeof( caps ) );
		devices << wstring_to_utf8( caps.szPname );
		devices << std::endl;
	}
	throw show_help_exception( devices.str() );
}

} // namespace openmpt123

#endif // WIN32

#endif // OPENMPT123_WAVEOUT_HPP
