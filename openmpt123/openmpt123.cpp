/*
 * openmpt123.cpp
 * --------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "openmpt123_config.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>

#if defined(_MSC_VER)
#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#else
#include <sys/poll.h>
#include <termios.h>
#include <unistd.h>
#endif

#include <libopenmpt/libopenmpt.hpp>

#include "openmpt123.hpp"

#include "openmpt123_flac.hpp"
#include "openmpt123_mmio.hpp"
#include "openmpt123_sndfile.hpp"
#include "openmpt123_stdout.hpp"
#include "openmpt123_portaudio.hpp"

namespace openmpt123 {

struct silent_exit_exception : public std::exception {
	silent_exit_exception() throw() { }
};

struct show_version_number_exception : public std::exception {
	show_version_number_exception() throw() { }
};

bool IsTerminal( int fd ) {
#if defined( _MSC_VER )
	return true
		&& ( _isatty( fd ) ? true : false )
		&& GetConsoleWindow() != NULL
		;
#else
	return isatty( fd ) ? true : false;
#endif
}

#if !defined( _MSC_VER )

static termios saved_attributes;

static void reset_input_mode() {
	tcsetattr( STDIN_FILENO, TCSANOW, &saved_attributes );
}

static void set_input_mode() {
	termios tattr;
	if ( !isatty( STDIN_FILENO ) ) {
		return;
	}
	tcgetattr( STDIN_FILENO, &saved_attributes );
	atexit( reset_input_mode );
	tcgetattr( STDIN_FILENO, &tattr );
	tattr.c_lflag &= ~( ICANON | ECHO );
	tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;
	tcsetattr( STDIN_FILENO, TCSAFLUSH, &tattr );
}

#endif

class file_audio_stream_raii : public file_audio_stream_base {
private:
	file_audio_stream_base * impl;
public:
	file_audio_stream_raii( const commandlineflags & flags, const std::string & filename, std::ostream & log )
		: impl(0)
	{
		if ( !flags.force_overwrite ) {
			std::ifstream testfile( filename, std::ios::binary );
			if ( testfile ) {
				throw exception( "file already exists" );
			}
		}
		if ( false ) {
			// nothing
#ifdef MPT_WITH_MMIO
		} else if ( flags.output_extension == "wav" ) {
			impl = new mmio_stream_raii( filename, flags );
#endif				
#ifdef MPT_WITH_FLAC
		} else if ( flags.output_extension == "flac" ) {
			impl = new flac_stream_raii( filename, flags );
#endif				
#ifdef MPT_WITH_SNDFILE
		} else {
			impl = new sndfile_stream_raii( filename, flags.output_extension, flags, log );
#endif
		}
		if ( !impl ) {
			throw show_help_exception();
		}
	}
	virtual ~file_audio_stream_raii() {
		if ( impl ) {
			delete impl;
			impl = 0;
		}
	}
	virtual void write_metadata( std::map<std::string,std::string> metadata ) {
		impl->write_metadata( metadata );
	}
	virtual void write_updated_metadata( std::map<std::string,std::string> metadata ) {
		impl->write_updated_metadata( metadata );
	}
	virtual void write( const std::vector<float*> buffers, std::size_t frames ) {
		impl->write( buffers, frames );
	}
	virtual void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
		impl->write( buffers, frames );
	}
};                                                                                                                

std::ostream & operator << ( std::ostream & s, const commandlineflags & flags ) {
	s << "Quiet: " << flags.quiet << std::endl;
	s << "Verbose: " << flags.verbose << std::endl;
	s << "Mode : " << mode_to_string( flags.mode ) << std::endl;
	s << "Show progress: " << flags.show_progress << std::endl;
	s << "Show peak meters: " << flags.show_meters << std::endl;
	s << "Show channel peak meters: " << flags.show_channel_meters << std::endl;
	s << "Show details: " << flags.show_details << std::endl;
	s << "Show message: " << flags.show_message << std::endl;
#ifdef MPT_WITH_PORTAUDIO
	s << "Device: " << flags.device << std::endl;
	s << "Buffer: " << flags.buffer << std::endl;
#endif
	s << "Samplerate: " << flags.samplerate << std::endl;
	s << "Channels: " << flags.channels << std::endl;
	s << "Float: " << flags.use_float << std::endl;
	s << "Gain: " << flags.gain / 100.0 << std::endl;
	s << "Stereo separation: " << flags.separation << std::endl;
	s << "Interpolation filter taps: " << flags.filtertaps << std::endl;
	s << "Volume ramping strength: " << flags.ramping << std::endl;
	s << "Repeat count: " << flags.repeatcount << std::endl;
	s << "Seek target: " << flags.seek_target << std::endl;
	s << "Standard output: " << flags.use_stdout << std::endl;
#if defined(MPT_WITH_FLAC) || defined(MPT_WITH_MMIO) || defined(MPT_WITH_SNDFILE)
	s << "Output filename: " << flags.output_filename << std::endl;
	s << "Force overwrite output file: " << flags.force_overwrite << std::endl;
#endif
	s << std::endl;
	s << "Files: " << std::endl;
	for ( std::vector<std::string>::const_iterator filename = flags.filenames.begin(); filename != flags.filenames.end(); ++filename ) {
		s << " " << *filename << std::endl;
	}
	s << std::endl;
	return s;
}

static std::string replace( std::string str, const std::string & oldstr, const std::string & newstr ) {
	std::size_t pos = 0;
	while ( ( pos = str.find( oldstr, pos ) ) != std::string::npos ) {
		str.replace( pos, oldstr.length(), newstr );
		pos += newstr.length();
	}
	return str;
}

#if defined( _MSC_VER )
static const char path_sep = '\\';
#else
static const char path_sep = '/';
#endif

static std::string get_filename( const std::string & filepath ) {
	if ( filepath.find_last_of( std::string(1,path_sep) ) == std::string::npos ) {
		return filepath;
	}
	return filepath.substr( filepath.find_last_of( std::string(1,path_sep) ) + 1 );
}

static std::string prepend_lines( std::string str, const std::string & prefix ) {
	if ( str.empty() ) {
		return str;
	}
	if ( str.substr( str.length() - 1, 1 ) == std::string("\n") ) {
		str = str.substr( 0, str.length() - 1 );
	}
	return replace( str, std::string("\n"), std::string("\n") + prefix );
}

static std::string bytes_to_string( std::uint64_t bytes ) {
	static const char * const suffixes[] = { "B", "KiB", "MiB", "GiB", "TiB", "PiB" };
	int offset = 0;
	while ( bytes > 9999 ) {
		bytes /= 1024;
		offset += 1;
		if ( offset == 5 ) {
			break;
		}
	}
	std::ostringstream result;
	result << bytes << suffixes[offset];
	return result.str();
}

static std::string seconds_to_string( double time ) {
	std::int64_t time_ms = static_cast<std::int64_t>( time * 1000 );
	std::int64_t milliseconds = time_ms % 1000;
	std::int64_t seconds = ( time_ms / 1000 ) % 60;
	std::int64_t minutes = ( time_ms / ( 1000 * 60 ) ) % 24;
	std::int64_t hours = ( time_ms / ( 1000 * 60 * 24 ) );
	std::ostringstream str;
	if ( hours > 0 ) {
		str << hours << ":";
	}
	str << std::setfill('0') << std::setw(2) << minutes;
	str << ":";
	str << std::setfill('0') << std::setw(2) << seconds;
	str << ".";
	str << std::setfill('0') << std::setw(3) << milliseconds;
	return str.str();
}

template < typename T >
std::string bytes_to_string( T bytes ) {
	std::ostringstream str;
	if ( bytes >= std::uint64_t(10) * 1024 * 1024 * 1024 ) {
		str << bytes / 1024 / 1024 / 1024 << "GiB";
	} else if ( bytes >= std::uint64_t(10) * 1024 * 1024 ) {
		str << bytes / 1024 / 1024 << "MiB";
	} else if ( bytes >= std::uint64_t(10) * 1024 ) {
		str << bytes / 1024 << "KiB";
	} else {
		str << bytes << "B";
	}
	return str.str();
}

static void show_info( std::ostream & log, bool verbose, bool modplug123 = false ) {
	log << ( modplug123 ? "modplug123 emulated by " : "" ) << "openmpt123" << " version " << OPENMPT123_VERSION_STRING << ", Copyright (c) 2013 OpenMPT developers (http://openmpt.org/)" << std::endl;
	log << " libopenmpt " << openmpt::string::get( openmpt::string::library_version ) << " (" << "OpenMPT " << openmpt::string::get( openmpt::string::core_version ) << ")" << std::endl;
	if ( !verbose ) {
		log << std::endl;
		return;
	}
	log << "  (built " << openmpt::string::get( openmpt::string::build ) << ")" << std::endl;
#ifdef MPT_WITH_PORTAUDIO
	log << " " << Pa_GetVersionText() << " (http://portaudio.com/)" << std::endl;
#endif
#ifdef MPT_WITH_FLAC
	log << " FLAC " << FLAC__VERSION_STRING << ", " << FLAC__VENDOR_STRING << ", API " << FLAC_API_VERSION_CURRENT << "." << FLAC_API_VERSION_REVISION << "." << FLAC_API_VERSION_AGE << " (https://xiph.org/flac/)" << std::endl;
#endif
#ifdef MPT_WITH_SNDFILE
	char sndfile_info[128];
	sf_command( 0, SFC_GET_LIB_VERSION, sndfile_info, sizeof( sndfile_info ) );
	log << " libsndfile " << sndfile_info << " (http://mega-nerd.com/libsndfile/)" << std::endl;
#endif
	log << std::endl;
}

static void show_version() {
	std::clog << OPENMPT123_VERSION_STRING << std::endl;
}

static std::string get_device_string( int device ) {
	if ( device == -1 ) {
		return "default";
	}
	std::ostringstream str;
	str << device;
	return str.str();
}

static void show_help( show_help_exception & e, bool verbose, bool modplug123 ) {
	show_info( std::clog, verbose, modplug123 );
	if ( modplug123 ) {
		std::clog << "Usage: modplug123 [options] file1 [file2] ..." << std::endl;
		std::clog << std::endl;
		std::clog << " -h, --help       Show help" << std::endl;
		std::clog << " -v, --version    Show version number and nothing else" << std::endl;
		if ( !e.longhelp ) {
			std::clog << std::endl;
			return;
		}
		std::clog << " -l               Loop song [default: " << commandlineflags().repeatcount << "]" << std::endl;
#ifdef MPT_WITH_PORTAUDIO
		std::clog << " -ao n            Set output device [default: " << commandlineflags().device << "]," << std::endl;
#endif
		std::clog << "                  use -ao help to show available devices" << std::endl;
	} else {
		std::clog << "Usage: openmpt123 [options] [--] file1 [file2] ..." << std::endl;
		std::clog << std::endl;
		std::clog << " -h, --help           Show help" << std::endl;
		std::clog << " -q, --quiet          Suppress non-error screen output" << std::endl;
		std::clog << " -v, --verbose        Show more screen output" << std::endl;
		std::clog << "     --version        Show version number and nothing else" << std::endl;
		std::clog << std::endl;
		std::clog << " Mode:" << std::endl;
		std::clog << "     --info           Display information about each file" << std::endl;
		std::clog << "     --ui             Interactively play each file" << std::endl;
		std::clog << "     --batch          Play each file" << std::endl;
		std::clog << "     --render         Render each file to PCM data" << std::endl;
		if ( !e.longhelp ) {
			std::clog << std::endl;
			return;
		}
		std::clog << std::endl;
		std::clog << "     --terminal-width n Assume terminal is n characters wide [default: " << commandlineflags().terminal_width << "]" << std::endl;
		std::clog << std::endl;
		std::clog << "     --[no-]progress  Show playback progress [default: " << commandlineflags().show_progress << "]" << std::endl;
		std::clog << "     --[no-]meters    Show peak meters [default: " << commandlineflags().show_meters << "]" << std::endl;
		std::clog << "     --[no-]channel-meters Show channel peak meters [default: " << commandlineflags().show_channel_meters << "]" << std::endl;
		std::clog << std::endl;
		std::clog << "     --[no-]details   Show song details [default: " << commandlineflags().show_details << "]" << std::endl;
		std::clog << "     --[no-]message   Show song message [default: " << commandlineflags().show_message << "]" << std::endl;
		std::clog << std::endl;
		std::clog << "     --samplerate n   Set samplerate to n Hz [default: " << commandlineflags().samplerate << "]" << std::endl;
		std::clog << "     --channels n     use n [1,2,4] output channels [default: " << commandlineflags().channels << "]" << std::endl;
		std::clog << "     --[no]-float     Output 32bit floating point instead of 16bit integer [default: " << commandlineflags().use_float << "]" << std::endl;
		std::clog << std::endl;
		std::clog << "     --gain n         Set output gain to n dB [default: " << commandlineflags().gain / 100.0 << "]" << std::endl;
		std::clog << "     --stereo n       Set stereo separation to n % [default: " << commandlineflags().separation << "]" << std::endl;
		std::clog << "     --filter n       Set interpolation filter taps to n [1,2,4,8] [default: " << commandlineflags().filtertaps << "]" << std::endl;
		std::clog << "     --ramping n      Set volume ramping strength n [0..5] [default: " << commandlineflags().ramping << "]" << std::endl;
		std::clog << std::endl;
		std::clog << "     --repeat n       Repeat song n times (-1 means forever) [default: " << commandlineflags().repeatcount << "]" << std::endl;
		std::clog << "     --seek n         Seek to n seconds on start [default: " << commandlineflags().seek_target << "]" << std::endl;
		std::clog << std::endl;
#ifdef MPT_WITH_PORTAUDIO
		std::clog << "     --device n       Set output device [default: " << get_device_string( commandlineflags().device ) << "]," << std::endl;
		std::clog << "                      use --device help to show available devices" << std::endl;
		std::clog << "     --buffer n       Set output buffer size to n ms [default: " << commandlineflags().buffer << "]" << std::endl;
#endif
		std::clog << "     --stdout         Write raw audio data to stdout [default: " << commandlineflags().use_stdout << "]" << std::endl;
#if defined(MPT_WITH_FLAC) || defined(MPT_WITH_MMIO) || defined(MPT_WITH_SNDFILE)
		std::clog << "     --output-type t  Use output format t when writing to a PCM file [default: " << commandlineflags().output_extension << "]" << std::endl;
		std::clog << " -o, --output f       Write PCM output to file f instead of streaming to audio device [default: " << commandlineflags().output_filename << "]" << std::endl;
		std::clog << "     --force          Force overwriting of output file [default: " << commandlineflags().force_overwrite << "]" << std::endl;
#endif
		std::clog << std::endl;
		std::clog << "     --               Interpret further arguments as filenames" << std::endl;
		std::clog << std::endl;
		std::clog << " Supported file formats: " << std::endl;
		std::clog << "    ";
		std::vector<std::string> extensions = openmpt::get_supported_extensions();
		bool first = true;
		for ( std::vector<std::string>::iterator i = extensions.begin(); i != extensions.end(); ++i ) {
			if ( first ) {
				first = false;
			} else {
				std::clog << ", ";
			}
			std::clog << *i;
		}
		std::clog << std::endl;
	}

	std::clog << std::endl;

	if ( e.message.size() > 0 ) {
		std::clog << e.message;
		std::clog << std::endl;
	}
}

template < typename Tmod >
static void apply_mod_settings( commandlineflags & flags, Tmod & mod ) {
	flags.separation = std::max( flags.separation,  0 );
	flags.filtertaps = std::max( flags.filtertaps,  1 );
	flags.ramping    = std::max( flags.ramping,    -1 );
	mod.set_render_param( openmpt::module::RENDER_MASTERGAIN_MILLIBEL, flags.gain );
	mod.set_render_param( openmpt::module::RENDER_STEREOSEPARATION_PERCENT, flags.separation );
	mod.set_render_param( openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH, flags.filtertaps );
	mod.set_render_param( openmpt::module::RENDER_VOLUMERAMPING_STRENGTH, flags.ramping );
}

struct prev_file { int count; prev_file( int c ) : count(c) { } };
struct next_file { int count; next_file( int c ) : count(c) { } };

template < typename Tmod >
static bool handle_keypress( int c, commandlineflags & flags, Tmod & mod ) {
	switch ( c ) {
		case 'q': throw silent_exit_exception(); break;
		case 'N': throw prev_file(10); break;
		case 'n': throw prev_file(1); break;
		case 'h': mod.set_position_seconds( mod.get_position_seconds() - 10.0 ); break;
		case 'j': mod.set_position_seconds( mod.get_position_seconds() - 1.0 ); break;
		case 'k': mod.set_position_seconds( mod.get_position_seconds() + 1.0 ); break;
		case 'l': mod.set_position_seconds( mod.get_position_seconds() + 10.0 ); break;
		case 'm': throw next_file(1); break;
		case 'M': throw next_file(10); break;
		case '3': flags.gain       -=100; apply_mod_settings( flags, mod ); break;
		case '4': flags.gain       +=100; apply_mod_settings( flags, mod ); break;
		case '5': flags.separation -=  5; apply_mod_settings( flags, mod ); break;
		case '6': flags.separation +=  5; apply_mod_settings( flags, mod ); break;
		case '7': flags.filtertaps -=  1; apply_mod_settings( flags, mod ); break;
		case '8': flags.filtertaps +=  1; apply_mod_settings( flags, mod ); break;
		case '9': flags.ramping    -=  1; apply_mod_settings( flags, mod ); break;
		case '0': flags.ramping    +=  1; apply_mod_settings( flags, mod ); break;
	}
	return true;
}

struct meter_channel {
	float peak;
	float clip;
	float hold;
	float hold_age;
	meter_channel()
		: peak(0.0f)
		, clip(0.0f)
		, hold(0.0f)
		, hold_age(0.0f)
	{
		return;
	}
};

struct meter_type {
	meter_channel channels[4];
};

static const float falloff_rate = 20.0f / 1.7f;

static void update_meter( meter_type & meter, const commandlineflags & flags, std::size_t count, const std::int16_t * const * buffers ) {
	float falloff_factor = std::pow( 10.0f, -falloff_rate / flags.samplerate / 20.0f );
	for ( int channel = 0; channel < flags.channels; ++channel ) {
		meter.channels[channel].peak = 0.0f;
		for ( std::size_t frame = 0; frame < count; ++frame ) {
			if ( meter.channels[channel].clip != 0.0f ) {
				meter.channels[channel].clip -= ( 1.0f / 2.0f ) * 1.0f / static_cast<float>( flags.samplerate );
				if ( meter.channels[channel].clip <= 0.0f ) {
					meter.channels[channel].clip = 0.0f;
				}
			}
			float val = std::fabs( buffers[channel][frame] / 32768.0f );
			if ( val >= 1.0f ) {
				meter.channels[channel].clip = 1.0f;
			}
			if ( val > meter.channels[channel].peak ) {
				meter.channels[channel].peak = val;
			}
			meter.channels[channel].hold *= falloff_factor;
			if ( val > meter.channels[channel].hold ) {
				meter.channels[channel].hold = val;
				meter.channels[channel].hold_age = 0.0f;
			} else {
				meter.channels[channel].hold_age += 1.0f / static_cast<float>( flags.samplerate );
			}
		}
	}
}

static void update_meter( meter_type & meter, const commandlineflags & flags, std::size_t count, const float * const * buffers ) {
	float falloff_factor = std::pow( 10.0f, -falloff_rate / flags.samplerate / 20.0f );
	for ( int channel = 0; channel < flags.channels; ++channel ) {
		if ( !count ) {
			meter = meter_type();
		}
		meter.channels[channel].peak = 0.0f;
		for ( std::size_t frame = 0; frame < count; ++frame ) {
			if ( meter.channels[channel].clip != 0.0f ) {
				meter.channels[channel].clip -= ( 1.0f / 2.0f ) * 1.0f / static_cast<float>( flags.samplerate );
				if ( meter.channels[channel].clip <= 0.0f ) {
					meter.channels[channel].clip = 0.0f;
				}
			}
			float val = std::fabs( buffers[channel][frame] );
			if ( val >= 1.0f ) {
				meter.channels[channel].clip = 1.0f;
			}
			if ( val > meter.channels[channel].peak ) {
				meter.channels[channel].peak = val;
			}
			meter.channels[channel].hold *= falloff_factor;
			if ( val > meter.channels[channel].hold ) {
				meter.channels[channel].hold = val;
				meter.channels[channel].hold_age = 0.0f;
			} else {
				meter.channels[channel].hold_age += 1.0f / static_cast<float>( flags.samplerate );
			}
		}
	}
}

static const char * const channel_tags[4][4] = {
	{ " C", "  ", "  ", "  " },
	{ " L", " R", "  ", "  " },
	{ "FL", "FR", "RC", "  " },
	{ "FL", "FR", "RL", "RR" },
};

static std::string channel_to_string( int channels, int channel, const meter_channel & meter, bool tiny = false ) {
	float db = 20.0f * std::log10( meter.peak );
	float db_hold = 20.0f * std::log10( meter.hold );
	int val = static_cast<int>( db + 48.0f );
	int hold_pos = static_cast<int>( db_hold + 48.0f );
	if ( val < 0 ) {
		val = 0;
	}
	int headroom = val;
	if ( val > 48 ) {
		val = 48;
	}
	headroom -= val;
	if ( headroom < 0 ) {
		headroom = 0;
	}
	if ( headroom > 12 ) {
		headroom = 12;
	}
	headroom -= 1; // clip indicator
	if ( headroom < 0 ) {
		headroom = 0;
	}
	if ( tiny ) {
		if ( meter.clip != 0.0f || db >= 0.0f ) {
			return "#";
		} else if ( db > -6.0f ) {
			return "O";
		} else if ( db > -12.0f ) {
			return "o";
		} else if ( db > -18.0f ) {
			return ".";
		} else {
			return " ";
		}
	} else {
		std::ostringstream res1;
		std::ostringstream res2;
		res1
			<< "        "
			<< channel_tags[channels-1][channel]
			<< " : "
			;
		res2 
			<< std::string(val,'>') << std::string(48-val,' ')
			<< ( ( meter.clip != 0.0f ) ? "#" : ":" )
			<< std::string(headroom,'>') << std::string(12-headroom,' ')
			;
		std::string tmp = res2.str();
		if ( 0 <= hold_pos && hold_pos <= 60 ) {
			if ( hold_pos == 48 ) {
				tmp[hold_pos] = '#';
			} else {
				tmp[hold_pos] = ':';
			}
		}
		return res1.str() + tmp;
	}
}

static char peak_to_char( float peak ) {
	if ( peak >= 1.0f ) {
		return '#';
	} else if ( peak >= 0.5f ) {
		return 'O';
	} else if ( peak >= 0.25f ) {
		return 'o';
	} else if ( peak >= 0.125f ) {
		return '.';
	} else {
		return ' ';
	}
}

static std::string peak_to_string_left( float peak, int width ) {
	std::string result;
	float thresh = 1.0f;
	while ( width-- ) {
		if ( peak >= thresh ) {
			if ( thresh == 1.0f ) {
				result.push_back( '#' );
			} else {
				result.push_back( '<' );
			}
		} else {
			result.push_back( ' ' );
		}
		thresh *= 0.5f;
	}
	return result;
}

static std::string peak_to_string_right( float peak, int width ) {
	std::string result;
	float thresh = 1.0f;
	while ( width-- ) {
		if ( peak >= thresh ) {
			if ( thresh == 1.0f ) {
				result.push_back( '#' );
			} else {
				result.push_back( '>' );
			}
		} else {
			result.push_back( ' ' );
		}
		thresh *= 0.5f;
	}
	std::reverse( result.begin(), result.end() );
	return result;
}

static void draw_meters( std::ostream & log, const meter_type & meter, const commandlineflags & flags ) {
	for ( int channel = 0; channel < flags.channels; ++channel ) {
		log << channel_to_string( flags.channels, channel, meter.channels[channel] ) << std::endl;
	}
}

static void draw_meters_tiny( std::ostream & log, const meter_type & meter, const commandlineflags & flags ) {
	for ( int channel = 0; channel < flags.channels; ++channel ) {
		log << channel_to_string( flags.channels, channel, meter.channels[channel], true );
	}
}

static void draw_channel_meters_tiny( std::ostream & log, float peak ) {
	log << peak_to_char( peak );
}

static void draw_channel_meters_tiny( std::ostream & log, float peak_left, float peak_right ) {
	log << peak_to_char( peak_left ) << peak_to_char( peak_right );
}

static void draw_channel_meters( std::ostream & log, float peak_left, float peak_right, int width ) {
	if ( width >= 8 + 1 + 8 ) {
		width = 8 + 1 + 8;
	}
	log << peak_to_string_left( peak_left, width / 2 ) << ( width % 1 == 1 ? "|" : "" ) << peak_to_string_right( peak_right, width / 2 );
}

template < typename Tsample, typename Tmod >
void render_loop( commandlineflags & flags, Tmod & mod, double & duration, std::ostream & log, write_buffers_interface & audio_stream ) {

	const std::size_t bufsize = 1024;

	std::vector<Tsample> left( bufsize );
	std::vector<Tsample> right( bufsize );
	std::vector<Tsample> rear_left( bufsize );
	std::vector<Tsample> rear_right( bufsize );
	std::vector<Tsample*> buffers( 4 ) ;
	buffers[0] = left.data();
	buffers[1] = right.data();
	buffers[2] = rear_left.data();
	buffers[3] = rear_right.data();
	buffers.resize( flags.channels );
	
	meter_type meter;
	
#ifdef _MSC_VER
	const bool multiline = false;
#else
	const bool multiline = flags.show_ui;
#endif
	
	int lines = 0;
	
	if ( multiline ) {
		lines += 1;
		if ( flags.show_ui ) {
			lines += 4;
		}
		if ( flags.show_meters ) {
			lines += 1;
			for ( int channel = 0; channel < flags.channels; ++channel ) {
				lines += 1;
			}
		}
		if ( flags.show_details ) {
			lines += 1;
			if ( flags.show_progress ) {
				lines += 1;
			}
		}
		if ( flags.show_progress ) {
			lines += 1;
		}
	} else if ( flags.show_ui || flags.show_details || flags.show_progress ) {
		log << std::endl;
	}
	for ( int line = 0; line < lines; ++line ) {
		log << std::endl;
	}

#if defined( _MSC_VER )
	HANDLE hStdErr = NULL;
	COORD coord_cursor = COORD();
	if( multiline ) {
		log.flush();
		hStdErr = GetStdHandle( STD_ERROR_HANDLE );
		if ( hStdErr ) {
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			ZeroMemory( &csbi, sizeof( CONSOLE_SCREEN_BUFFER_INFO ) );
			GetConsoleScreenBufferInfo( hStdErr, &csbi );
			coord_cursor = csbi.dwCursorPosition;
			coord_cursor.X = 1;
			coord_cursor.Y -= lines;
		}
	}
#endif

	double cpu_smooth = 0.0;

	while ( true ) {

		if ( flags.mode == ModeUI ) {

#if defined( _MSC_VER )

			while ( kbhit() ) {
				int c = getch();
				if ( !handle_keypress( c, flags, mod ) ) {
					return;
				}
			}

#else

			while ( true ) {
				pollfd pollfds;
				pollfds.fd = STDIN_FILENO;
				pollfds.events = POLLIN;
				poll(&pollfds, 1, 0);
				if ( !( pollfds.revents & POLLIN ) ) {
					break;
				}
				char c = 0;
				if ( read( STDIN_FILENO, &c, 1 ) != 1 ) {
					break;
				}
				if ( !handle_keypress( c, flags, mod ) ) {
					return;
				}
			}

#endif

		}
		
		std::clock_t cpu_beg = 0;
		std::clock_t cpu_end = 0;
		if ( flags.show_details ) {
			cpu_beg = std::clock();
		}

		std::size_t count = 0;

		switch ( flags.channels ) {
			case 1: count = mod.read( flags.samplerate, bufsize, left.data() ); break;
			case 2: count = mod.read( flags.samplerate, bufsize, left.data(), right.data() ); break;
			case 4: count = mod.read( flags.samplerate, bufsize, left.data(), right.data(), rear_left.data(), rear_right.data() ); break;
		}
		
		char cpu_str[64] = "";
		if ( flags.show_details ) {
			cpu_end = std::clock();
			if ( count > 0 ) {
				double cpu = 1.0;
				cpu *= ( (double)cpu_end - (double)cpu_beg ) / (double)CLOCKS_PER_SEC;
				cpu /= ( (double)count ) / (double)flags.samplerate;
				double mix = ( (double)count ) / (double)flags.samplerate;
				cpu_smooth = ( 1.0 - mix ) * cpu_smooth + mix * cpu;
				sprintf( cpu_str, "%.2f%%", cpu_smooth * 100.0 );
			}
		}

		if ( flags.show_meters ) {
			update_meter( meter, flags, count, buffers.data() );
		}

		if ( count > 0 ) {
			audio_stream.write( buffers, count );
		}

		if ( multiline ) {
#if defined( _MSC_VER )
			log.flush();
			if ( hStdErr ) {
				SetConsoleCursorPosition( hStdErr, coord_cursor );
			}
#else
			for ( int line = 0; line < lines; ++line ) {
				log << "\x1b[1A";
			}
#endif
			log << std::endl;
			if ( flags.show_ui ) {
				log << "                      <3>|<4>   " << "\r";
				log << "Gain.......: " << flags.gain * 0.01f << " dB" << std::endl;
				log << "                      <5>|<6>   " << "\r";
				log << "Stereo.....: " << flags.separation << " %" << std::endl;
				log << "                      <7>|<8>   " << "\r";
				log << "Filter.....: " << flags.filtertaps << " taps" << std::endl;
				log << "                      <9>|<0>   " << "\r";
				log << "Ramping....: " << flags.ramping << std::endl;
			}
			if ( flags.show_meters ) {
				log << std::endl;
				draw_meters( log, meter, flags );
			}
			if ( flags.show_details ) {
				log << "Mixer......: ";
				log << "CPU:" << std::setw(3) << std::setfill(':') << cpu_str;
				log << "   ";
				log << "Chn:" << std::setw(3) << std::setfill(':') << mod.get_current_playing_channels();
				log << "   ";
				log << std::endl;
				if ( flags.show_progress ) {
					log << "Player.....: ";
					log << "Ord:" << std::setw(3) << std::setfill(':') << mod.get_current_order() << "/" << std::setw(3) << std::setfill(':') << mod.get_num_orders();
					log << " ";
					log << "Pat:" << std::setw(3) << std::setfill(':') << mod.get_current_pattern();
					log << " ";
					log << "Row:" << std::setw(3) << std::setfill(':') << mod.get_current_row();
					log << "   ";
					log << "Spd:" << std::setw(2) << std::setfill(':') << mod.get_current_speed();
					log << " ";
					log << "Tmp:" << std::setw(3) << std::setfill(':') << mod.get_current_tempo();
					log << "   ";
					log << std::endl;
				}
			}
			if ( flags.show_progress ) {
				log << "Position...: " << seconds_to_string( mod.get_position_seconds() ) << " / " << seconds_to_string( duration ) << "   " << std::endl;
			}
		} else if ( flags.show_channel_meters ) {
			if ( flags.show_ui || flags.show_details || flags.show_progress ) {
				int width = flags.terminal_width / mod.get_num_channels();
				log << " ";
				for ( std::int32_t channel = 0; channel < mod.get_num_channels(); ++channel ) {
					if ( width >= 3 ) {
						log << "|";
					}
					if ( width == 1 ) {
						draw_channel_meters_tiny( log, ( mod.get_current_channel_vu_left( channel ) + mod.get_current_channel_vu_right( channel ) ) * (1.0f/std::sqrt(2.0f)) );
					} else if ( width == 2 || width == 3 ) {
						draw_channel_meters_tiny( log, mod.get_current_channel_vu_left( channel ), mod.get_current_channel_vu_right( channel ) );
					} else {
						draw_channel_meters( log, mod.get_current_channel_vu_left( channel ), mod.get_current_channel_vu_right( channel ), width - 1 );
					}
				}
				if ( width >= 3 ) {
					log << "|";
				}
			}
			log << "   " << "\r";
		} else {
			if ( flags.show_ui ) {
				log << " ";
				log << std::setw(3) << std::setfill(':') << flags.gain * 0.01f << "dB";
				log << "|";
				log << std::setw(3) << std::setfill(':') << flags.separation << "%";
				log << "|";
				log << std::setw(2) << std::setfill(':') << flags.filtertaps << "taps";
				log << "|";
				log << std::setw(3) << std::setfill(':') << flags.ramping;
			}
			if ( flags.show_meters ) {
				log << " ";
				draw_meters_tiny( log, meter, flags );
			}
			if ( flags.show_details && flags.show_ui ) {
				log << " ";
				log << "CPU:" << std::setw(3) << std::setfill(':') << cpu_str;
				log << "|";
				log << "Chn:" << std::setw(3) << std::setfill(':') << mod.get_current_playing_channels();
			}
			if ( flags.show_details && !flags.show_ui ) {
				if ( flags.show_progress ) {
					log << " ";
					log << "Ord:" << std::setw(3) << std::setfill(':') << mod.get_current_order() << "/" << std::setw(3) << std::setfill(':') << mod.get_num_orders();
					log << "|";
					log << "Pat:" << std::setw(3) << std::setfill(':') << mod.get_current_pattern();
					log << "|";
					log << "Row:" << std::setw(3) << std::setfill(':') << mod.get_current_row();
					log << " ";
					log << "Spd:" << std::setw(2) << std::setfill(':') << mod.get_current_speed();
					log << "|";
					log << "Tmp:" << std::setw(3) << std::setfill(':') << mod.get_current_tempo();
				}
			}
			if ( flags.show_progress ) {
				log << " ";
				log << seconds_to_string( mod.get_position_seconds() );
				log << "/";
				log << seconds_to_string( duration );
			}
			if ( flags.show_ui || flags.show_details || flags.show_progress ) {
				log << "   " << "\r";
			}
		}

		if ( count == 0 ) {
			break;
		}
		
	}

}

template < typename Tmod >
std::map<std::string,std::string> get_metadata( const Tmod & mod ) {
	std::map<std::string,std::string> result;
	const std::vector<std::string> metadata_keys = mod.get_metadata_keys();
	for ( std::vector<std::string>::const_iterator key = metadata_keys.begin(); key != metadata_keys.end(); ++key ) {
		result[ *key ] = mod.get_metadata( *key );
	}
	return result;
}

template < typename Tmod >
void render_mod_file( commandlineflags & flags, const std::string & filename, std::uint64_t filesize, Tmod & mod, std::ostream & log, write_buffers_interface & audio_stream ) {

	if ( flags.mode != ModeInfo ) {
		mod.set_repeat_count( flags.repeatcount );
		apply_mod_settings( flags, mod );
	}
	
	double duration = mod.get_duration_seconds();
	if ( flags.verbose ) {
		log << "Path.......: " << filename << std::endl;
	}
	if ( flags.show_details ) {
		log << "Filename...: " << get_filename( filename ) << std::endl;
		log << "Size.......: " << bytes_to_string( filesize ) << std::endl;
		log << "Type.......: " << mod.get_metadata( "type" ) << " (" << mod.get_metadata( "type_long" ) << ")" << std::endl;
		log << "Tracker....: " << mod.get_metadata( "tracker" ) << std::endl;
	}
	if ( true ) {
		log << "Title......: " << mod.get_metadata( "title" ) << std::endl;
		log << "Duration...: " << seconds_to_string( duration ) << std::endl;
	}
	if ( flags.show_details ) {
		log << "Channels...: " << mod.get_num_channels() << std::endl;
		log << "Orders.....: " << mod.get_num_orders() << std::endl;
		log << "Patterns...: " << mod.get_num_patterns() << std::endl;
		log << "Instruments: " << mod.get_num_instruments() << std::endl;
		log << "Samples....: " << mod.get_num_samples() << std::endl;
	}

	if ( flags.filenames.size() == 1 || flags.mode == ModeRender ) {
		audio_stream.write_metadata( get_metadata( mod ) );
	} else {
		audio_stream.write_updated_metadata( get_metadata( mod ) );
	}

	if ( flags.show_message ) {
		log << "Message....: " << prepend_lines( mod.get_metadata( "message" ), "           : " ) << std::endl;
	}
	
	if ( flags.mode == ModeInfo ) {
		return;
	}

	if ( flags.seek_target > 0.0 ) {
		mod.set_position_seconds( flags.seek_target );
	}

	try {
		if ( flags.use_float ) {
			render_loop<float>( flags, mod, duration, log, audio_stream );
		} else {
			render_loop<std::int16_t>( flags, mod, duration, log, audio_stream );
		}
		if ( flags.show_progress ) {
			log << std::endl;
		}
	} catch ( ... ) {
		if ( flags.show_progress ) {
			log << std::endl;
		}
		throw;
	}

}

static void render_file( commandlineflags & flags, const std::string & filename, std::ostream & log, write_buffers_interface & audio_stream ) {

	try {

		std::ifstream file_stream;
		std::istream & stdin_stream = std::cin;
		std::uint64_t filesize = 0;
		bool use_stdin = ( filename == "-" );
		if ( !use_stdin ) {
			file_stream.open( filename, std::ios::binary );
			file_stream.seekg( 0, std::ios::end );
			filesize = file_stream.tellg();
			file_stream.seekg( 0, std::ios::beg );
		}
		std::istream & data_stream = use_stdin ? stdin_stream : file_stream;
		if ( data_stream.fail() ) {
			throw exception( "file open error" );
		}

		{
			openmpt::module mod( data_stream );
			render_mod_file( flags, filename, filesize, mod, log, audio_stream );
		} 

	} catch ( prev_file & ) {
		throw;
	} catch ( next_file & ) {
		throw;
	} catch ( silent_exit_exception & ) {
		throw;
	} catch ( std::exception & e ) {
		std::cerr << "error playing '" << filename << "': " << e.what() << std::endl;
	} catch ( ... ) {
		std::cerr << "unknown error playing '" << filename << "'" << std::endl;
	}

	log << std::endl;

}


static void render_files( commandlineflags & flags, std::ostream & log, write_buffers_interface & audio_stream ) {
	std::vector<std::string>::iterator filename = flags.filenames.begin();
	while ( true ) {
		if ( filename == flags.filenames.end() ) {
			break;
		}
		if ( filename == flags.filenames.begin() - 1 ) {
			filename++;
		}
		try {
			render_file( flags, *filename, log, audio_stream );
			filename++;
			continue;
		} catch ( prev_file & e ) {
			while ( filename != flags.filenames.begin() && e.count ) {
				e.count--;
				--filename;
			}
			continue;
		} catch ( next_file & e ) {
			while ( filename != flags.filenames.end() && e.count ) {
				e.count--;
				++filename++;
			}
			continue;
		} catch ( ... ) {
			throw;
		}
	}
}


static commandlineflags parse_modplug123( const std::vector<std::string> & args ) {

	commandlineflags flags;

	flags.modplug123 = true;

	bool files_only = false;
	for ( std::vector<std::string>::const_iterator i = args.begin(); i != args.end(); ++i ) {
		if ( i == args.begin() ) {
			// skip program name
			continue;
		}
		std::string arg = *i;
		std::string nextarg = ( i+1 != args.end() ) ? *(i+1) : "";
		if ( files_only ) {
			flags.filenames.push_back( arg );
		} else if ( arg.substr( 0, 1 ) != "-" ) {
			flags.filenames.push_back( arg );
		} else if ( arg == "-h" || arg == "--help" ) {
			throw show_help_exception();
		} else if ( arg == "-v" || arg == "--version" ) {
			show_info( std::clog, true, true );
			throw silent_exit_exception();
		} else if ( arg == "-l" ) {
			flags.repeatcount = -1;
		} else if ( arg == "-ao" && nextarg != "" ) {
			if ( false ) {
				// nothing
			} else if ( nextarg == "stdout" ) {
				flags.use_stdout = true;
#ifdef MPT_WITH_PORTAUDIO
			} else if ( nextarg == "help" ) {
				show_audio_devices();
			} else if ( nextarg == "default" ) {
				flags.device = -1;
			} else {
				std::istringstream istr( nextarg );
				istr >> flags.device;
#endif
			}
			++i;
		}
	}

	return flags;

}

static commandlineflags parse_openmpt123( const std::vector<std::string> & args ) {

	commandlineflags flags;

	bool files_only = false;
	for ( std::vector<std::string>::const_iterator i = args.begin(); i != args.end(); ++i ) {
		if ( i == args.begin() ) {
			// skip program name
			continue;
		}
		std::string arg = *i;
		std::string nextarg = ( i+1 != args.end() ) ? *(i+1) : "";
		if ( files_only ) {
			flags.filenames.push_back( arg );
		} else if ( arg.substr( 0, 1 ) != "-" ) {
			flags.filenames.push_back( arg );
		} else {
			if ( arg == "--" ) {
				files_only = true;
			} else if ( arg == "-h" || arg == "--help" ) {
				throw show_help_exception();
			} else if ( arg == "-q" || arg == "--quiet" ) {
				flags.quiet = true;
			} else if ( arg == "-v" || arg == "--verbose" ) {
				flags.verbose = true;
			} else if ( arg == "--version" ) {
				throw show_version_number_exception();
			} else if ( arg == "--info" ) {
				flags.mode = ModeInfo;
			} else if ( arg == "--ui" ) {
				flags.mode = ModeUI;
			} else if ( arg == "--batch" ) {
				flags.mode = ModeBatch;
			} else if ( arg == "--render" ) {
				flags.mode = ModeRender;
			} else if ( arg == "--terminal-width" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.terminal_width;
				++i;
			} else if ( arg == "--progress" ) {
				flags.show_progress = true;
			} else if ( arg == "--no-progress" ) {
				flags.show_progress = false;
			} else if ( arg == "--meters" ) {
				flags.show_meters = true;
			} else if ( arg == "--no-meters" ) {
				flags.show_meters = false;
			} else if ( arg == "--channel-meters" ) {
				flags.show_channel_meters = true;
			} else if ( arg == "--no-channel-meters" ) {
				flags.show_channel_meters = false;
			} else if ( arg == "--details" ) {
				flags.show_details = true;
			} else if ( arg == "--no-details" ) {
				flags.show_details = false;
			} else if ( arg == "--message" ) {
				flags.show_message = true;
			} else if ( arg == "--no-message" ) {
				flags.show_message = false;
			} else if ( arg == "--device" && nextarg != "" ) {
				if ( false ) {
					// nothing
				} else if ( nextarg == "stdout" ) {
					flags.use_stdout = true;
#ifdef MPT_WITH_PORTAUDIO
				} else if ( nextarg == "help" ) {
					show_audio_devices();
				} else if ( nextarg == "default" ) {
					flags.device = -1;
				} else {
					std::istringstream istr( nextarg );
					istr >> flags.device;
#endif
				}
				++i;
#ifdef MPT_WITH_PORTAUDIO
			} else if ( arg == "--buffer" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.buffer;
				++i;
#endif
			} else if ( arg == "--stdout" ) {
				flags.use_stdout = true;
#if defined(MPT_WITH_FLAC) || defined(MPT_WITH_MMIO) || defined(MPT_WITH_SNDFILE)
			} else if ( ( arg == "-o" || arg == "--output" ) && nextarg != "" ) {
				flags.output_filename = nextarg;
				++i;
			} else if ( arg == "--force" ) {
				flags.force_overwrite = true;
			} else if ( arg == "--output-type" && nextarg != "" ) {
				flags.output_extension = nextarg;
				++i;
#endif
			} else if ( arg == "--samplerate" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.samplerate;
				++i;
			} else if ( arg == "--channels" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.channels;
				++i;
			} else if ( arg == "--float" ) {
				flags.use_float = true;
			} else if ( arg == "--no-float" ) {
				flags.use_float = false;
			} else if ( arg == "--gain" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				double gain = 0.0;
				istr >> gain;
				flags.gain = static_cast<std::int32_t>( gain * 100.0 );
				++i;
			} else if ( arg == "--stereo" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.separation;
				++i;
			} else if ( arg == "--filter" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.filtertaps;
				++i;
			} else if ( arg == "--ramping" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.ramping;
				++i;
			} else if ( arg == "--repeat" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.repeatcount;
				++i;
			} else if ( arg == "--seek" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.seek_target;
				++i;
			} else if ( arg.size() > 0 && arg.substr( 0, 1 ) == "-" ) {
				throw show_help_exception();
			}
		}
	}

	return flags;

}

static bool equal_end( const std::string & str1, const std::string & str2 ) {
	std::size_t minlength = std::min( str1.length(), str2.length() );
	return str1.substr( str1.length() - minlength ) == str2.substr( str2.length() - minlength );
}

static bool is_modplug123_binary_name( std::string name ) {
	std::transform( name.begin(), name.end(), name.begin(), tolower );
	if ( equal_end( name, "modplug123" ) ) {
		return true;
	}
	if ( equal_end( name, "modplug123.exe" ) ) {
		return true;
	}
	if ( equal_end( name, "modplug12364" ) ) {
		return true;
	}
	if ( equal_end( name, "modplug12364.exe" ) ) {
		return true;
	}
	return false;
}

static void show_credits( std::ostream & s ) {
	s << openmpt::string::get( openmpt::string::contact ) << std::endl;
	s << std::endl;
	s << openmpt::string::get( openmpt::string::credits );
}

static int main( int argc, char * argv [] ) {

	commandlineflags flags;

	try {

		std::vector<std::string> args( argv, argv + argc );

		if ( args.size() > 0 && is_modplug123_binary_name( args[0] ) ) {

			flags = parse_modplug123( args );

		} else {

			flags = parse_openmpt123( args );

		}

		if ( args.size() <= 1 ) {
			throw show_help_exception( "", false );
		}

		flags.check_and_sanitize();

		std::ostringstream dummy_log;
		std::ostream & log = flags.quiet ? dummy_log : std::clog;

		show_info( log, flags.verbose, flags.modplug123 );

		if ( flags.verbose ) {

			show_credits( log );

			log << flags;

		}

		#if defined(_MSC_VER)

			for ( std::vector<std::string>::iterator filename = flags.filenames.begin(); filename != flags.filenames.end(); ++filename ) {
				if ( *filename == "-" ) {
					_setmode( _fileno( stdin ), _O_BINARY );
					break;
				}
			}

		#endif

		#if !defined(_MSC_VER)

			if ( flags.mode == ModeUI ) {
				set_input_mode();

			}

		#endif
		
		switch ( flags.mode ) {
			case ModeInfo: {
				void_audio_stream dummy;
				render_files( flags, log, dummy );
			} break;
			case ModeUI:
			case ModeBatch: {
				if ( flags.use_stdout ) {
					stdout_stream_raii stdout_audio_stream;
					render_files( flags, log, stdout_audio_stream );
#if defined(MPT_WITH_FLAC) || defined(MPT_WITH_MMIO) || defined(MPT_WITH_SNDFILE)
				} else if ( !flags.output_filename.empty() ) {
					file_audio_stream_raii file_audio_stream( flags, flags.output_filename, log );
					render_files( flags, log, file_audio_stream );
#endif
#ifdef MPT_WITH_PORTAUDIO
				} else {
					portaudio_stream_raii portaudio_stream( flags, log );
					render_files( flags, log, portaudio_stream );
#endif
				}
			} break;
			case ModeRender: {
				for ( std::vector<std::string>::iterator filename = flags.filenames.begin(); filename != flags.filenames.end(); ++filename ) {
					file_audio_stream_raii file_audio_stream( flags, *filename + std::string(".") + flags.output_extension, log );
					render_file( flags, *filename, log, file_audio_stream );
				}
			} break;
			case ModeNone:
				throw show_help_exception();
			break;
		}

	} catch ( show_help_exception & e ) {
		show_help( e, flags.verbose, flags.modplug123 );
		if ( flags.verbose ) {
			show_credits( std::clog );
		}
		return 1;
	} catch ( show_version_number_exception & ) {
		show_version();
		return 0;
#ifdef MPT_WITH_PORTAUDIO
	} catch ( portaudio_exception & e ) {
		std::cerr << "PortAudio error: " << e.what() << std::endl;
#endif
	} catch ( silent_exit_exception & ) {
		return 0;
	} catch ( std::exception & e ) {
		std::cerr << "error: " << e.what() << std::endl;
		return 1;
	} catch ( ... ) {
		std::cerr << "unknown error" << std::endl;
		return 1;
	}

	return 0;
}

} // namespace openmpt123

int main( int argc, char * argv [] ) {
	return openmpt123::main( argc, argv );
}
