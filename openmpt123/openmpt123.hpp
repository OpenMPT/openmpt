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

struct commandlineflags {
	bool modplug123;
#ifdef MPT_WITH_PORTAUDIO
	int device;
	std::int32_t buffer;
#endif
	std::int32_t channels;
	std::int32_t repeatcount;
	std::int32_t samplerate;
	std::int32_t gain;
	std::int32_t quality;
	std::int32_t filtertaps;
	int ramping; // ramping strength : -1:default 0:off 1 2 3 4 5 // roughly milliseconds
	bool quiet;
	bool verbose;
	bool use_ui;
	bool show_info;
	bool show_message;
	bool show_progress;
	double seek_target;
	bool use_float;
	bool use_stdout;
	std::vector<std::string> filenames;
#if defined(MPT_WITH_FLAC) || defined(MPT_WITH_MMIO) || defined(MPT_WITH_SNDFILE)
	std::string output_filename;
	bool force_overwrite;
#endif
	commandlineflags() {
		modplug123 = false;
#ifdef MPT_WITH_PORTAUDIO
		device = -1;
		buffer = 250;
#endif
		channels = 2;
		repeatcount = 0;
		samplerate = 48000;
		gain = 0;
		quality = 100;
		filtertaps = 8;
		ramping = -1;
		quiet = false;
		verbose = false;
		show_info = true;
		show_message = false;
#if defined(_MSC_VER)
		use_ui = true;
#else
		use_ui = isatty( STDIN_FILENO ) ? true : false;
#endif
#if defined(_MSC_VER)
		show_progress = false;
#else
		show_progress = isatty( STDERR_FILENO ) ? true : false;
#endif
		seek_target = 0.0;
		use_float = false;
		use_stdout = false;
#if defined(MPT_WITH_FLAC) || defined(MPT_WITH_MMIO) || defined(MPT_WITH_SNDFILE)
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
				use_ui = false;
			}
		}
#if defined(MPT_WITH_FLAC) || defined(MPT_WITH_MMIO) || defined(MPT_WITH_SNDFILE)
		if ( !output_filename.empty() ) {
			use_ui = false;
		}
#endif
		if ( quiet ) {
			verbose = false;
			show_info = false;
			show_progress = false;
		}
		if ( verbose ) {
			show_info = true;
		}
		if ( channels != 1 && channels != 2 && channels != 4 ) {
			channels = commandlineflags().channels;
		}
		if ( samplerate < 0 ) {
			samplerate = commandlineflags().samplerate;
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
};

} // namespace openmpt123

#endif // OPENMPT123_HPP
