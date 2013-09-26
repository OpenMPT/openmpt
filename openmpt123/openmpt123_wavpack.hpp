/*
 * openmpt123_wavpack.hpp
 * ----------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_WAVPACK_HPP
#define OPENMPT123_WAVPACK_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

#if defined(MPT_WITH_WAVPACK)

#include <wavpack/wavpack.h>

namespace openmpt123 {

#define CHECKED(x) do { \
	int err = x ; \
	if ( !err ) { \
		throw exception( WavpackGetErrorMessage( wpc ) ); \
	} \
} while(0)

class wavpack_stream_raii : public file_audio_stream_base {
private:
	commandlineflags flags;
	std::ofstream file;
	WavpackContext * wpc;
	WavpackConfig wpconfig;
	bool first_block_seen;
	std::streampos first_block_pos;
	std::vector<char> first_block;
	std::vector<int32_t> interleaved_buffer;
	static int write_block( void * id, void * data, int32_t length ) {
		return reinterpret_cast<wavpack_stream_raii*>(id)->write_block( reinterpret_cast<const char*>(data), length ) ? 1 : 0;
	}
	bool write_block( const char * data, std::size_t length ) {
		if ( !first_block_seen ) {
			first_block_pos = file.tellp();
			first_block = std::vector<char>( data, data + length );
			first_block_seen = true;
		}
		file.write( data, length );
		if ( !file ) {
			return false;
		}
		return true;
	}
public:
	wavpack_stream_raii( const std::string & filename, const commandlineflags & flags_ )
		: flags(flags_)
		, file(filename.c_str(), std::ios::binary | std::ios::trunc)
		, wpc(NULL)
		, wpconfig()
		, first_block_seen(false)
		, first_block_pos(0)
	{
		wpc = WavpackOpenFileOutput( write_block, this, NULL );
		wpconfig.bytes_per_sample = flags.use_float ? 4 : 2;
		wpconfig.bits_per_sample = flags.use_float ? 32 : 16;
		if ( flags.channels == 1 ) {
			wpconfig.channel_mask = (1<<2);
		} else if ( flags.channels == 2 ) {
			wpconfig.channel_mask = (1<<0)|(1<<1);
		} else if ( flags.channels == 3 ) {
			wpconfig.channel_mask = (1<<0)|(1<<1)|(1<<8);
		} else if ( flags.channels == 4 ) {
			wpconfig.channel_mask = (1<<0)|(1<<1)|(1<<4)|(1<<5);
		} else {
			wpconfig.channel_mask = 0;
		}
		wpconfig.num_channels = flags.channels;
		wpconfig.sample_rate = flags.samplerate;
		wpconfig.flags = flags.use_float ? CONFIG_SKIP_WVX : 0;
		wpconfig.float_norm_exp = flags.use_float ? 127 : 0;
		CHECKED( WavpackSetConfiguration( wpc, &wpconfig, -1 ) );
		CHECKED( WavpackPackInit( wpc ) );
	}
	~wavpack_stream_raii() {
		CHECKED( WavpackFlushSamples( wpc  ) );
		if ( first_block_seen ) {
			WavpackUpdateNumSamples( wpc, reinterpret_cast<void*>( first_block.data() ) );
			std::streampos endpos = file.tellp();
			file.seekp( first_block_pos );
			file.write( first_block.data(), first_block.size() );
			file.seekp( endpos );
		}
		WavpackCloseFile( wpc );
		wpc = NULL;
	}
	void write( const std::vector<float*> buffers, std::size_t frames ) {
		interleaved_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				union {
					std::uint32_t i;
					float f;
				} conv;
				conv.f = buffers[channel][frame];
				interleaved_buffer.push_back( conv.i );
			}
		}
		CHECKED( WavpackPackSamples( wpc, interleaved_buffer.data(), frames ) );
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
		interleaved_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				interleaved_buffer.push_back( buffers[channel][frame] );
			}
		}
		CHECKED( WavpackPackSamples( wpc, interleaved_buffer.data(), frames ) );
	}
};

#undef CHECKED

} // namespace openmpt123

#endif // MPT_WITH_WAVPACK

#endif // OPENMPT123_WAVPACK_HPP
