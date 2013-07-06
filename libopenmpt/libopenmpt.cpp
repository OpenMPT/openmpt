/*
 * libopenmpt.cpp
 * --------------
 * Purpose: libopenmpt general implementation
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

namespace openmpt {

void run_tests() {
	MptTest::DoTests();
}

} // namespace openmpt

#if defined( LIBOPENMPT_BUILD_DLL ) 
#if defined( _WIN32 )

#include <windows.h>

#ifndef NO_XMPLAY

void xmp_openmpt_on_dll_load();
void xmp_openmpt_on_dll_unload();

#endif // NOXMPLAY

BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved ) {
	switch ( fdwReason ) {
	case DLL_PROCESS_ATTACH:
		#ifndef NO_XMPLAY
			xmp_openmpt_on_dll_load();
		#endif // NOXMPLAY
		break;
	case DLL_PROCESS_DETACH:
		#ifndef NO_XMPLAY
			xmp_openmpt_on_dll_unload();
		#endif // NOXMPLAY
		break;
	}
	return TRUE;
}

#endif // _WIN32
#endif // LIBOPENMPT_BUILD_DLL
