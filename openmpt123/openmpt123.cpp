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
#include <cstring>

#if defined(_MSC_VER)
#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#else
#include <unistd.h>
#endif

#include <libopenmpt/libopenmpt.hpp>

#include "openmpt123.hpp"

#include "openmpt123_flac.hpp"
#include "openmpt123_mmio.hpp"
#include "openmpt123_sndfile.hpp"
#include "openmpt123_stdout.hpp"
#include "openmpt123_portaudio.hpp"

namespace openmpt {
LIBOPENMPT_CXX_API void run_tests();
} // namespace openmpt

namespace openmpt123 {

struct silent_exit_exception : public std::exception {
	silent_exit_exception() throw() { }
};

struct show_version_number_exception : public std::exception {
	show_version_number_exception() throw() { }
};

std::ostream & operator << ( std::ostream & s, const commandlineflags & flags ) {
	s << "Quiet: " << flags.quiet << std::endl;
	s << "Verbose: " << flags.verbose << std::endl;
	s << "Show message: " << flags.show_message << std::endl;
	s << "Show progress: " << flags.show_progress << std::endl;
#ifdef MPT_WITH_PORTAUDIO
	s << "Device: " << flags.device << std::endl;
#endif
	s << "Channels: " << flags.channels << std::endl;
	s << "Buffer: " << flags.buffer << std::endl;
	s << "Repeat count: " << flags.repeatcount << std::endl;
	s << "Sample rate: " << flags.samplerate << std::endl;
	s << "Gain: " << flags.gain / 100.0 << std::endl;
	s << "Filter taps: " << flags.filtertaps << std::endl;
	s << "Seek target: " << flags.seek_target << std::endl;
	s << "Float: " << flags.use_float << std::endl;
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

static void show_info( std::ostream & log, bool modplug123 = false ) {
	log << ( modplug123 ? "modplug123 emulated by " : "" ) << "openmpt123" << " version " << OPENMPT123_VERSION_STRING << std::endl;
	log << " Copyright (c) 2013 OpenMPT developers (http://openmpt.org/)" << std::endl;
	log << " libopenmpt " << openmpt::string::get( openmpt::string::library_version ) << " " << "(built " << openmpt::string::get( openmpt::string::build ) << ")" << std::endl;
	log << " OpenMPT " << openmpt::string::get( openmpt::string::core_version ) << std::endl;
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

static void show_help( show_help_exception & e, bool modplug123 ) {
	show_info( std::clog, modplug123 );
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
		std::clog << " -h, --help       Show help" << std::endl;
		std::clog << " -q, --quiet      Suppress non-error screen output" << std::endl;
		std::clog << " -v, --verbose    Show more screen output" << std::endl;
		std::clog << " --version        Show version number and nothing else" << std::endl;
		if ( !e.longhelp ) {
			std::clog << std::endl;
			return;
		}
		std::clog << " --[no-]message   Show song message [default: " << commandlineflags().show_message << "]" << std::endl;
		std::clog << " --[no-]progress  Show playback progress [default: " << commandlineflags().show_progress << "]" << std::endl;
#ifdef MPT_WITH_PORTAUDIO
		std::clog << " --device n       Set output device [default: " << get_device_string( commandlineflags().device ) << "]," << std::endl;
		std::clog << "                  use --device help to show available devices" << std::endl;
#endif
		std::clog << " --stdout         Write raw audio data to stdout [default: " << commandlineflags().use_stdout << "]" << std::endl;
#if defined(MPT_WITH_FLAC) || defined(MPT_WITH_MMIO) || defined(MPT_WITH_SNDFILE)
		std::clog << " -o, --output f   Write PCM output to file f instead of streaming to audio device [default: " << commandlineflags().output_filename << "]" << std::endl;
		std::clog << " --force          Force overwriting of output file [default: " << commandlineflags().force_overwrite << "]" << std::endl;
#endif
		std::clog << " --channels n     use n [1,2,4] output channels [default: " << commandlineflags().channels << "]" << std::endl;
		std::clog << " --[no]-float     Output 32bit floating point instead of 16bit integer [default: " << commandlineflags().use_float << "]" << std::endl;
		std::clog << " --buffer n       Set output buffer size to n ms [default: " << commandlineflags().buffer << "]," << std::endl;
		std::clog << " --samplerate n   Set samplerate to n Hz [default: " << commandlineflags().samplerate << "]" << std::endl;
		std::clog << " --gain n         Set output gain to n dB [default: " << commandlineflags().gain / 100.0 << "]" << std::endl;
		std::clog << " --repeat n       Repeat song n times (-1 means forever) [default: " << commandlineflags().repeatcount << "]" << std::endl;
		std::clog << " --filtertaps n   Set interpolation filter taps to n % [default: " << commandlineflags().filtertaps << "]" << std::endl;
		std::clog << " --seek n         Seek to n seconds on start [default: " << commandlineflags().seek_target << "]" << std::endl;
		std::clog << " --volrampup n    Use n microseconds volume ramping up [default: " << commandlineflags().rampupus << "]" << std::endl;
		std::clog << " --volrampdown n  Use n microseconds volume ramping down [default: " << commandlineflags().rampdownus << "]" << std::endl;
		std::clog << std::endl;
		std::clog << " --               Interpret further arguments as filenames" << std::endl;
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

template < typename Tsample, typename Tmod >
void render_loop( const commandlineflags & flags, const std::string & filename, Tmod & mod, double & duration, std::ostream & log, write_buffers_interface & audio_stream ) {

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

	bool file_stdin = ( filename == "-" );

	while ( true ) {

		if ( !file_stdin ) {

#if defined( _MSC_VER )

			while ( kbhit() ) {
				int c = getch();
				switch ( c ) {
				case 'q': throw silent_exit_exception(); break;
				case 'z': return; break;
				case 'x': mod.seek_seconds( mod.get_current_position_seconds() - 10.0 ); break;
				case 'c': mod.seek_seconds( mod.get_current_position_seconds() - 1.0 ); break;
				case 'v': mod.seek_seconds( mod.get_current_position_seconds() - 0.1 ); break;
				case 'b': mod.seek_seconds( mod.get_current_position_seconds() + 0.1 ); break;
				case 'n': mod.seek_seconds( mod.get_current_position_seconds() + 1.0 ); break;
				case 'm': mod.seek_seconds( mod.get_current_position_seconds() + 10.0 ); break;
				case ',': return; break;
				}
			}

#endif // _MSC_VER

		}

		std::size_t count = 0;

		switch ( flags.channels ) {
			case 1: count = mod.read( flags.samplerate, bufsize, left.data() ); break;
			case 2: count = mod.read( flags.samplerate, bufsize, left.data(), right.data() ); break;
			case 4: count = mod.read( flags.samplerate, bufsize, left.data(), right.data(), rear_left.data(), rear_right.data() ); break;
		}

		if ( count == 0 ) {
			break;
		}

		audio_stream.write( buffers, count );

		if ( flags.show_progress ) {
			log
				<< "   "
				<< seconds_to_string( mod.get_current_position_seconds() )
				<< " / "
				<< seconds_to_string( duration )
				<< "   "
				<< "Ord:" << mod.get_current_order() << "/" << mod.get_num_orders() << " Pat:" << mod.get_current_pattern() << " Row:" << mod.get_current_row()
				<< "   "
				<< "Spd:" << mod.get_current_speed() << " Tmp:" << mod.get_current_tempo()
				<< "  "
				<< "Chn:" << mod.get_current_playing_channels()
				<< "  "
				<< "\r";
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
void render_mod_file( const commandlineflags & flags, const std::string & filename, Tmod & mod, std::ostream & log, write_buffers_interface & audio_stream ) {

	double duration = mod.get_duration_seconds();
	log << "File: " << filename << std::endl;
	log << "Type: " << mod.get_metadata( "type" ) << " (" << mod.get_metadata( "type_long" ) << ")" << std::endl;
	log << "Tracker: " << mod.get_metadata( "tracker" ) << std::endl;
	log << "Title: " << mod.get_metadata( "title" ) << std::endl;
	log << "Duration: " << seconds_to_string( duration ) << std::endl;
	log << "Channels: " << mod.get_num_channels() << std::endl;
	log << "Orders: " << mod.get_num_orders() << std::endl;
	log << "Patterns: " << mod.get_num_patterns() << std::endl;
	log << "Instruments: " << mod.get_num_instruments() << std::endl;
	log << "Samples: " << mod.get_num_samples() << std::endl;

	if ( flags.filenames.size() == 1 ) {
		audio_stream.write_metadata( get_metadata( mod ) );
	} else {
		audio_stream.write_updated_metadata( get_metadata( mod ) );
	}

	if ( flags.show_message ) {
		log << "Message: " << std::endl;
		log << mod.get_metadata( "message" ) << std::endl;
		log << std::endl;
	}

	if ( flags.seek_target > 0.0 ) {
		mod.seek_seconds( flags.seek_target );
	}

	if ( flags.use_float ) {
		render_loop<float>( flags, filename, mod, duration, log, audio_stream );
	} else {
		render_loop<std::int16_t>( flags, filename, mod, duration, log, audio_stream );
	}

	if ( flags.show_progress ) {
		log << std::endl;
	}

}


static void render_file( const commandlineflags & flags, const std::string & filename, std::ostream & log, write_buffers_interface & audio_stream ) {

	try {

		std::ifstream file_stream;
		std::istream & stdin_stream = std::cin;
		bool use_stdin = ( filename == "-" );
		if ( !use_stdin ) {
			file_stream.open( filename, std::ios::binary );
		}
		std::istream & data_stream = use_stdin ? stdin_stream : file_stream;
		if ( data_stream.fail() ) {
			throw exception( "file open error" );
		}

		{

			openmpt::module mod( data_stream );

			mod.set_render_param( openmpt::module::RENDER_REPEATCOUNT, flags.repeatcount );
			mod.set_render_param( openmpt::module::RENDER_INTERPOLATION_FILTER_LENGTH, flags.filtertaps );
			mod.set_render_param( openmpt::module::RENDER_MASTERGAIN_MILLIBEL, flags.gain );
			mod.set_render_param( openmpt::module::RENDER_VOLUMERAMP_UP_MICROSECONDS, flags.rampupus );
			mod.set_render_param( openmpt::module::RENDER_VOLUMERAMP_DOWN_MICROSECONDS, flags.rampdownus );

			render_mod_file( flags, filename, mod, log, audio_stream );

		} 

	} catch ( std::exception & e ) {
		std::cerr << "error playing '" << filename << "': " << e.what() << std::endl;
	} catch ( ... ) {
		std::cerr << "unknown error playing '" << filename << "'" << std::endl;
	}

	log << std::endl;

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
			show_info( std::clog, true );
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
			} else if ( arg == "--message" ) {
				flags.show_message = true;
			} else if ( arg == "--no-message" ) {
				flags.show_message = false;
			} else if ( arg == "--progress" ) {
				flags.show_progress = true;
			} else if ( arg == "--no-progress" ) {
				flags.show_progress = false;
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
			} else if ( arg == "--stdout" ) {
				flags.use_stdout = true;
#if defined(MPT_WITH_FLAC) || defined(MPT_WITH_MMIO) || defined(MPT_WITH_SNDFILE)
			} else if ( ( arg == "-o" || arg == "--output" ) && nextarg != "" ) {
				flags.output_filename = nextarg;
				++i;
			} else if ( arg == "--force" ) {
				flags.force_overwrite = true;
#endif
			} else if ( arg == "--channels" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.channels;
				++i;
			} else if ( arg == "--float" ) {
				flags.use_float = true;
			} else if ( arg == "--no-float" ) {
				flags.use_float = false;
			} else if ( arg == "--buffer" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.buffer;
				++i;
			} else if ( arg == "--samplerate" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.samplerate;
				++i;
			} else if ( arg == "--gain" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				double gain = 0.0;
				istr >> gain;
				flags.gain = static_cast<std::int32_t>( gain * 100.0 );
				++i;
			} else if ( arg == "--repeat" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.repeatcount;
				++i;
			} else if ( arg == "--filtertaps" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.filtertaps;
				++i;
			} else if ( arg == "--seek" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.seek_target;
				++i;
			} else if ( arg == "--volrampup" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.rampupus;
				++i;
			} else if ( arg == "--volrampdown" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.rampdownus;
				++i;
			} else if ( arg == "--runtests" ) {
				flags.run_tests = true;
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

		if ( flags.run_tests ) {
			try {
				openmpt::run_tests();
			} catch ( std::exception & e ) {
				std::cerr << "FAIL: " << e.what() << std::endl;
				return -1;
			} catch ( ... ) {
				std::cerr << "FAIL" << std::endl;
				return -1;
			}
			return 0;
		}
		
		if ( args.size() <= 1 ) {
			throw show_help_exception( "", false );
		}

		flags.check_and_sanitize();

		std::ostringstream dummy_log;
		std::ostream & log = flags.quiet ? dummy_log : std::clog;

		show_info( log, flags.modplug123 );

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

		if ( flags.use_stdout ) {

			stdout_stream_raii stdout_audio_stream;

			for ( std::vector<std::string>::iterator filename = flags.filenames.begin(); filename != flags.filenames.end(); ++filename ) {
				render_file( flags, *filename, log, stdout_audio_stream );
			}

#if defined(MPT_WITH_FLAC) || defined(MPT_WITH_MMIO) || defined(MPT_WITH_SNDFILE)
		} else if ( !flags.output_filename.empty() ) {
		
			if ( !flags.force_overwrite ) {
				std::ifstream testfile( flags.output_filename, std::ios::binary );
				if ( testfile ) {
					throw exception( "file already exists" );
				}
			}

			if ( false ) {
				// nothing
#ifdef MPT_WITH_MMIO
			} else if ( get_extension( flags.output_filename ) == "wav" ) {
				mmio_stream_raii mmio_audio_stream( flags );
				for ( std::vector<std::string>::iterator filename = flags.filenames.begin(); filename != flags.filenames.end(); ++filename ) {
					render_file( flags, *filename, log, mmio_audio_stream );
				}
#endif				
#ifdef MPT_WITH_FLAC
			} else if ( get_extension( flags.output_filename ) == "flac" ) {
				flac_stream_raii flac_audio_stream( flags );
				for ( std::vector<std::string>::iterator filename = flags.filenames.begin(); filename != flags.filenames.end(); ++filename ) {
					render_file( flags, *filename, log, flac_audio_stream );
				}
#endif				
#ifdef MPT_WITH_SNDFILE
			} else {
				sndfile_stream_raii sndfile_audio_stream( flags, log );
				for ( std::vector<std::string>::iterator filename = flags.filenames.begin(); filename != flags.filenames.end(); ++filename ) {
					render_file( flags, *filename, log, sndfile_audio_stream );
				}
#endif
			}
#endif		
#ifdef MPT_WITH_PORTAUDIO
		} else {

			portaudio_stream_raii portaudio_stream( flags, log );

			for ( std::vector<std::string>::iterator filename = flags.filenames.begin(); filename != flags.filenames.end(); ++filename ) {
				render_file( flags, *filename, log, portaudio_stream );
			}
#endif
		}

	} catch ( show_help_exception & e ) {
		show_help( e, flags.modplug123 );
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
