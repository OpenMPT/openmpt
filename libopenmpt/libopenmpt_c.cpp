/*
 * libopenmpt_c.cpp
 * ----------------
 * Purpose: libopenmpt C interface implementation
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "common/stdafx.h"

#include "libopenmpt_internal.h"
#include "libopenmpt.h"
#include "libopenmpt_stream_callbacks.h"

#include "libopenmpt_impl.hpp"

#include <stdexcept>

#include <cmath>
#include <cstdlib>
#include <cstring>

//#ifndef NO_LIBOPENMPT_C

namespace openmpt {

static const char * strdup( const char * src ) {
	char * dst = (char*)std::malloc( std::strlen( src ) + 1 );
	if ( !dst ) {
		return NULL;
	}
	std::strcpy( dst, src );
	return dst;
}

class callbacks_streambuf : public std::streambuf {
public:
	callbacks_streambuf( openmpt_stream_callbacks callbacks_, void * stream_ ) : callbacks(callbacks_), stream(stream_) {
		return;
	}
private:
	int_type underflow() {
		if ( gptr() < egptr() ) {
			return traits_type::to_int_type( *gptr() );
		}
		char * base = &buffer.front();
		char * start = base;
		if ( eback() == base ) {
			std::memmove( base, egptr() - put_back, put_back );
			start += put_back;
		}
		if ( !callbacks.read ) {
			return traits_type::eof();
		}
		std::size_t n = callbacks.read( stream, start, buffer.size() - ( start - base ) );
		if ( n == 0 ) {
			return traits_type::eof();
		}
		setg( base, start, start + n );
		return traits_type::to_int_type( *gptr() );
	}
	callbacks_streambuf( const callbacks_streambuf & );
	callbacks_streambuf & operator = ( const callbacks_streambuf & );
private:
	openmpt_stream_callbacks callbacks;
	void * stream;
	static const std::size_t put_back = 4096;
	static const std::size_t buf_size = 65536;
	std::vector<char> buffer;
}; // class callbacks_streambuf

class callbacks_istream : public std::istream {
private:
	callbacks_streambuf buf;
private:
	callbacks_istream( const callbacks_istream & );
	callbacks_istream & operator = ( const callbacks_istream & );
public:
	callbacks_istream( openmpt_stream_callbacks callbacks, void * stream ) : std::istream(&buf), buf(callbacks, stream) {
		return;
	}
	~callbacks_istream() {
		return;
	}
}; // class xmplay_istream

class logfunc_logger : public log_interface {
private:
	openmpt_log_func m_logfunc;
	void * m_user;
public:
	logfunc_logger( openmpt_log_func func, void * user ) : m_logfunc(func), m_user(user) {
		return;
	}
	virtual ~logfunc_logger() {
		return;
	}
	virtual void log( const std::string & message ) const {
		if ( m_logfunc ) {
			m_logfunc( message.c_str(), m_user );
		} else {
			openmpt_log_func_default( message.c_str(), m_user );
		}
	}
}; // class CSoundFileLog_log_func

} // namespace openmpt

extern "C" {

struct openmpt_module {
	openmpt_log_func logfunc;
	void * user;
	openmpt::module_impl * impl;
};

#define OPENMPT_INTERFACE_CATCH \
	 catch ( openmpt::exception & e ) { \
		std::cerr \
			<< "openmpt: " \
			<< __FUNCTION__ << ": " \
			<< "ERROR: " \
			<< e.what() \
			<< std::endl; \
	} catch ( std::exception & e ) { \
		std::cerr \
			<< "openmpt: " \
			<< __FUNCTION__ << ": " \
			<< "INTERNAL ERROR: " \
			<< e.what() \
			<< std::endl; \
	} catch ( ... ) { \
		std::cerr \
			<< "openmpt: " \
			<< __FUNCTION__ << ": " \
			<< "UNKOWN INTERNAL ERROR" \
			<< std::endl; \
	} \
	do { } while (0) \
/**/

#define OPENMPT_INTERFACE_CATCH_TO_LOG_FUNC \
	 catch ( openmpt::exception & e ) { \
		std::ostringstream err; \
		err \
			<< __FUNCTION__ << ": " \
			<< "ERROR: " \
			<< e.what(); \
		if ( logfunc ) { \
			logfunc( err.str().c_str(), user ); \
		} else { \
			openmpt_log_func_default( err.str().c_str(), NULL ); \
		} \
	} catch ( std::exception & e ) { \
		std::ostringstream err; \
		err \
			<< __FUNCTION__ << ": " \
			<< "INTERNAL ERROR: " \
			<< e.what(); \
		if ( logfunc ) { \
			logfunc( err.str().c_str(), user ); \
		} else { \
			openmpt_log_func_default( err.str().c_str(), NULL ); \
		} \
	} catch ( ... ) { \
		std::ostringstream err; \
		err \
			<< __FUNCTION__ << ": " \
			<< "UNKOWN INTERNAL ERROR"; \
		if ( logfunc ) { \
			logfunc( err.str().c_str(), user ); \
		} else { \
			openmpt_log_func_default( err.str().c_str(), NULL ); \
		} \
	} \
	do { } while (0) \
/**/

#define OPENMPT_INTERFACE_CATCH_TO_MOD_LOG_FUNC \
	 catch ( openmpt::exception & e ) { \
		std::ostringstream err; \
		err \
			<< __FUNCTION__ << ": " \
			<< "ERROR: " \
			<< e.what(); \
		if ( mod->logfunc ) { \
			mod->logfunc( err.str().c_str(), mod->user ); \
		} else { \
			openmpt_log_func_default( err.str().c_str(), NULL ); \
		} \
	} catch ( std::exception & e ) { \
		std::ostringstream err; \
		err \
			<< __FUNCTION__ << ": " \
			<< "INTERNAL ERROR: " \
			<< e.what(); \
		if ( mod->logfunc ) { \
			mod->logfunc( err.str().c_str(), mod->user ); \
		} else { \
			openmpt_log_func_default( err.str().c_str(), NULL ); \
		} \
	} catch ( ... ) { \
		std::ostringstream err; \
		err \
			<< __FUNCTION__ << ": " \
			<< "UNKOWN INTERNAL ERROR"; \
		if ( mod->logfunc ) { \
			mod->logfunc( err.str().c_str(), mod->user ); \
		} else { \
			openmpt_log_func_default( err.str().c_str(), NULL ); \
		} \
	} \
	do { } while (0) \
/**/

#define OPENMPT_INTERFACE_CATCH_TO_LOG \
	 catch ( openmpt::exception & e ) { \
		std::ostringstream err; \
		err \
			<< __FUNCTION__ << ": " \
			<< "ERROR: " \
			<< e.what(); \
		if ( mod && mod->impl ) { \
			mod->impl->PushToCSoundFileLog( LogError, err.str() ); \
		} else { \
			openmpt_log_func_default( err.str().c_str(), NULL ); \
		} \
	} catch ( std::exception & e ) { \
		std::ostringstream err; \
		err \
			<< __FUNCTION__ << ": " \
			<< "INTERNAL ERROR: " \
			<< e.what(); \
		if ( mod && mod->impl ) { \
			mod->impl->PushToCSoundFileLog( LogError, err.str() ); \
		} else { \
			openmpt_log_func_default( err.str().c_str(), NULL ); \
		} \
	} catch ( ... ) { \
		std::ostringstream err; \
		err \
			<< __FUNCTION__ << ": " \
			<< "UNKOWN INTERNAL ERROR"; \
		if ( mod && mod->impl ) { \
			mod->impl->PushToCSoundFileLog( LogError, err.str() ); \
		} else { \
			openmpt_log_func_default( err.str().c_str(), NULL ); \
		} \
	} \
	do { } while (0) \
/**/

#define OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod ) \
	do { \
		if ( !(mod) ) { \
			throw openmpt::exception_message("module * not valid"); \
		} \
	} while(0) \
/**/

#define OPENMPT_INTERFACE_CHECK_POINTER( value ) \
	do { \
		if ( !(value) ) { \
			throw openmpt::exception_message("null pointer"); \
		} \
	} while(0) \
/**/

int openmpt_is_compatible_version( uint32_t api_version ) {
	try {
		return openmpt::version::get_version_compatbility( api_version );
	} OPENMPT_INTERFACE_CATCH;
	return 0;
}

uint32_t openmpt_get_library_version(void) {
	try {
		return openmpt::get_library_version();
	} OPENMPT_INTERFACE_CATCH;
	return 0;
}

uint32_t openmpt_get_core_version(void) {
	try {
		return openmpt::get_core_version();
	} OPENMPT_INTERFACE_CATCH;
	return 0;
}

void openmpt_free_string( const char * str ) {
	try {
		std::free( (void*)str );
	} OPENMPT_INTERFACE_CATCH;
	return;
}

const char * openmpt_get_string( const char * key ) {
	try {
		if ( !key ) {
			return openmpt::strdup( "" );
		}
		return openmpt::strdup( openmpt::string::get( key ).c_str() );
	} OPENMPT_INTERFACE_CATCH;
	return NULL;
}

const char * openmpt_get_supported_extensions(void) {
	try {
		std::string retval;
		bool first = true;
		std::vector<std::string> supported_extensions = openmpt::get_supported_extensions();
		for ( std::vector<std::string>::iterator i = supported_extensions.begin(); i != supported_extensions.end(); ++i ) {
			if ( first ) {
				first = false;
			} else {
				retval += ";";
			}
			retval += *i;
		}
		return openmpt::strdup( retval.c_str() );
	} OPENMPT_INTERFACE_CATCH;
	return NULL;
}

int openmpt_is_extension_supported( const char * extension ) {
	try {
		if ( !extension ) {
			return 0;
		}
		return openmpt::is_extension_supported( extension ) ? 1 : 0;
	} OPENMPT_INTERFACE_CATCH;
	return 0;
}

void openmpt_log_func_default( const char * message, void * /*user*/ ) {
	fprintf( stderr, "%s\n", message );
	fflush( stderr );
}

void openmpt_log_func_silent( const char * /*message*/, void * /*user*/ ) {
	return;
}

double openmpt_could_open_propability( openmpt_stream_callbacks stream_callbacks, void * stream, double effort, openmpt_log_func logfunc, void * user ) {
	try {
		openmpt::callbacks_istream istream( stream_callbacks, stream );
		return openmpt::module_impl::could_open_propability( istream, effort, std::make_shared<openmpt::logfunc_logger>( logfunc ? logfunc : openmpt_log_func_default, user ) );
	} OPENMPT_INTERFACE_CATCH_TO_LOG_FUNC;
	return 0.0;
}

openmpt_module * openmpt_module_create( openmpt_stream_callbacks stream_callbacks, void * stream, openmpt_log_func logfunc, void * user ) {
	try {
		openmpt_module * mod = (openmpt_module*)std::malloc( sizeof( openmpt_module ) );
		if ( !mod ) {
			throw std::bad_alloc();
		}
		mod->logfunc = logfunc ? logfunc : openmpt_log_func_default;
		mod->user = user;
		mod->impl = 0;
		try {
			openmpt::callbacks_istream istream( stream_callbacks, stream );
			mod->impl = new openmpt::module_impl( istream, std::make_shared<openmpt::logfunc_logger>( mod->logfunc, mod->user ) );
			return mod;
		} OPENMPT_INTERFACE_CATCH_TO_MOD_LOG_FUNC;
		delete mod->impl;
		mod->impl = 0;
		std::free( (void*)mod );
		mod = NULL;
	} OPENMPT_INTERFACE_CATCH;
	return NULL;
}

openmpt_module * openmpt_module_create_from_memory( const void * filedata, size_t filesize, openmpt_log_func logfunc, void * user ) {
	try {
		openmpt_module * mod = (openmpt_module*)std::malloc( sizeof( openmpt_module ) );
		if ( !mod ) {
			throw std::bad_alloc();
		}
		mod->logfunc = logfunc ? logfunc : openmpt_log_func_default;
		mod->user = user;
		mod->impl = 0;
		try {
			mod->impl = new openmpt::module_impl( filedata, filesize, std::make_shared<openmpt::logfunc_logger>( mod->logfunc, mod->user ) );
			return mod;
		} OPENMPT_INTERFACE_CATCH_TO_MOD_LOG_FUNC;
		delete mod->impl;
		mod->impl = 0;
		std::free( (void*)mod );
		mod = NULL;
	} OPENMPT_INTERFACE_CATCH;
	return NULL;
}

void openmpt_module_destroy( openmpt_module * mod ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		delete mod->impl;
		mod->impl = 0;
		std::free( (void*)mod );
		mod = NULL;
		return;
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return;
}

int openmpt_module_get_render_param( openmpt_module * mod, int command, int32_t * value ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		OPENMPT_INTERFACE_CHECK_POINTER( value );
		*value = mod->impl->get_render_param( (openmpt::module::render_param)command );
		return 1;
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}

int openmpt_module_set_render_param( openmpt_module * mod, int command, int32_t value ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		mod->impl->set_render_param( (openmpt::module::render_param)command, value );
		return 1;
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}

int openmpt_module_select_subsong( openmpt_module * mod, int32_t subsong ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		mod->impl->select_subsong( subsong );
		return 1;
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}

double openmpt_module_seek_seconds( openmpt_module * mod, double seconds ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->seek_seconds( seconds );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}

size_t openmpt_module_read_mono( openmpt_module * mod, int32_t samplerate, size_t count, int16_t * mono ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->read( samplerate, count, mono );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}
size_t openmpt_module_read_stereo( openmpt_module * mod, int32_t samplerate, size_t count, int16_t * left, int16_t * right ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->read( samplerate, count, left, right );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}
size_t openmpt_module_read_quad( openmpt_module * mod, int32_t samplerate, size_t count, int16_t * left, int16_t * right, int16_t * back_left, int16_t * back_right ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->read( samplerate, count, left, right, back_left, back_right );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}
size_t openmpt_module_read_float_mono( openmpt_module * mod, int32_t samplerate, size_t count, float * mono ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->read( samplerate, count, mono );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}
size_t openmpt_module_read_float_stereo( openmpt_module * mod, int32_t samplerate, size_t count, float * left, float * right ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->read( samplerate, count, left, right );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}
size_t openmpt_module_read_float_quad( openmpt_module * mod, int32_t samplerate, size_t count, float * left, float * right, float * back_left, float * back_right ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->read( samplerate, count, left, right, back_left, back_right );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}

double openmpt_module_get_current_position_seconds( openmpt_module * mod ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_duration_seconds();
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0.0;
}

double openmpt_module_get_duration_seconds( openmpt_module * mod ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_duration_seconds();
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0.0;
}

const char * openmpt_module_get_metadata_keys( openmpt_module * mod ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		std::string retval;
		bool first = true;
		std::vector<std::string> metadata_keys = mod->impl->get_metadata_keys();
		for ( std::vector<std::string>::iterator i = metadata_keys.begin(); i != metadata_keys.end(); ++i ) {
			if ( first ) {
				first = false;
			} else {
				retval += ";";
			}
			retval += *i;
		}
		return openmpt::strdup( retval.c_str() );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return NULL;
}
const char * openmpt_module_get_metadata( openmpt_module * mod, const char * key ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		OPENMPT_INTERFACE_CHECK_POINTER( key );
		return openmpt::strdup( mod->impl->get_metadata( key ).c_str() );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return NULL;
}

LIBOPENMPT_API int32_t openmpt_module_get_current_speed( openmpt_module * mod ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_current_speed();
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_current_tempo( openmpt_module * mod ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_current_tempo();
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_current_order( openmpt_module * mod ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_current_order();
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_current_pattern( openmpt_module * mod ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_current_pattern();
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_current_row( openmpt_module * mod ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_current_row();
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_current_playing_channels( openmpt_module * mod ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_current_playing_channels();
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}

LIBOPENMPT_API int32_t openmpt_module_get_num_subsongs( openmpt_module * mod ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_num_subsongs();
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_num_channels( openmpt_module * mod ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_num_channels();
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_num_orders( openmpt_module * mod ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_num_orders();
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_num_patterns( openmpt_module * mod ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_num_patterns();
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_num_instruments( openmpt_module * mod ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_num_instruments();
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_num_samples( openmpt_module * mod ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_num_channels();
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}

LIBOPENMPT_API const char * openmpt_module_get_subsong_name( openmpt_module * mod, int32_t index ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		std::vector<std::string> names = mod->impl->get_subsong_names();
		if ( names.size() >= (std::size_t)std::numeric_limits<int32_t>::max() ) {
			throw std::runtime_error("too many names");
		}
		if ( index < 0 || index >= (int32_t)names.size() ) {
			return openmpt::strdup( "" );
		}
		return openmpt::strdup( names[index].c_str() );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return NULL;
}
LIBOPENMPT_API const char * openmpt_module_get_channel_name( openmpt_module * mod, int32_t index ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		std::vector<std::string> names = mod->impl->get_channel_names();
		if ( names.size() >= (std::size_t)std::numeric_limits<int32_t>::max() ) {
			throw std::runtime_error("too many names");
		}
		if ( index < 0 || index >= (int32_t)names.size() ) {
			return openmpt::strdup( "" );
		}
		return openmpt::strdup( names[index].c_str() );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return NULL;
}
LIBOPENMPT_API const char * openmpt_module_get_order_name( openmpt_module * mod, int32_t index ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		std::vector<std::string> names = mod->impl->get_order_names();
		if ( names.size() >= (std::size_t)std::numeric_limits<int32_t>::max() ) {
			throw std::runtime_error("too many names");
		}
		if ( index < 0 || index >= (int32_t)names.size() ) {
			return openmpt::strdup( "" );
		}
		return openmpt::strdup( names[index].c_str() );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return NULL;
}
LIBOPENMPT_API const char * openmpt_module_get_pattern_name( openmpt_module * mod, int32_t index ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		std::vector<std::string> names = mod->impl->get_pattern_names();
		if ( names.size() >= (std::size_t)std::numeric_limits<int32_t>::max() ) {
			throw std::runtime_error("too many names");
		}
		if ( index < 0 || index >= (int32_t)names.size() ) {
			return openmpt::strdup( "" );
		}
		return openmpt::strdup( names[index].c_str() );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return NULL;
}
LIBOPENMPT_API const char * openmpt_module_get_instrument_name( openmpt_module * mod, int32_t index ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		std::vector<std::string> names = mod->impl->get_instrument_names();
		if ( names.size() >= (std::size_t)std::numeric_limits<int32_t>::max() ) {
			throw std::runtime_error("too many names");
		}
		if ( index < 0 || index >= (int32_t)names.size() ) {
			return openmpt::strdup( "" );
		}
		return openmpt::strdup( names[index].c_str() );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return NULL;
}
LIBOPENMPT_API const char * openmpt_module_get_sample_name( openmpt_module * mod, int32_t index ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		std::vector<std::string> names = mod->impl->get_sample_names();
		if ( names.size() >= (std::size_t)std::numeric_limits<int32_t>::max() ) {
			throw std::runtime_error("too many names");
		}
		if ( index < 0 || index >= (int32_t)names.size() ) {
			return openmpt::strdup( "" );
		}
		return openmpt::strdup( names[index].c_str() );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return NULL;
}

LIBOPENMPT_API int32_t openmpt_module_get_order_pattern( openmpt_module * mod, int32_t order ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_order_pattern( order );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}

LIBOPENMPT_API int32_t openmpt_module_get_pattern_num_rows( openmpt_module * mod, int32_t pattern ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_pattern_num_rows( pattern );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}

LIBOPENMPT_API uint8_t openmpt_module_get_pattern_row_channel_command( openmpt_module * mod, int32_t pattern, int32_t row, int32_t channel, int command ) {
	try {
		OPENMPT_INTERFACE_CHECK_SOUNDFILE( mod );
		return mod->impl->get_pattern_row_channel_command( pattern, row, channel, command );
	} OPENMPT_INTERFACE_CATCH_TO_LOG;
	return 0;
}

#undef OPENMPT_INTERFACE_CHECK_POINTER
#undef OPENMPT_INTERFACE_CHECK_SOUNDFILE
#undef OPENMPT_INTERFACE_CATCH_TO_LOG
#undef OPENMPT_INTERFACE_CATCH_TO_MOD_LOG_FUNC
#undef OPENMPT_INTERFACE_CATCH_TO_LOG_FUNC
#undef OPENMPT_INTERFACE_CATCH

}; // extern "C"

//#endif // NO_LIBOPENMPT_C
