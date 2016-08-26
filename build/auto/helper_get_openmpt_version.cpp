
#include <iostream>
#include <string>
#include <vector>

#define OPENMPT_NAMESPACE_BEGIN
#define OPENMPT_NAMESPACE_END
#include "common/versionNumber.h"

#include "libopenmpt/libopenmpt_version.h"

int main( int argc, char * argv [] ) {
	std::vector<std::string> args = std::vector<std::string>( argv, argv + argc );
	for ( std::vector<std::string>::const_iterator i = args.begin(); i != args.end(); ++i ) {
		if ( i == args.begin() ) {
			// skip program name
			continue;
		}
		std::string arg = *i;
		if ( arg == "" ) {
			std::cout << "unknown" << std::endl;

		} else if ( arg == "openmpt" ) {
			std::cout << MPT_VERSION_STR << std::endl;

#ifdef OPENMPT_API_VERSION_MAJOR
		} else if ( arg == "libopenmpt-version-major" ) {
			std::cout << OPENMPT_API_VERSION_MAJOR << std::endl;
#endif

#ifdef OPENMPT_API_VERSION_MINOR
		} else if ( arg == "libopenmpt-version-minor" ) {
			std::cout << OPENMPT_API_VERSION_MINOR << std::endl;
#endif

#ifdef OPENMPT_API_VERSION_PATCH
		} else if ( arg == "libopenmpt-version-patch" ) {
			std::cout << OPENMPT_API_VERSION_PATCH << std::endl;
#endif

#ifdef OPENMPT_API_VERSION_PREREL
		} else if ( arg == "libopenmpt-version-prerel" ) {
			std::cout << OPENMPT_API_VERSION_PREREL << std::endl;
#endif

#ifdef OPENMPT_API_VERSION_STRING
		} else if ( arg == "libopenmpt-version-string" ) {
			std::cout << OPENMPT_API_VERSION_STRING << std::endl;
#endif

		} else {
			std::cout << "unknown" << std::endl;			
		}
	}
	return 0;
}
