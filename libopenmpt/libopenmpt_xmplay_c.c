/*
 * libopenmpt_xmplay_c.c
 * ---------------------
 * Purpose: libopenmpt xmplay input plugin interface
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "BuildSettings.h"

#ifndef NO_XMPLAY

#include "xmplay/xmpin.h"

XMPIN * XMPIN_GetInterface_cxx( DWORD face, InterfaceProc faceproc );
// Stupid MSVC does not undecorate function name when using linker pragma.
// So, we have to use EXPORTS in the .def file.
/*LIBOPENMPT_XMPLAY_API*/ XMPIN * WINAPI XMPIN_GetInterface( DWORD face, InterfaceProc faceproc ) {
	return XMPIN_GetInterface_cxx( face, faceproc );
}
//#pragma comment(linker, "/EXPORT:XMPIN_GetInterface")

#else // !NO_XMPLAY

#ifdef _MSC_VER
#include <windows.h>
// to prevent linking failure with symbols from .def file ... this is getting complicated
void * XMPIN_GetInterface( DWORD face, void * faceproc ) {
	return NULL;
}
#endif // _MSC_VER

#endif // NO_XMPLAY
