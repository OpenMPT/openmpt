/*
 * libopenmpt_ext.hpp
 * ------------------
 * Purpose: libopenmpt public c++ interface for extending libopenmpt
 * Notes  : The API defined in this file is currently not considered stable yet. Do NOT ship in distributions yet to avoid applications relying on API/ABI compatibility.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_EXT_HPP
#define LIBOPENMPT_EXT_HPP

#include "libopenmpt_config.h"
#include "libopenmpt.hpp"

#if !defined(LIBOPENMPT_EXT_IS_EXPERIMENTAL)

#error "libopenmpt_ext is still completely experimental. The ABIs and APIs WILL change. Use at your own risk, and use only internally for now. You have to #define LIBOPENMPT_EXT_IS_EXPERIMENTAL to use it."

#else // LIBOPENMPT_EXT_IS_EXPERIMENTAL

namespace openmpt {

class module_ext_impl;

class LIBOPENMPT_CXX_API module_ext : public module {
	
private:
	module_ext_impl * ext_impl;
private:
	// non-copyable
	module_ext( const module_ext & );
	void operator = ( const module_ext & );
public:
	module_ext( std::istream & stream, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	module_ext( const std::vector<char> & data, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	module_ext( const char * data, std::size_t size, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	module_ext( const void * data, std::size_t size, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	virtual ~module_ext();

public:

	void * get_interface( const std::string & interface_id );

}; // class module_ext

namespace ext {

#define LIBOPENMPT_DECLARE_EXT_INTERFACE(name) \
	static const char name ## _id [] = # name ; \
	class name; \
/**/

#define LIBOPENMPT_EXT_INTERFACE(name) \
	protected: \
		name () {} \
		virtual ~ name() {} \
	public: \
/**/


LIBOPENMPT_DECLARE_EXT_INTERFACE(pattern_vis)

class pattern_vis {

	LIBOPENMPT_EXT_INTERFACE(pattern_vis)

	enum effect_type {

		effect_unknown = 0,
		effect_general = 1,
		effect_global = 2,
		effect_volume = 3,
		effect_panning = 4,
		effect_pitch = 5

	}; // enum effect_type

	virtual effect_type get_pattern_row_channel_volume_effect_type( std::int32_t pattern, std::int32_t row, std::int32_t channel ) const = 0;

	virtual effect_type get_pattern_row_channel_effect_type( std::int32_t pattern, std::int32_t row, std::int32_t channel ) const = 0;

}; // class pattern_vis



/* add stuff here */



#undef LIBOPENMPT_DECLARE_EXT_INTERFACE
#undef LIBOPENMPT_EXT_INTERFACE

} // namespace ext

} // namespace openmpt

#endif // LIBOPENMPT_EXT_IS_EXPERIMENTAL

#endif // LIBOPENMPT_EXT_HPP
