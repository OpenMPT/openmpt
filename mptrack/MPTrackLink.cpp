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


#if MPT_COMPILER_MSVC

#pragma comment(lib, "delayimp.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "htmlhelp.lib")

#pragma comment(lib, "dmoguids.lib")
#pragma comment(lib, "strmiids.lib")

#ifndef NO_DSOUND
#pragma comment(lib, "dsound.lib")
#endif
#pragma comment(lib, "winmm.lib")

#pragma comment(lib, "ksuser.lib")
#if MPT_MSVC_BEFORE(2010,0) && defined(_M_X64)
// VS2008 x64 SDK has no ksguid.lib. Work-around in include\portaudio\src\os\win\pa_win_wdmks_utils.c .
#else
#pragma comment(lib, "ksguid.lib")
#endif

#pragma comment(lib, "msacm32.lib")

#endif // MPT_COMPILER_MSVC


OPENMPT_NAMESPACE_END
