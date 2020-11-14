/*
 * MPTrackLink.cpp
 * ---------------
 * Purpose: Consolidated linking against MSVC/Windows libraries.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_BUILD_MSVC)
#if MPT_COMPILER_MSVC || MPT_COMPILER_CLANG

#pragma comment(lib, "delayimp.lib")

#pragma comment(lib, "version.lib")
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "htmlhelp.lib")
#pragma comment(lib, "uxtheme.lib")

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "ncrypt.lib")

#pragma comment(lib, "gdiplus.lib")

#pragma comment(lib, "dmoguids.lib")
#pragma comment(lib, "strmiids.lib")

#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "winmm.lib")

#pragma comment(lib, "ksuser.lib")

#ifdef MPT_WITH_MEDIAFOUNDATION
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib") // static lib
#pragma comment(lib, "propsys.lib")
#endif

// work-around VS2019 16.8.1 bug on ARM and ARM64
#if MPT_COMPILER_MSVC
#if defined(_M_ARM) || defined(_M_ARM64)
#pragma comment(lib, "Synchronization.lib")
#endif
#endif

#if MPT_COMPILER_MSVC
#pragma comment( linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df'\"" )
#endif // MPT_COMPILER_MSVC

#endif // MPT_COMPILER_MSVC || MPT_COMPILER_CLANG
#endif // MPT_BUILD_MSVC


OPENMPT_NAMESPACE_END
