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
class Dither;

namespace openmpt {

namespace version {

std::uint32_t get_library_version();
std::uint32_t get_core_version();
std::string get_string( const std::string & key );

} // namespace version

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
	double m_currentPositionSeconds;
	std::unique_ptr<CSoundFile> m_sndFile;
	std::unique_ptr<Dither> m_Dither;
	float m_Gain;
	std::vector<std::string> m_loaderMessages;
public:
	void PushToCSoundFileLog( const std::string & text ) const;
	void PushToCSoundFileLog( int loglevel, const std::string & text ) const;
protected:
	std::string mod_string_to_utf8( const std::string & encoded ) const;
	void apply_mixer_settings( std::int32_t samplerate, int channels );
	void apply_libopenmpt_defaults();
	void init( const std::map< std::string, std::string > & ctls );
	static void load( CSoundFile & sndFile, const FileReader & file );
	void load( const FileReader & file );
	std::size_t read_wrapper( std::size_t count, std::int16_t * left, std::int16_t * right, std::int16_t * rear_left, std::int16_t * rear_right );
	std::size_t read_wrapper( std::size_t count, float * left, float * right, float * rear_left, float * rear_right );
	std::size_t read_interleaved_wrapper( std::size_t count, std::size_t channels, std::int16_t * interleaved );
	std::size_t read_interleaved_wrapper( std::size_t count, std::size_t channels, float * interleaved );
	std::pair< std::string, std::string > format_and_highlight_pattern_row_channel_command( std::int32_t p, std::int32_t r, std::int32_t c, int command ) const;
	std::pair< std::string, std::string > format_and_highlight_pattern_row_channel( std::int32_t p, std::int32_t r, std::int32_t c, std::size_t width, bool pad ) const;
public:
	static std::vector<std::string> get_supported_extensions();
	static bool is_extension_supported( const std::string & extension );
	static double could_open_propability( std::istream & stream, double effort, std::shared_ptr<log_interface> log );
	module_impl( std::istream & stream, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls );
	module_impl( const std::vector<std::uint8_t> & data, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls );
	module_impl( const std::vector<char> & data, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls );
	module_impl( const std::uint8_t * data, std::size_t size, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls );
	module_impl( const char * data, std::size_t size, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls );
	module_impl( const void * data, std::size_t size, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls );
	~module_impl();
public:
	void select_subsong( std::int32_t subsong );
	void set_repeat_count( std::int32_t repeat_count );
	std::int32_t get_repeat_count() const;
	double get_duration_seconds() const;
	double set_position_seconds( double seconds );
	double get_position_seconds() const;
	double set_position_order_row( std::int32_t order, std::int32_t row );
	std::int32_t get_render_param( int param ) const;
	void set_render_param( int param, std::int32_t value );
	std::size_t read( std::int32_t samplerate, std::size_t count, std::int16_t * mono );
	std::size_t read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right );
	std::size_t read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right, std::int16_t * rear_left, std::int16_t * rear_right );
	std::size_t read( std::int32_t samplerate, std::size_t count, float * mono );
	std::size_t read( std::int32_t samplerate, std::size_t count, float * left, float * right );
	std::size_t read( std::int32_t samplerate, std::size_t count, float * left, float * right, float * rear_left, float * rear_right );
	std::size_t read_interleaved_stereo( std::int32_t samplerate, std::size_t count, std::int16_t * interleaved_stereo );
	std::size_t read_interleaved_quad( std::int32_t samplerate, std::size_t count, std::int16_t * interleaved_quad );
	std::size_t read_interleaved_stereo( std::int32_t samplerate, std::size_t count, float * interleaved_stereo );
	std::size_t read_interleaved_quad( std::int32_t samplerate, std::size_t count, float * interleaved_quad );
	std::vector<std::string> get_metadata_keys() const;
	std::string get_metadata( const std::string & key ) const;
	std::int32_t get_current_speed() const;
	std::int32_t get_current_tempo() const;
	std::int32_t get_current_order() const;
	std::int32_t get_current_pattern() const;
	std::int32_t get_current_row() const;
	std::int32_t get_current_playing_channels() const;
	float get_current_channel_vu_mono( std::int32_t channel ) const;
	float get_current_channel_vu_left( std::int32_t channel ) const;
	float get_current_channel_vu_right( std::int32_t channel ) const;
	float get_current_channel_vu_rear_left( std::int32_t channel ) const;
	float get_current_channel_vu_rear_right( std::int32_t channel ) const;
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
	std::string format_pattern_row_channel_command( std::int32_t p, std::int32_t r, std::int32_t c, int cmd ) const;
	std::string highlight_pattern_row_channel_command( std::int32_t p, std::int32_t r, std::int32_t c, int cmd ) const;
	std::string format_pattern_row_channel( std::int32_t p, std::int32_t r, std::int32_t c, std::size_t width, bool pad ) const;
	std::string highlight_pattern_row_channel( std::int32_t p, std::int32_t r, std::int32_t c, std::size_t width, bool pad ) const;
	std::vector<std::string> get_ctls() const;
	std::string ctl_get( const std::string & ctl ) const;
	void ctl_set( const std::string & ctl, const std::string & value );
}; // class module_impl

} // namespace openmpt

#endif // LIBOPENMPT_IMPL_HPP
