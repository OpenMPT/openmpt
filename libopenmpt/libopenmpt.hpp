/*
 * libopenmpt.hpp
 * --------------
 * Purpose: libopenmpt public c++ interface
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_HPP
#define LIBOPENMPT_HPP

#include "libopenmpt_config.h"

#include <iostream>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <cstdint>

namespace openmpt {

class LIBOPENMPT_CXX_API exception : public std::runtime_error {
public:
	exception( const std::string & text );
}; // class exception

LIBOPENMPT_CXX_API std::uint32_t get_library_version();

LIBOPENMPT_CXX_API std::uint32_t get_core_version();

namespace detail {

LIBOPENMPT_CXX_API void version_compatible_or_throw( std::int32_t api_version );

class api_version_checker {
public:
	inline api_version_checker( std::int32_t api_version = OPENMPT_API_VERSION ) {
		version_compatible_or_throw( api_version );
	}
}; // class api_version_checker

} // namespace detail

namespace string {

static const char * const library_version = "library_version";
static const char * const core_version    = "core_version";
static const char * const build           = "build";
static const char * const credits         = "credits";
static const char * const contact         = "contact";

LIBOPENMPT_CXX_API std::string get( const std::string & key );

} // namespace string

LIBOPENMPT_CXX_API std::vector<std::string> get_supported_extensions();

LIBOPENMPT_CXX_API bool is_extension_supported( const std::string & extension );

LIBOPENMPT_CXX_API double could_open_propability( std::istream & stream, double effort = 1.0, std::ostream & log = std::clog, const detail::api_version_checker & apicheck = detail::api_version_checker() );

class module_impl;

class interactive_module;

class LIBOPENMPT_CXX_API module {

	friend class interactive_module;

public:

	enum render_param {
		RENDER_MASTERGAIN_MILLIBEL          = 1,
		RENDER_STEREOSEPARATION_PERCENT     = 2,
		RENDER_INTERPOLATION_FILTER_LENGTH  = 3,
		RENDER_VOLUMERAMP_UP_MICROSECONDS   = 4,
		RENDER_VOLUMERAMP_DOWN_MICROSECONDS = 5
	};

	enum command_index {
		command_note        = 0,
		command_instrument  = 1,
		command_volumeffect = 2,
		command_effect      = 3,
		command_volume      = 4,
		command_parameter   = 5
	};

private:
	module_impl * impl;
private:
	// non-copyable
	module( const module & );
	void operator = ( const module & );
private:
	// for interactive_module
	module();
	void set_impl( module_impl * i );
public:
	module( std::istream & stream, std::ostream & log = std::clog, const detail::api_version_checker & apicheck = detail::api_version_checker() );
	module( const std::vector<std::uint8_t> & data, std::ostream & log = std::clog, const detail::api_version_checker & apicheck = detail::api_version_checker() );
	module( const std::uint8_t * beg, const std::uint8_t * end, std::ostream & log = std::clog, const detail::api_version_checker & apicheck = detail::api_version_checker() );
	module( const std::uint8_t * data, std::size_t size, std::ostream & log = std::clog, const detail::api_version_checker & apicheck = detail::api_version_checker() );
	module( const std::vector<char> & data, std::ostream & log = std::clog, const detail::api_version_checker & apicheck = detail::api_version_checker() );
	module( const char * beg, const char * end, std::ostream & log = std::clog, const detail::api_version_checker & apicheck = detail::api_version_checker() );
	module( const char * data, std::size_t size, std::ostream & log = std::clog, const detail::api_version_checker & apicheck = detail::api_version_checker() );
	module( const void * data, std::size_t size, std::ostream & log = std::clog, const detail::api_version_checker & apicheck = detail::api_version_checker() );
	virtual ~module();
public:

	std::int32_t get_render_param( int param ) const;
	void set_render_param( int param, std::int32_t value );

	void select_subsong( std::int32_t subsong );
	void set_repeat_count( std::int32_t repeat_count );
	std::int32_t get_repeat_count() const;
 
	double seek_seconds( double seconds );

	std::size_t read( std::int32_t samplerate, std::size_t count, std::int16_t * mono );
	std::size_t read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right );
	std::size_t read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right, std::int16_t * rear_left, std::int16_t * rear_right );
	std::size_t read( std::int32_t samplerate, std::size_t count, float * mono );
	std::size_t read( std::int32_t samplerate, std::size_t count, float * left, float * right );
	std::size_t read( std::int32_t samplerate, std::size_t count, float * left, float * right, float * rear_left, float * rear_right );

	double get_current_position_seconds() const;

	double get_duration_seconds() const;

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

	std::int32_t get_order_pattern( std::int32_t order ) const;

	std::int32_t get_pattern_num_rows( std::int32_t pattern ) const;

	std::uint8_t get_pattern_row_channel_command( std::int32_t pattern, std::int32_t row, std::int32_t channel, int command ) const;

	std::vector<std::string> get_ctls() const;

	std::string ctl_get_string( const std::string & ctl ) const;
	double ctl_get_double( const std::string & ctl ) const;
	std::int64_t ctl_get_int64( const std::string & ctl ) const;

	void ctl_set( const std::string & ctl, const std::string & value );
	void ctl_set( const std::string & ctl, double value );
	void ctl_set( const std::string & ctl, std::int64_t value );

	// remember to add new functions to both C and C++ interfaces and to increase OPENMPT_API_VERSION_MINOR

}; // class module

} // namespace openmpt

#endif // LIBOPENMPT_HPP
