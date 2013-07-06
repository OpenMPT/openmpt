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

namespace MptTest {
	void DoTests();
} // namespace MptTest

#if defined( LIBOPENMPT_BUILD_TEST ) && ( defined( _MSC_VER ) || defined( LIBOPENMPT_TEST_MAIN ) )

int main( int argc, char * argv [] ) {
	try {
		MptTest::DoTests();
	} catch ( const std::exception & e ) {
		std::cerr << "TEST ERROR: exception: " << ( e.what() ? e.what() : "" ) << std::endl;
		return 1;
	} catch ( ... ) {
		std::cerr << "TEST ERROR: unknown exception" << std::endl;
		return 1;
	}
	return 0;
}

#endif // LIBOPENMPT_BUILD_TEST
