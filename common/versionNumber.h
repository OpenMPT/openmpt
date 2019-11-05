/*
 * versionNumber.h
 * ---------------
 * Purpose: OpenMPT version handling.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

OPENMPT_NAMESPACE_BEGIN

#define VER_HELPER_STRINGIZE(x) #x
#define VER_STRINGIZE(x)        VER_HELPER_STRINGIZE(x)

#define MPT_MAKE_VERSION_NUMERIC_HELPER(prefix,v0,v1,v2,v3) Version( prefix ## v0 , prefix ## v1 , prefix ## v2 , prefix ## v3 )
#define MPT_MAKE_VERSION_NUMERIC(v0,v1,v2,v3) MPT_MAKE_VERSION_NUMERIC_HELPER(0x, v0, v1, v2, v3)

// Version definitions. The only thing that needs to be changed when changing version number.
#define VER_MAJORMAJOR  1
#define VER_MAJOR      29
#define VER_MINOR      00
#define VER_MINORMINOR 35

// Numerical value of the version.
#define MPT_VERSION_CURRENT MPT_MAKE_VERSION_NUMERIC(VER_MAJORMAJOR,VER_MAJOR,VER_MINOR,VER_MINORMINOR)

OPENMPT_NAMESPACE_END
