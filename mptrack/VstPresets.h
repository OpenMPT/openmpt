/*
 * VstPresets.h
 * ------------
 * Purpose: VST plugin preset / bank handling
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <iosfwd>

OPENMPT_NAMESPACE_BEGIN

class FileReader;
class IMixPlugin;

class VSTPresets
{
public:
	enum ErrorCode
	{
		noError,
		invalidFile,
		wrongPlugin,
		wrongParameters,
		outOfMemory,
	};

#ifndef NO_PLUGINS
	static ErrorCode LoadFile(FileReader &file, IMixPlugin &plugin);
	static bool SaveFile(std::ostream &, IMixPlugin &plugin, bool bank);
	static const char *GetErrorMessage(ErrorCode code);

protected:
	static void SaveProgram(std::ostream &f, IMixPlugin &plugin);

#else
	static ErrorCode LoadFile(FileReader &, IMixPlugin &) { return invalidFile; }
	static bool SaveFile(std::ostream &, IMixPlugin &, bool) { return false; }
	static const char *GetErrorMessage(ErrorCode) { return "OpenMPT has been built without VST support"; }
#endif // NO_PLUGINS
};

OPENMPT_NAMESPACE_END
