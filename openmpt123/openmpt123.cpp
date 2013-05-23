/*
 * openmpt123.cpp
 * --------------
 * Purpose: libopenmpt command line example player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#define LIBOPENMPT_ALPHA_WARNING_SEEN_AND_I_KNOW_WHAT_I_AM_DOING

#define OPENMPT123_VERSION_STRING "0.1"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <cstdint>
#include <cstring>

#if defined(_MSC_VER)
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h> 
#endif

#include <libopenmpt/libopenmpt.hpp>

#include <portaudio.h>

struct show_help_exception {
	std::string message;
	bool longhelp;
	show_help_exception( const std::string & msg = "", bool longhelp_ = true ) : message(msg), longhelp(longhelp_) { }
};

struct openmpt123_exception : public openmpt::exception {
	openmpt123_exception( const char * text_ ) throw() : text(text_) { }
	virtual const char * what() const throw() {
		return text;
	}
	const char * text;
};

struct silent_exit_exception : public std::exception {
	silent_exit_exception() throw() { }
};

struct show_version_number_exception : public std::exception {
	show_version_number_exception() throw() { }
};

struct portaudio_exception : public openmpt123_exception {
	portaudio_exception( PaError code ) throw() : openmpt123_exception( Pa_GetErrorText( code ) ) { }
};

struct openmpt123_flags {
	bool modplug123;
	int device;
	std::int32_t channels;
	std::int32_t buffer;
	std::int32_t repeatcount;
	std::int32_t samplerate;
	std::int32_t gain;
	std::int32_t quality;
	std::int32_t rampinus;
	std::int32_t rampoutus;
	bool quiet;
	bool verbose;
	bool show_message;
	bool show_progress;
	double seek_target;
	bool use_float;
	bool use_stdout;
	std::vector<std::string> filenames;
	openmpt123_flags() {
		modplug123 = false;
		device = -1;
		channels = 2;
		buffer = 250;
		repeatcount = 0;
		samplerate = 48000;
		gain = 0;
		quality = 100;
		rampinus = ( 16 * 1000000 + ( 44100 / 2 ) ) / 44100; // openmpt defaults at 44KHz, rounded
		rampoutus = ( 42 * 1000000 + ( 44100 / 2 ) ) / 44100; // openmpt defaults at 44KHz, rounded
		quiet = false;
		verbose = false;
		show_message = false;
		show_progress = false;
		seek_target = 0.0;
		use_float = false;
		use_stdout = false;
	}
	void check_and_sanitize() {
		if ( filenames.size() == 0 ) {
			throw show_help_exception();
		}
		if ( quiet ) {
			verbose = false;
			show_progress = false;
		}
		if ( channels != 1 && channels != 2 && channels != 4 ) {
			channels = openmpt123_flags().channels;
		}
		if ( samplerate < 0 ) {
			samplerate = openmpt123_flags().samplerate;
		}
	}
};

std::ostream & operator << ( std::ostream & s, const openmpt123_flags & flags ) {
	s << "Quiet: " << flags.quiet << std::endl;
	s << "Verbose: " << flags.verbose << std::endl;
	s << "Show message: " << flags.show_message << std::endl;
	s << "Show progress: " << flags.show_progress << std::endl;
	s << "Device: " << flags.device << std::endl;
	s << "Channels: " << flags.channels << std::endl;
	s << "Buffer: " << flags.buffer << std::endl;
	s << "Repeat count: " << flags.repeatcount << std::endl;
	s << "Sample rate: " << flags.samplerate << std::endl;
	s << "Gain: " << flags.gain / 100.0 << std::endl;
	s << "Quality: " << flags.quality << std::endl;
	s << "Seek target: " << flags.seek_target << std::endl;
	s << "Float: " << flags.use_float << std::endl;
	s << "Standard output: " << flags.use_stdout << std::endl;
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

class write_buffers_interface {
public:
	virtual void write( const std::vector<float*> buffers, std::size_t frames ) = 0;
	virtual void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) = 0;
};

class portaudio_stream_raii : public portaudio_raii, public write_buffers_interface {
private:
	PaStream * stream;
public:
	portaudio_stream_raii( const openmpt123_flags & flags, std::ostream & log = std::cerr ) : portaudio_raii(flags.verbose, log), stream(NULL) {
		PaStreamParameters streamparameters;
		std::memset( &streamparameters, 0, sizeof(PaStreamParameters) );
		streamparameters.device = ( flags.device == -1 ) ? Pa_GetDefaultOutputDevice() : flags.device;
		streamparameters.channelCount = flags.channels;
		streamparameters.sampleFormat = ( flags.use_float ? paFloat32 : paInt16 ) | paNonInterleaved;
		streamparameters.suggestedLatency = flags.buffer * 0.001;
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

class stdout_stream_raii : public write_buffers_interface {
private:
	std::vector<float> interleaved_float_buffer;
	std::vector<std::int16_t> interleaved_int_buffer;
public:
	stdout_stream_raii() {
		#if defined(_MSC_VER)
			_setmode( _fileno( stdout ), _O_BINARY );
		#endif
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) {
		interleaved_float_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				interleaved_float_buffer.push_back( buffers[channel][frame] );
			}
		}
		std::cout.write( (const char*)( interleaved_float_buffer.data() ), interleaved_float_buffer.size() * sizeof( float ) );
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
		interleaved_int_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				interleaved_int_buffer.push_back( buffers[channel][frame] );
			}
		}
		std::cout.write( (const char*)( interleaved_int_buffer.data() ), interleaved_int_buffer.size() * sizeof( std::int16_t ) );
	}
};

static void show_info( std::ostream & log, bool modplug123 = false ) {
	log << ( modplug123 ? "modplug123 emulated by " : "" ) << "openmpt123" << " version " << OPENMPT123_VERSION_STRING << std::endl;
	log << " Copyright (c) 2013 OpenMPT developers (http://openmpt.org/)" << std::endl;
	log << " libopenmpt version " << openmpt::string::get( openmpt::string::library_version ) << " " << "(built " << openmpt::string::get( openmpt::string::build ) << ")" << std::endl;
	log << " OpenMPT version " << openmpt::string::get( openmpt::string::core_version ) << std::endl;
	log << " " << Pa_GetVersionText() << " (http://portaudio.com/)" << std::endl;
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
		std::clog << " -l               Loop song [default: " << openmpt123_flags().repeatcount << "]" << std::endl;
		std::clog << " -ao n            Set output device [default: " << openmpt123_flags().device << "]," << std::endl;
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
		std::clog << " --[no-]message   Show song message [default: " << openmpt123_flags().show_message << "]" << std::endl;
		std::clog << " --[no-]progress  Show playback progress [default: " << openmpt123_flags().show_progress << "]" << std::endl;
		std::clog << " --device n       Set output device [default: " << get_device_string( openmpt123_flags().device ) << "]," << std::endl;
		std::clog << "                  use --device help to show available devices" << std::endl;
		std::clog << " --channels n     use n [1,2,4] output channels [default: " << openmpt123_flags().channels << "]" << std::endl;
		std::clog << " --[no]-float     Output 32bit floating point instead of 16bit integer [default: " << openmpt123_flags().use_float << "]" << std::endl;
		std::clog << " --buffer n       Set output buffer size to n ms [default: " << openmpt123_flags().buffer << "]," << std::endl;
		std::clog << " --samplerate n   Set samplerate to n Hz [default: " << openmpt123_flags().samplerate << "]" << std::endl;
		std::clog << " --gain n         Set output gain to n dB [default: " << openmpt123_flags().gain / 100.0 << "]" << std::endl;
		std::clog << " --repeat n       Repeat song n times (-1 means forever) [default: " << openmpt123_flags().repeatcount << "]" << std::endl;
		std::clog << " --quality n      Set rendering quality to n % [default: " << openmpt123_flags().quality << "]" << std::endl;
		std::clog << " --seek n         Seek to n seconds on start [default: " << openmpt123_flags().seek_target << "]" << std::endl;
		std::clog << " --volrampin n    Use n microseconds volume ramping on sample begin [default: " << openmpt123_flags().rampinus << "]" << std::endl;
		std::clog << " --volrampout n   Use n microseconds volume ramping on sample end [default: " << openmpt123_flags().rampoutus << "]" << std::endl;
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

static void show_audio_devices() {
	std::ostringstream devices;
	devices << " Available devices:" << std::endl;
	portaudio_raii portaudio( false );
	devices << "    " << "stdout" << ": " << "use standard output" << std::endl;
	devices << "    " << "default" << ": " << "default" << std::endl;
	for ( PaDeviceIndex i = 0; i < Pa_GetDeviceCount(); ++i ) {
		if ( Pa_GetDeviceInfo( i ) && Pa_GetDeviceInfo( i )->maxOutputChannels > 0 ) {
			devices << "    " << i << ": " << Pa_GetDeviceInfo( i )->name << std::endl;
		}
	}
	throw show_help_exception( devices.str() );
}

template < typename Tsample >
void render_loop( const openmpt123_flags & flags, openmpt::module & mod, double & duration, std::ostream & log, write_buffers_interface & audio_stream ) {

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

	while ( true ) {

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

static void render_file( const openmpt123_flags & flags, const std::string & filename, std::ostream & log, write_buffers_interface & audio_stream ) {

	try {

		std::ifstream file_stream;
		std::istream & stdin_stream = std::cin;
		bool use_stdin = ( filename == "-" );
		if ( !use_stdin ) {
			file_stream.open( filename, std::ios::binary );
		}
		std::istream & data_stream = use_stdin ? stdin_stream : file_stream;
		if ( data_stream.fail() ) {
			throw openmpt123_exception( "file open error" );
		}

		openmpt::module mod( data_stream );

		if ( !use_stdin ) {
			file_stream.close();
		}

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

		if ( flags.show_message ) {
			log << "Message: " << std::endl;
			log << mod.get_metadata( "message" ) << std::endl;
			log << std::endl;
		}

		mod.set_render_param( openmpt::module::RENDER_REPEATCOUNT, flags.repeatcount );
		mod.set_render_param( openmpt::module::RENDER_QUALITY_PERCENT, flags.quality );
		mod.set_render_param( openmpt::module::RENDER_MASTERGAIN_MILLIBEL, flags.gain );
		mod.set_render_param( openmpt::module::RENDER_VOLUMERAMP_IN_MICROSECONDS, flags.rampinus );
		mod.set_render_param( openmpt::module::RENDER_VOLUMERAMP_OUT_MICROSECONDS, flags.rampoutus );

		if ( flags.seek_target > 0.0 ) {
			mod.seek_seconds( flags.seek_target );
		}

		if ( flags.use_float ) {
			render_loop<float>( flags, mod, duration, log, audio_stream );
		} else {
			render_loop<std::int16_t>( flags, mod, duration, log, audio_stream );
		}

		if ( flags.show_progress ) {
			log << std::endl;
		}

	} catch ( std::exception & e ) {
		std::cerr << "error playing '" << filename << "': " << e.what() << std::endl;
	} catch ( ... ) {
		std::cerr << "unknown error playing '" << filename << "'" << std::endl;
	}

	log << std::endl;

}

static openmpt123_flags parse_modplug123( const std::vector<std::string> & args ) {

	openmpt123_flags flags;

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
			if ( nextarg == "help" ) {
				show_audio_devices();
			} else if ( nextarg == "stdout" ) {
				flags.use_stdout = true;
			} else if ( nextarg == "default" ) {
				flags.device = -1;
			} else {
				std::istringstream istr( nextarg );
				istr >> flags.device;
			}
			++i;
		}
	}

	return flags;

}

static openmpt123_flags parse_openmpt123( const std::vector<std::string> & args ) {

	openmpt123_flags flags;

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
				if ( nextarg == "help" ) {
					show_audio_devices();
				} else if ( nextarg == "stdout" ) {
					flags.use_stdout = true;
				} else if ( nextarg == "default" ) {
					flags.device = -1;
				} else {
					std::istringstream istr( nextarg );
					istr >> flags.device;
				}
				++i;
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
			} else if ( arg == "--quality" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.quality;
				++i;
			} else if ( arg == "--seek" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.seek_target;
				++i;
			} else if ( arg == "--volrampin" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.rampinus;
				++i;
			} else if ( arg == "--volrampout" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.rampoutus;
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

int main( int argc, char * argv [] ) {

	openmpt123_flags flags;

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

		} else {

			portaudio_stream_raii portaudio_stream( flags, log );

			for ( std::vector<std::string>::iterator filename = flags.filenames.begin(); filename != flags.filenames.end(); ++filename ) {
				render_file( flags, *filename, log, portaudio_stream );
			}

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
	} catch ( portaudio_exception & e ) {
		std::cerr << "PortAudio error: " << e.what() << std::endl;
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
