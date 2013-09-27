
#include "BuildSettings.h"

#include "libopenmpt_internal.h"
#include "libopenmpt_interactive.hpp"
#include "libopenmpt_impl.hpp"

#include <stdexcept>

#ifndef NO_LIBOPENMPT_CXX

namespace openmpt {

class interactive_module_impl : public module_impl {
public:
	interactive_module_impl( std::istream & stream, std::ostream & log, const std::map< std::string, std::string > & ctls ) : module_impl( stream, std::make_shared<std_ostream_log>( log ), ctls ) {
		init();
	}
	interactive_module_impl( const std::vector<char> & data, std::ostream & log, const std::map< std::string, std::string > & ctls ) : module_impl( data, std::make_shared<std_ostream_log>( log ), ctls ) {
		init();
	}
	interactive_module_impl( const char * data, std::size_t size, std::ostream & log, const std::map< std::string, std::string > & ctls ) : module_impl( data, size, std::make_shared<std_ostream_log>( log ), ctls ) {
		init();
	}
	interactive_module_impl( const void * data, std::size_t size, std::ostream & log, const std::map< std::string, std::string > & ctls ) : module_impl( data, size, std::make_shared<std_ostream_log>( log ), ctls ) {
		init();
	}

private:



	// members
	/* add stuff here */



private:

	void init() {



		// constructor
		/* add stuff here */



	}

public:

	~interactive_module_impl() {



		// destructor
		/* add stuff here */



	}

public:



	// interface
	/* add stuff here */



}; // class interactive_module_impl



// interface
/* add stuff here */



interactive_module::interactive_module( std::istream & stream, std::ostream & log, const std::map< std::string, std::string > & ctls ) : interactive_impl(0) {
	interactive_impl = new interactive_module_impl( stream, log, ctls );
	set_impl( interactive_impl );
}
interactive_module::interactive_module( const std::vector<char> & data, std::ostream & log, const std::map< std::string, std::string > & ctls ) : interactive_impl(0) {
	interactive_impl = new interactive_module_impl( data, log, ctls );
	set_impl( interactive_impl );
}
interactive_module::interactive_module( const char * data, std::size_t size, std::ostream & log, const std::map< std::string, std::string > & ctls ) : interactive_impl(0) {
	interactive_impl = new interactive_module_impl( data, size, log, ctls );
	set_impl( interactive_impl );
}
interactive_module::interactive_module( const void * data, std::size_t size, std::ostream & log, const std::map< std::string, std::string > & ctls ) : interactive_impl(0) {
	interactive_impl = new interactive_module_impl( data, size, log, ctls );
	set_impl( interactive_impl );
}
interactive_module::~interactive_module() {
	set_impl( 0 );
	delete interactive_impl;
	interactive_impl = 0;
}
interactive_module::interactive_module( const interactive_module & other ) : module(other) {
	throw std::runtime_error("openmpt::interactive_module is non-copyable");
}
void interactive_module::operator = ( const interactive_module & ) {
	throw std::runtime_error("openmpt::interactive_module is non-copyable");
}

} // namespace openmpt

#endif // NO_LIBOPENMPT_CXX
