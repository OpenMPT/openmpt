/*
 * libopenmpt_interactive.hpp
 * --------------------------
 * Purpose: libopenmpt public c++ interface for interactive libopenmpt use
 * Notes  : The API defined in this file is currently not considered stable yet. Do NOT ship in distributions yet to avoid applications relying on API/ABI compatibility.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_INTERACTIVE_HPP
#define LIBOPENMPT_INTERACTIVE_HPP

#include "libopenmpt_config.h"
#include "libopenmpt.hpp"

#if !defined(LIBOPENMPT_INTERACTIVE_IS_EXPERIMENTAL)

#error "libopenmpt_interactive is still completely experimental. The ABI and API WILL change. Use at your own risk, and use only internally for now. You have to #define LIBOPENMPT_INTERACTIVE_IS_EXPERIMENTAL to use it."

#else // LIBOPENMPT_INTERACTIVE_IS_EXPERIMENTAL

namespace openmpt {

class interactive_module_impl;

class /* LIBOPENMPT_CXX_API */ interactive_module : public module {
	
private:
	interactive_module_impl * interactive_impl;
private:
	// non-copyable
	interactive_module( const interactive_module & );
	void operator = ( const interactive_module & );
public:
	interactive_module( std::istream & stream, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	interactive_module( const std::vector<char> & data, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	interactive_module( const char * data, std::size_t size, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	interactive_module( const void * data, std::size_t size, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	virtual ~interactive_module();

public:



	// interface
	/* add stuff here */



}; // class interactive_module

} // namespace openmpt

#endif // LIBOPENMPT_INTERACTIVE_IS_EXPERIMENTAL

#endif // LIBOPENMPT_INTERACTIVE_HPP
