/*
 * openmpt123_flac.hpp
 * -------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_FLAC_HPP
#define OPENMPT123_FLAC_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

#if defined(MPT_WITH_FLAC)

#include <FLAC++/encoder.h>
#include <FLAC/metadata.h>
#include <FLAC/format.h>

namespace openmpt123 {
	
class flac_stream_raii : public write_buffers_interface {
private:
	commandlineflags flags;
	bool called_init;
	std::vector< std::pair< std::string, std::string > > tags;
	FLAC__StreamMetadata * metadata[1];
	FLAC__StreamMetadata_VorbisComment_Entry entry;
	FLAC::Encoder::File encoder;
	std::vector<FLAC__int32> interleaved_buffer;
	void add_vorbiscomment_field( FLAC__StreamMetadata * vorbiscomment, const std::string & field, const std::string & value ) {
		if ( !value.empty() ) {
			FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair( &entry, field.c_str(), value.c_str() );
			FLAC__metadata_object_vorbiscomment_append_comment( vorbiscomment, entry, true );
		}
	}
public:
	flac_stream_raii( const commandlineflags & flags_ ) : flags(flags_), called_init(false) {
		metadata[0] = 0;
		encoder.set_channels( flags.channels );
		encoder.set_bits_per_sample( flags.use_float ? 24 : 16 );
		encoder.set_sample_rate( flags.samplerate );
		encoder.set_compression_level( 8 );
	}
	~flac_stream_raii() {
		encoder.finish();
		FLAC__metadata_object_delete( metadata[0] );
	}
	void write_metadata( const openmpt::module & mod ) {
		if ( called_init ) {
			return;
		}
		tags.clear();
		tags.push_back( std::make_pair<std::string, std::string>( "TITLE", mod.get_metadata( "title" ) ) );
		tags.push_back( std::make_pair<std::string, std::string>( "ARTIST", mod.get_metadata( "author" ) ) );
		tags.push_back( std::make_pair<std::string, std::string>( "COMMENTS", mod.get_metadata( "message" ) ) );
		tags.push_back( std::make_pair<std::string, std::string>( "ENCODING", append_software_tag( mod.get_metadata( "tracker" ) ) ) );
		metadata[0] = FLAC__metadata_object_new( FLAC__METADATA_TYPE_VORBIS_COMMENT );
		for ( std::vector< std::pair< std::string, std::string > >::iterator tag = tags.begin(); tag != tags.end(); ++tag ) {
			add_vorbiscomment_field( metadata[0], tag->first, tag->second );
		}
		encoder.set_metadata( metadata, 1 );
	}
	void write( const std::vector<float*> buffers, std::size_t frames ) {
		if ( !called_init ) {
			encoder.init( flags.output_filename );
			called_init = true;
		}
		interleaved_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				float in = buffers[channel][frame];
				if ( in <= -1.0f ) {
					in = -1.0f;
				} else if ( in >= 1.0f ) {
					in = 1.0f;
				}
				FLAC__int32 out = mpt_lround( in * (1<<23) );
				out = std::max( 0 - (1<<23), out );
				out = std::min( out, 0 + (1<<23) - 1 );
				interleaved_buffer.push_back( out );
			}
		}
		encoder.process_interleaved( interleaved_buffer.data(), frames );
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
		if ( !called_init ) {
			encoder.init( flags.output_filename );
			called_init = true;
		}
		interleaved_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				interleaved_buffer.push_back( buffers[channel][frame] );
			}
		}
		encoder.process_interleaved( interleaved_buffer.data(), frames );
	}
};

} // namespace openmpt123

#endif // MPT_WITH_FLAC

#endif // OPENMPT123_FLAC_HPP
