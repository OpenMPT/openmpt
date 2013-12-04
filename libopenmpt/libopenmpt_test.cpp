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

#include <locale>

#include <clocale>

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

		// run test with "C" / classic() locale
		openmpt::run_tests();

		// try setting the C locale to the user locale
		setlocale( LC_ALL, "" );
		
		// run all tests again with a set C locale
		openmpt::run_tests();
		
		// try to set the C and C++ locales to the user locale
		try {
			std::locale::global( std::locale( "" ) );
		} catch ( ... ) {
			// Setting c++ global locale does not work.
			// This is no problem for libopenmpt, just continue.
		}
		
		// and now, run all tests once again
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
