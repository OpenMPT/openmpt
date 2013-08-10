/*
 * openmpt123.hpp
 * --------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_HPP
#define OPENMPT123_HPP

#include "openmpt123_config.hpp"

namespace openmpt123 {

struct exception : public openmpt::exception {
	exception( const char * text ) : openmpt::exception(text) { }
};

struct show_help_exception {
	std::string message;
	bool longhelp;
	show_help_exception( const std::string & msg = "", bool longhelp_ = true ) : message(msg), longhelp(longhelp_) { }
};

static inline float mpt_round( float val ) {
	if ( val >= 0.0f ) {
		return std::floor( val + 0.5f );
	} else {
		return std::ceil( val - 0.5f );
	}
}

static inline long mpt_lround( float val ) {
	return static_cast< long >( mpt_round( val ) );
}

static inline std::string append_software_tag( std::string software ) {
	std::string openmpt123 = std::string() + "openmpt123 " + OPENMPT123_VERSION_STRING + " (libopenmpt " + openmpt::string::get( openmpt::string::library_version ) + ", OpenMPT " + openmpt::string::get( openmpt::string::core_version ) + ")";
	if ( software.empty() ) {
		software = openmpt123;
	} else {
		software += " (via " + openmpt123 + ")";
	}
	return software;
}

static inline std::string get_encoder_tag() {
	return std::string() + "openmpt123 " + OPENMPT123_VERSION_STRING + " (libopenmpt " + openmpt::string::get( openmpt::string::library_version ) + ", OpenMPT " + openmpt::string::get( openmpt::string::core_version ) + ")";
}

static inline std::string get_extension( std::string filename ) {
	if ( filename.find_last_of( "." ) != std::string::npos ) {
		return filename.substr( filename.find_last_of( "." ) + 1 );
	}
	return "";
}

bool IsTerminal( int fd );

enum Mode {
	ModeNone,
	ModeInfo,
	ModeUI,
	ModeBatch,
	ModeRender
};

static inline std::string mode_to_string( Mode mode ) {
	switch ( mode ) {
		case ModeNone:   return "none"; break;
		case ModeInfo:   return "info"; break;
		case ModeUI:     return "ui"; break;
		case ModeBatch:  return "batch"; break;
		case ModeRender: return "render"; break;
	}
	return "";
}

struct commandlineflags {
	bool modplug123;
	Mode mode;
	bool canUI;
	bool canProgress;
#ifdef MPT_WITH_PORTAUDIO
	int device;
	std::int32_t buffer;
#endif
	std::int32_t samplerate;
	std::int32_t channels;
	std::int32_t gain;
	std::int32_t separation;
	std::int32_t filtertaps;
	std::int32_t ramping; // ramping strength : -1:default 0:off 1 2 3 4 5 // roughly milliseconds
	std::int32_t repeatcount;
	double seek_target;
	bool quiet;
	bool verbose;
	int terminal_width;
	bool show_details;
	bool show_message;
	bool show_ui;
	bool show_progress;
	bool show_meters;
	bool show_channel_meters;
	bool use_float;
	bool use_stdout;
	std::vector<std::string> filenames;
#if defined(MPT_WITH_FLAC) || defined(MPT_WITH_MMIO) || defined(MPT_WITH_SNDFILE)
	std::string output_filename;
	std::string output_extension;
	bool force_overwrite;
#endif
	commandlineflags() {
		modplug123 = false;
		mode = ModeUI;
#ifdef MPT_WITH_PORTAUDIO
		device = -1;
		buffer = 250;
#endif
		samplerate = 48000;
		channels = 2;
		use_float = true;
		gain = 0;
		separation = 100;
		filtertaps = 8;
		ramping = -1;
		repeatcount = 0;
		seek_target = 0.0;
		quiet = false;
		verbose = false;
		terminal_width = 72;
#if !defined(_MSC_VER)
		if ( isatty( STDERR_FILENO ) ) {
			if ( std::getenv( "COLUMNS" ) ) {
				std::istringstream istr( std::getenv( "COLUMNS" ) );
				int tmp = 0;
				istr >> tmp;
				if ( tmp > 0 ) {
					terminal_width = tmp;
				}
			}
			#if TIOCGSIZE
				struct ttysize ts;
				if ( ioctl( STDERR_FILENO, TIOCGSIZE, &ts ) >= 0 ) {
					terminal_width = ts.ts_cols;
				}
			#elif defined(TIOCGWINSZ)
				struct winsize ts;
				if ( ioctl( STDERR_FILENO, TIOCGWINSZ, &ts ) >= 0 ) {
					terminal_width = ts.ws_col;
				}
			#endif
		}
#endif
		show_details = true;
		show_message = false;
#if defined(_MSC_VER)
		canUI = IsTerminal( 0 ) ? true : false;
		canProgress = IsTerminal( 2 ) ? true : false;
#else
		canUI = isatty( STDIN_FILENO ) ? true : false;
		canProgress = isatty( STDERR_FILENO ) ? true : false;
#endif
		show_ui = canUI;
		show_progress = canProgress;
		show_meters = canUI && canProgress;
		show_channel_meters = false;
		use_stdout = false;
#if defined(MPT_WITH_FLAC) || defined(MPT_WITH_MMIO) || defined(MPT_WITH_SNDFILE)
		output_extension = "wav";
		force_overwrite = false;
#endif
	}
	void check_and_sanitize() {
		if ( filenames.size() == 0 ) {
			throw show_help_exception();
		}
#if defined(MPT_WITH_PORTAUDIO) && ( defined(MPT_WITH_FLAC) || defined(MPT_WITH_MMIO) || defined(MPT_WITH_SNDFILE) )
		if ( use_stdout && ( device != commandlineflags().device || !output_filename.empty() ) ) {
			throw show_help_exception();
		}
		if ( !output_filename.empty() && ( device != commandlineflags().device || use_stdout ) ) {
			throw show_help_exception();
		}
#endif
		for ( std::vector<std::string>::iterator i = filenames.begin(); i != filenames.end(); ++i ) {
			if ( *i == "-" ) {
				canUI = false;
			}
		}
		show_ui = canUI;
		if ( mode == ModeNone ) {
			if ( canUI ) {
				mode = ModeUI;
			} else {
				mode = ModeBatch;
			}
		}
		if ( mode == ModeUI && !canUI ) {
			throw show_help_exception();
		}
		if ( show_progress && !canProgress ) {
			throw show_help_exception();
		}
		switch ( mode ) {
			case ModeNone:
				throw show_help_exception();
			break;
			case ModeInfo:
				show_ui = false;
				show_progress = false;
				show_meters = false;
				show_channel_meters = false;
			break;
			case ModeUI:
			break;
			case ModeBatch:
				show_meters = false;
				show_channel_meters = false;
			break;
			case ModeRender:
				show_meters = false;
				show_channel_meters = false;
				show_ui = false;
			break;
		}
		if ( quiet ) {
			verbose = false;
			show_ui = false;
			show_details = false;
			show_progress = false;
			show_channel_meters = false;
		}
		if ( verbose ) {
			show_details = true;
		}
		if ( channels != 1 && channels != 2 && channels != 4 ) {
			channels = commandlineflags().channels;
		}
		if ( samplerate < 0 ) {
			samplerate = commandlineflags().samplerate;
		}
		if ( !output_filename.empty() ) {
			output_extension = get_extension( output_filename );
		}
		if ( mode == ModeRender && output_extension.empty() ) {
			throw show_help_exception();
		}
	}
};

class write_buffers_interface {
public:
	virtual void write_metadata( std::map<std::string,std::string> metadata ) {
		(void)metadata;
		return;
	}
	virtual void write_updated_metadata( std::map<std::string,std::string> metadata ) {
		(void)metadata;
		return;
	}
	virtual void write( const std::vector<float*> buffers, std::size_t frames ) = 0;
	virtual void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) = 0;
	virtual bool is_dummy() const {
		return false;
	}
};

class void_audio_stream : public write_buffers_interface {
	virtual void write( const std::vector<float*> buffers, std::size_t frames ) {
		(void)buffers;
		(void)frames;
	}
	virtual void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
		(void)buffers;
		(void)frames;
	}
	virtual bool is_dummy() const {
		return true;
	}
};

class file_audio_stream_base : public write_buffers_interface {
protected:
	file_audio_stream_base() {
		return;
	}
public:
	virtual void write_metadata( std::map<std::string,std::string> metadata ) {
		(void)metadata;
		return;
	}
	virtual void write_updated_metadata( std::map<std::string,std::string> metadata ) {
		(void)metadata;
		return;
	}
	virtual void write( const std::vector<float*> buffers, std::size_t frames ) = 0;
	virtual void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) = 0;
	virtual ~file_audio_stream_base() {
		return;
	}
};

} // namespace openmpt123

#endif // OPENMPT123_HPP
