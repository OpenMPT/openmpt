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
	std::string openmpt123 = std::string() + "openmpt123 " + OPENMPT123_VERSION_STRING + ", libopenmpt " + openmpt::string::get( openmpt::string::library_version ) + ", OpenMPT " + openmpt::string::get( openmpt::string::core_version );
	if ( software.empty() ) {
		software = openmpt123;
	} else {
		software += " (" + openmpt123 + ")";
	}
	return software;
}

static inline std::string get_extension( std::string filename ) {
	if ( filename.find_last_of( "." ) != std::string::npos ) {
		return filename.substr( filename.find_last_of( "." ) + 1 );
	}
	return "";
}

struct commandlineflags {
	bool run_tests;
	bool modplug123;
#ifdef MPT_WITH_PORTAUDIO
	int device;
#endif
	std::int32_t channels;
	std::int32_t buffer;
	std::int32_t repeatcount;
	std::int32_t samplerate;
	std::int32_t gain;
	std::int32_t quality;
	std::int32_t filtertaps;
	std::int32_t rampupus;
	std::int32_t rampdownus;
	bool quiet;
	bool verbose;
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
		run_tests = false;
		modplug123 = false;
#ifdef MPT_WITH_PORTAUDIO
		device = -1;
#endif
		channels = 2;
		buffer = 250;
		repeatcount = 0;
		samplerate = 48000;
		gain = 0;
		quality = 100;
		filtertaps = 8;
		rampupus = ( 16 * 1000000 + ( 44100 / 2 ) ) / 44100; // openmpt defaults at 44KHz, rounded
		rampdownus = ( 42 * 1000000 + ( 44100 / 2 ) ) / 44100; // openmpt defaults at 44KHz, rounded
		quiet = false;
		verbose = false;
		show_message = false;
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
		if ( quiet ) {
			verbose = false;
			show_progress = false;
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
