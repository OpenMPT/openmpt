/*
 * openmpt.hpp
 * -----------
 * Purpose: Various definitions, so that UnRAR project settings are identical to OpenMPT's settings when including rar.hpp
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#undef Log
#undef STRICT
#define UNRAR
#define SILENT
#define RARDLL
#define RAR_NOCRYPT
#define NOVOLUME
#define NOMINMAX	// For windows.h
