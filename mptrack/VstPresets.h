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
class CVstPlugin;

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

#ifndef NO_VST
	static ErrorCode LoadFile(FileReader &file, CVstPlugin &plugin);
	static bool SaveFile(std::ostream &, CVstPlugin &plugin, bool bank);
	static const char *GetErrorMessage(ErrorCode code);

protected:
	static void SaveProgram(std::ostream &f, CVstPlugin &plugin);

#else
	static ErrorCode LoadFile(FileReader &, CVstPlugin &) { return invalidFile; }
	static bool SaveFile(std::ostream &, CVstPlugin &, bool) { return false; }
	static const char *GetErrorMessage(ErrorCode) { return "OpenMPT has been built without VST support"; }
#endif // NO_VST
};

OPENMPT_NAMESPACE_END
