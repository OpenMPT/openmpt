/*
 * openmpt123_sndfile.hpp
 * ----------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_SNDFILE_HPP
#define OPENMPT123_SNDFILE_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

#if defined(MPT_WITH_SNDFILE)

#include <sndfile.h>

namespace openmpt123 {
	
class sndfile_stream_raii : public write_buffers_interface {
private:
	commandlineflags flags;
	std::ostream & log;
	SNDFILE * sndfile;
	std::vector<float> interleaved_float_buffer;
	std::vector<std::int16_t> interleaved_int_buffer;
private:
	enum match_mode_enum {
		match_print,
		match_recurse,
		match_exact,
		match_better,
		match_any
	};
	int matched_result( const SF_FORMAT_INFO & format_info, const SF_FORMAT_INFO & subformat_info ) {
		if ( flags.verbose ) {
			log << "sndfile: using format '"
			    << format_info.name << " (" << format_info.extension << ")" << " / " << subformat_info.name
			    << "'"
			    << std::endl;
		}
		return ( format_info.format & SF_FORMAT_TYPEMASK ) | subformat_info.format;
	}
	int find_format( std::string extension, match_mode_enum match_mode ) {

		if ( match_mode == match_recurse ) {
			int result = 0;
			result = find_format( extension, match_exact );
			if ( result ) {
				return result;
			}
			result = find_format( extension, match_better );
			if ( result ) {
				return result;
			}
			result = find_format( extension, match_any );
			if ( result ) {
				return result;
			}
			if ( result ) {
				return result;
			}
			return 0;
		}

		int format = 0;
		int major_count;
		sf_command( 0, SFC_GET_FORMAT_MAJOR_COUNT, &major_count, sizeof( int ) );
		for ( int m = 0; m < major_count; m++ ) {

			SF_FORMAT_INFO format_info;
			format_info.format = m;
			sf_command( 0, SFC_GET_FORMAT_MAJOR, &format_info, sizeof( SF_FORMAT_INFO ) );
			format = format_info.format;

			int subtype_count;
			sf_command( 0, SFC_GET_FORMAT_SUBTYPE_COUNT, &subtype_count, sizeof( int ) );
			for ( int s = 0; s < subtype_count; s++ ) {

				SF_FORMAT_INFO subformat_info;
				subformat_info.format = s;
				sf_command( 0, SFC_GET_FORMAT_SUBTYPE, &subformat_info, sizeof( SF_FORMAT_INFO ) );
				format = ( format & SF_FORMAT_TYPEMASK ) | subformat_info.format;

				SF_INFO sfinfo;
				std::memset( &sfinfo, 0, sizeof( SF_INFO ) );
				sfinfo.channels = flags.channels;
				sfinfo.format = format;
				if ( sf_format_check( &sfinfo ) ) {

					switch ( match_mode ) {
					case match_print:
						log << "sndfile: "
						    << ( format_info.name ? format_info.name : "" ) << " (" << ( format_info.extension ? format_info.extension : "" ) << ")"
						    << " / "
						    << ( subformat_info.name ? subformat_info.name : "" )
						    << std::endl;
						break;
					case match_recurse:
						break;
					case match_exact:
						if ( extension == format_info.extension ) {
							if ( flags.use_float && ( format_info.format & SF_FORMAT_FLOAT ) ) {
								return matched_result( format_info, subformat_info );
							} else if ( !flags.use_float && ( format_info.format & SF_FORMAT_PCM_16 ) ) {
								return matched_result( format_info, subformat_info );
							}
						}
						break;
					case match_better:
						if ( extension == format_info.extension ) {
							if ( flags.use_float && ( format_info.format & ( SF_FORMAT_FLOAT | SF_FORMAT_DOUBLE ) ) ) {
								return matched_result( format_info, subformat_info );
							} else if ( !flags.use_float && ( format_info.format & ( SF_FORMAT_PCM_16 | SF_FORMAT_PCM_24 | SF_FORMAT_PCM_32 ) ) ) {
								return matched_result( format_info, subformat_info );
							}
						}
						break;
					case match_any:
						if ( extension == format_info.extension ) {
							return matched_result( format_info, subformat_info );
						}
						break;
					}

				}
			}
		}

		return 0;

	}
	void write_metadata_field( int str_type, const std::string & str ) {
		if ( !str.empty() ) {
			sf_set_string( sndfile, str_type, str.c_str() );
		}
	}
public:
	sndfile_stream_raii( const commandlineflags & flags_, std::ostream & log_ = std::cerr ) : flags(flags_), log(log_),sndfile(0) {
		if ( flags.verbose ) {
			find_format( "", match_print );
			log << std::endl;
		}
		int format = find_format( get_extension( flags.output_filename ), match_recurse );
		if ( !format ) {
			throw exception( "unknown file type" );
		}
		SF_INFO info;
		std::memset( &info, 0, sizeof( SF_INFO ) );
		info.samplerate = flags.samplerate;
		info.channels = flags.channels;
		info.format = format;
		sndfile = sf_open( flags.output_filename.c_str(), SFM_WRITE, &info );
	}
	~sndfile_stream_raii() {
		sf_close( sndfile );
		sndfile = 0;
	}
	void write_metadata( const openmpt::module & mod ) {
		write_metadata_field( SF_STR_TITLE, mod.get_metadata( "title" ) );
		write_metadata_field( SF_STR_ARTIST, mod.get_metadata( "author" ) );
		write_metadata_field( SF_STR_COMMENT, mod.get_metadata( "message" ) );
		write_metadata_field( SF_STR_SOFTWARE, append_software_tag( mod.get_metadata( "tracker" ) ) );
	}
	void write( const std::vector<float*> buffers, std::size_t frames ) {
		interleaved_float_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				interleaved_float_buffer.push_back( buffers[channel][frame] );
			}
		}
		sf_writef_float( sndfile, interleaved_float_buffer.data(), frames );
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
		interleaved_int_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				interleaved_int_buffer.push_back( buffers[channel][frame] );
			}
		}
		sf_writef_short( sndfile, interleaved_int_buffer.data(), frames );
	}
};

} // namespace openmpt123

#endif // MPT_WITH_SNDFILE

#endif // OPENMPT123_SNDFILE_HPP
