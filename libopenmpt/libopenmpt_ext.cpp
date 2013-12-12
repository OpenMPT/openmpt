
#include "common/stdafx.h"

#define LIBOPENMPT_EXT_IS_EXPERIMENTAL

#include "libopenmpt_internal.h"
#include "libopenmpt_ext.hpp"
#include "libopenmpt_impl.hpp"

#include <stdexcept>

#include "soundlib/Sndfile.h"

#ifndef NO_LIBOPENMPT_CXX

namespace openmpt {

class module_ext_impl
	: public module_impl



	/* add stuff here */



{
public:
	module_ext_impl( std::istream & stream, std::ostream & log, const std::map< std::string, std::string > & ctls ) : module_impl( stream, std::make_shared<std_ostream_log>( log ), ctls ) {
		init();
	}
	module_ext_impl( const std::vector<char> & data, std::ostream & log, const std::map< std::string, std::string > & ctls ) : module_impl( data, std::make_shared<std_ostream_log>( log ), ctls ) {
		init();
	}
	module_ext_impl( const char * data, std::size_t size, std::ostream & log, const std::map< std::string, std::string > & ctls ) : module_impl( data, size, std::make_shared<std_ostream_log>( log ), ctls ) {
		init();
	}
	module_ext_impl( const void * data, std::size_t size, std::ostream & log, const std::map< std::string, std::string > & ctls ) : module_impl( data, size, std::make_shared<std_ostream_log>( log ), ctls ) {
		init();
	}

private:



	/* add stuff here */



private:

	void init() {



		/* add stuff here */



	}

public:

	~module_ext_impl() {



		/* add stuff here */



	}

public:

	void * get_interface( const std::string & interface_id ) {
		if ( interface_id.empty() ) {
			return 0;



			/* add stuff here */



		} else {
			return 0;
		}
	}



	/* add stuff here */



}; // class module_ext_impl

module_ext::module_ext( std::istream & stream, std::ostream & log, const std::map< std::string, std::string > & ctls ) : ext_impl(0) {
	ext_impl = new module_ext_impl( stream, log, ctls );
	set_impl( ext_impl );
}
module_ext::module_ext( const std::vector<char> & data, std::ostream & log, const std::map< std::string, std::string > & ctls ) : ext_impl(0) {
	ext_impl = new module_ext_impl( data, log, ctls );
	set_impl( ext_impl );
}
module_ext::module_ext( const char * data, std::size_t size, std::ostream & log, const std::map< std::string, std::string > & ctls ) : ext_impl(0) {
	ext_impl = new module_ext_impl( data, size, log, ctls );
	set_impl( ext_impl );
}
module_ext::module_ext( const void * data, std::size_t size, std::ostream & log, const std::map< std::string, std::string > & ctls ) : ext_impl(0) {
	ext_impl = new module_ext_impl( data, size, log, ctls );
	set_impl( ext_impl );
}
module_ext::~module_ext() {
	set_impl( 0 );
	delete ext_impl;
	ext_impl = 0;
}
module_ext::module_ext( const module_ext & other ) : module(other) {
	throw std::runtime_error("openmpt::module_ext is non-copyable");
}
void module_ext::operator = ( const module_ext & ) {
	throw std::runtime_error("openmpt::module_ext is non-copyable");
}

void * module_ext::get_interface( const std::string & interface_id ) {
	return ext_impl->get_interface( interface_id );
}

} // namespace openmpt

#endif // NO_LIBOPENMPT_CXX
