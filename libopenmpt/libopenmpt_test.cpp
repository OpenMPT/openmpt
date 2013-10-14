/*
 * libopenmpt_test.cpp
 * -------------------
 * Purpose: libopenmpt test suite driver
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "BuildSettings.h"

#include "libopenmpt_internal.h"
#include "libopenmpt.hpp"
#include "libopenmpt.h"

#if defined( LIBOPENMPT_BUILD_TEST )

#if defined(WIN32) && defined(UNICODE)
#if defined(__GNUC__)
// mingw64 does only default to special C linkage for "main", but not for "wmain".
extern "C"
#endif
int wmain( int /*wargc*/, wchar_t * /*wargv*/ [] ) {
#else
int main( int /*argc*/, char * /*argv*/ [] ) {
#endif
	try {
		openmpt::run_tests();
	} catch ( const std::exception & e ) {
		std::cerr << "TEST ERROR: exception: " << ( e.what() ? e.what() : "" ) << std::endl;
		return -1;
	} catch ( ... ) {
		std::cerr << "TEST ERROR: unknown exception" << std::endl;
		return -1;
	}
	return 0;
}

#endif // LIBOPENMPT_BUILD_TEST
