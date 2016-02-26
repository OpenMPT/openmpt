/*
 * StdAfx.h
 * --------
 * Purpose: Include file for standard system include files, or project specific include files that are used frequently, but are changed infrequently. Also includes the global build settings from BuildSettings.h.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


// has to be first
#include "BuildSettings.h"


#if defined(MODPLUG_TRACKER) && !defined(MPT_BUILD_WINESUPPORT)

#if MPT_OS_WINDOWS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>			// MFC support for Windows Common Controls
#include <afxcview.h>
#include <afxdlgs.h>
#include <afxole.h>

#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <mmsystem.h>

#endif // MPT_OS_WINDOWS

#endif // MODPLUG_TRACKER && !MPT_BUILD_WINESUPPORT


#if MPT_COMPILER_MSVC
#include <intrin.h>
#endif


// this will be available everywhere

#include "../common/typedefs.h"
// <memory>
// <new>
// <cstdint>
// <stdint.h>

#include "../common/mptTypeTraits.h"
// <type_traits> // if available

#include "../common/mptString.h"
// <algorithm>
// <limits>
// <string>
// <type_traits>
// <cstring>

#include "../common/mptStringFormat.h"

#include "../common/mptPathString.h"

#include "../common/Logging.h"

#include "../common/misc_util.h"
// <algorithm>
// <limits>
// <string>
// <type_traits>
// <vector>
// <cmath>
// <cstring>
// <time.h>

// for std::abs
#include <cstdlib>
#include <stdlib.h>

#if defined(MPT_ENABLE_FILEIO_STDIO)
// for FILE* definition (which cannot be forward-declared in a portable way)
#include <cstdio>
#include <stdio.h>
#endif


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
