/*
 * libopenmpt_cxx.cpp
 * ------------------
 * Purpose: libopenmpt C++ interface implementation
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "common/stdafx.h"

#include "libopenmpt_internal.h"
#include "libopenmpt.hpp"

#include "libopenmpt_impl.hpp"

#include <algorithm>

//#ifndef NO_LIBOPENMPT_CXX

namespace openmpt {

exception::exception( const char * text ) : std::exception( text ) {
	return;
}

std::uint32_t get_library_version() {
	return openmpt::version::get_library_version();
}

std::uint32_t get_core_version() {
	return openmpt::version::get_core_version();
}

namespace string {

std::string get( const std::string & key ) {
	return openmpt::version::get_string( key );
}

} // namespace string

std::vector<std::string> get_supported_extensions() {
	std::vector<std::string> retval;
	std::vector<const char *> extensions = CSoundFile::GetSupportedExtensions( false );
	std::copy( extensions.begin(), extensions.end(), std::back_insert_iterator<std::vector<std::string> >( retval ) );
	return retval;
}

bool is_extension_supported( const std::string & extension ) {
	std::vector<std::string> extensions = get_supported_extensions();
	std::string lowercase_ext = extension;
	std::transform( lowercase_ext.begin(), lowercase_ext.end(), lowercase_ext.begin(), tolower );
	return std::find( extensions.begin(), extensions.end(), lowercase_ext ) != extensions.end();
}

double could_open_propability( std::istream & stream, double effort, std::ostream & log, const detail::api_version_checker & ) {
	return openmpt::module_impl::could_open_propability( stream, effort, std::make_shared<std_ostream_log>( log ) );
}

module::module( const module & ) {
	throw std::runtime_error("openmpt::module is non-copyable");
}

void module::operator = ( const module & ) {
	throw std::runtime_error("openmpt::module is non-copyable");
}

module::module() : impl(0) {
	return;
}

void module::set_impl( module_impl * i ) {
	impl = i;
}

module::module( std::istream & stream, std::ostream & log, const detail::api_version_checker & ) : impl(0) {
	impl = new module_impl( stream, std::make_shared<std_ostream_log>( log ) );
}

module::module( const std::vector<std::uint8_t> & data, std::ostream & log, const detail::api_version_checker & ) : impl(0) {
	impl = new module_impl( data, std::make_shared<std_ostream_log>( log ) );
}

module::module( const std::uint8_t * beg, const std::uint8_t * end, std::ostream & log, const detail::api_version_checker & ) : impl(0) {
	impl = new module_impl( beg, end - beg, std::make_shared<std_ostream_log>( log ) );
}

module::module( const std::vector<char> & data, std::ostream & log, const detail::api_version_checker & ) : impl(0) {
	impl = new module_impl( data, std::make_shared<std_ostream_log>( log ) );
}

module::module( const char * beg, const char * end, std::ostream & log, const detail::api_version_checker & apicheck ) : impl(0) {
	impl = new module_impl( beg, end - beg, std::make_shared<std_ostream_log>( log ) );
}

module::module( const char * data, std::size_t size, std::ostream & log, const detail::api_version_checker & ) : impl(0) {
	impl = new module_impl( data, size, std::make_shared<std_ostream_log>( log ) );
}

module::module( const void * data, std::size_t size, std::ostream & log, const detail::api_version_checker & ) : impl(0) {
	impl = new module_impl( data, size, std::make_shared<std_ostream_log>( log ) );
}

module::~module() {
	delete impl;
	impl = 0;
}

std::int32_t module::get_render_param( int command ) const {
	return impl->get_render_param( command );
}

void module::set_render_param( int command, std::int32_t value ) {
	impl->set_render_param( command, value );
}

void module::select_subsong( std::int32_t subsong ) {
	impl->select_subsong( subsong );
}

double module::seek_seconds( double seconds ) {
	return impl->seek_seconds( seconds );
}

std::size_t module::read( std::int32_t samplerate, std::size_t count, std::int16_t * mono ) {
	return impl->read( samplerate, count, mono );
}
std::size_t module::read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right ) {
	return impl->read( samplerate, count, left, right );
}
std::size_t module::read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right, std::int16_t * back_left, std::int16_t * back_right ) {
	return impl->read( samplerate, count, left, right, back_left, back_right );
}
std::size_t module::read( std::int32_t samplerate, std::size_t count, float * mono ) {
	return impl->read( samplerate, count, mono );
}
std::size_t module::read( std::int32_t samplerate, std::size_t count, float * left, float * right ) {
	return impl->read( samplerate, count, left, right );
}
std::size_t module::read( std::int32_t samplerate, std::size_t count, float * left, float * right, float * back_left, float * back_right ) {
	return impl->read( samplerate, count, left, right, back_left, back_right );
}

double module::get_current_position_seconds() const {
	return impl->get_current_position_seconds();
}

double module::get_duration_seconds() const {
	return impl->get_duration_seconds();
}

std::vector<std::string> module::get_metadata_keys() const {
	return impl->get_metadata_keys();
}
std::string module::get_metadata( const std::string & key ) const {
	return impl->get_metadata( key );
}

std::int32_t module::get_current_speed() const {
	return impl->get_current_speed();
}
std::int32_t module::get_current_tempo() const {
	return impl->get_current_tempo();
}
std::int32_t module::get_current_order() const {
	return impl->get_current_order();
}
std::int32_t module::get_current_pattern() const {
	return impl->get_current_pattern();
}
std::int32_t module::get_current_row() const {
	return impl->get_current_row();
}
std::int32_t module::get_current_playing_channels() const {
	return impl->get_current_playing_channels();
}

std::int32_t module::get_num_subsongs() const {
	return impl->get_num_subsongs();
}
std::int32_t module::get_num_channels() const {
	return impl->get_num_channels();
}
std::int32_t module::get_num_orders() const {
	return impl->get_num_orders();
}
std::int32_t module::get_num_patterns() const {
	return impl->get_num_patterns();
}
std::int32_t module::get_num_instruments() const {
	return impl->get_num_instruments();
}
std::int32_t module::get_num_samples() const {
	return impl->get_num_samples();
}

std::vector<std::string> module::get_subsong_names() const {
	return impl->get_subsong_names();
}
std::vector<std::string> module::get_channel_names() const {
	return impl->get_channel_names();
}
std::vector<std::string> module::get_order_names() const {
	return impl->get_order_names();
}
std::vector<std::string> module::get_pattern_names() const {
	return impl->get_pattern_names();
}
std::vector<std::string> module::get_instrument_names() const {
	return impl->get_instrument_names();
}
std::vector<std::string> module::get_sample_names() const {
	return impl->get_sample_names();
}

std::int32_t module::get_order_pattern( std::int32_t order ) const {
	return impl->get_order_pattern( order );
}
std::int32_t module::get_pattern_num_rows( std::int32_t pattern ) const {
	return impl->get_pattern_num_rows( pattern );
}

std::uint8_t module::get_pattern_row_channel_command( std::int32_t pattern, std::int32_t row, std::int32_t channel, int command ) const {
	return impl->get_pattern_row_channel_command( pattern, row, channel, command );
}

} // namespace openmpt

//#endif // NO_LIBOPENMPT_CXX
