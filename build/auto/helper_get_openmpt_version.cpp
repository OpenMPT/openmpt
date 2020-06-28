
#define LIBOPENMPT_BUILD
#include "common/BuildSettings.h"

#include <iostream>
#include <string>
#include <vector>

#include "common/versionNumber.h"
#include "libopenmpt/libopenmpt_version.h"

#define VER_HELPER_STRINGIZE(x) #x
#define VER_STRINGIZE(x)        VER_HELPER_STRINGIZE(x)

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
			std::cout << VER_STRINGIZE(VER_MAJORMAJOR) << "." << VER_STRINGIZE(VER_MAJOR) << "." << VER_STRINGIZE(VER_MINOR) << "." << VER_STRINGIZE(VER_MINORMINOR) << std::endl;

		} else if ( arg == "openmpt-version-majormajor" ) {
			std::cout << VER_STRINGIZE(VER_MAJORMAJOR) << std::endl;

		} else if ( arg == "openmpt-version-major" ) {
			std::cout << VER_STRINGIZE(VER_MAJOR) << std::endl;

		} else if ( arg == "openmpt-version-minor" ) {
			std::cout << VER_STRINGIZE(VER_MINOR) << std::endl;

		} else if ( arg == "openmpt-version-minorminor" ) {
			std::cout << VER_STRINGIZE(VER_MINORMINOR) << std::endl;

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
