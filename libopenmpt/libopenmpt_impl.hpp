/*
 * libopenmpt_impl.hpp
 * -------------------
 * Purpose: libopenmpt private interface
 * Notes  : This is not a public header. Do NOT ship in distributions dev packages.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_IMPL_HPP
#define LIBOPENMPT_IMPL_HPP

#include "libopenmpt_internal.h"
#include "libopenmpt.hpp"

#include <memory>
#include <ostream>

// forward declarations
class FileReader;
class CSoundFile;

namespace openmpt {

namespace version {

std::uint32_t get_library_version();
std::uint32_t get_core_version();
std::string get_string( const std::string & key );
int get_version_compatbility( std::uint32_t api_version );

} // namespace version

// has to be exported for type_info lookup to work
class LIBOPENMPT_CXX_API exception_message : public exception {
public:
	exception_message( const char * text_ ) throw();
	virtual ~exception_message() throw();
public:
	virtual const char * what() const throw();
private:
	const char * text;
}; // class exception_message

class log_interface {
protected:
	log_interface();
public:
	virtual ~log_interface();
	virtual void log( const std::string & message ) const = 0;
}; // class log_interface

class std_ostream_log : public log_interface {
private:
	std::ostream & destination;
public:
	std_ostream_log( std::ostream & dst );
	virtual ~std_ostream_log();
	virtual void log( const std::string & message ) const;
}; // class CSoundFileLog_std_ostream

class log_forwarder;

class module_impl {
protected:
	std::shared_ptr<log_interface> m_Log;
	std::unique_ptr<log_forwarder> m_LogForwarder;
	std::vector<std::int16_t> m_int16Buffer;
	std::vector<float> m_floatBuffer;
	double m_currentPositionSeconds;
	std::unique_ptr<CSoundFile> m_sndFile;
	std::vector<std::string> m_loaderMessages;
public:
	void PushToCSoundFileLog( const std::string & text ) const;
	void PushToCSoundFileLog( int loglevel, const std::string & text ) const;
private:
	std::int32_t get_quality() const;
	void set_quality( std::int32_t value );
	void apply_mixer_settings( std::int32_t samplerate, int channels, bool format_float );
	void apply_libopenmpt_defaults();
	void init();
	static void load( CSoundFile & sndFile, const FileReader & file );
	void load( const FileReader & file );
	std::size_t read_wrapper( void * buffer, std::size_t count );
public:
	static std::vector<std::string> get_supported_extensions();
	static bool is_extension_supported( const std::string & extension );
	static double could_open_propability( std::istream & stream, double effort, std::shared_ptr<log_interface> log );
	module_impl( std::istream & stream, std::shared_ptr<log_interface> log );
	module_impl( const std::vector<std::uint8_t> & data, std::shared_ptr<log_interface> log );
	module_impl( const std::vector<char> & data, std::shared_ptr<log_interface> log );
	module_impl( const std::uint8_t * data, std::size_t size, std::shared_ptr<log_interface> log );
	module_impl( const char * data, std::size_t size, std::shared_ptr<log_interface> log );
	module_impl( const void * data, std::size_t size, std::shared_ptr<log_interface> log );
	~module_impl();
public:
	std::int32_t get_render_param( int command ) const;
	void set_render_param( int command, std::int32_t value );
	std::size_t read( std::int32_t samplerate, std::size_t count, std::int16_t * mono );
	std::size_t read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right );
	std::size_t read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right, std::int16_t * rear_left, std::int16_t * rear_right );
	std::size_t read( std::int32_t samplerate, std::size_t count, float * mono );
	std::size_t read( std::int32_t samplerate, std::size_t count, float * left, float * right );
	std::size_t read( std::int32_t samplerate, std::size_t count, float * left, float * right, float * rear_left, float * rear_right );
	double get_duration_seconds() const;
	double get_current_position_seconds() const;
	void select_subsong( std::int32_t subsong );
	double seek_seconds( double seconds );
	std::vector<std::string> get_metadata_keys() const;
	std::string get_metadata( const std::string & key ) const;
	std::int32_t get_current_speed() const;
	std::int32_t get_current_tempo() const;
	std::int32_t get_current_order() const;
	std::int32_t get_current_pattern() const;
	std::int32_t get_current_row() const;
	std::int32_t get_current_playing_channels() const;
	std::int32_t get_num_subsongs() const;
	std::int32_t get_num_channels() const;
	std::int32_t get_num_orders() const;
	std::int32_t get_num_patterns() const;
	std::int32_t get_num_instruments() const;
	std::int32_t get_num_samples() const;
	std::vector<std::string> get_subsong_names() const;
	std::vector<std::string> get_channel_names() const;
	std::vector<std::string> get_order_names() const;
	std::vector<std::string> get_pattern_names() const;
	std::vector<std::string> get_instrument_names() const;
	std::vector<std::string> get_sample_names() const;
	std::int32_t get_order_pattern( std::int32_t o ) const;
	std::int32_t get_pattern_num_rows( std::int32_t p ) const;
	std::uint8_t get_pattern_row_channel_command( std::int32_t p, std::int32_t r, std::int32_t c, int cmd ) const;
}; // class module_impl

} // namespace openmpt

#endif // LIBOPENMPT_IMPL_HPP
