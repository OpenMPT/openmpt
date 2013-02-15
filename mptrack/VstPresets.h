/*
 * VstPresets.h
 * ------------
 * Purpose: VST plugin preset / bank handling
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

class VSTPresets
{
public:
	enum ErrorCode
	{
		noError,
		invalidFile,
		wrongPlugin,
		outdatedPlugin,
		wrongParameters,
	};

#ifndef NO_VST
	static ErrorCode LoadFile(FileReader &file, CVstPlugin &plugin);
	static bool SaveFile(const char *filename, CVstPlugin &plugin, bool bank);

protected:
	static void SaveProgram(FILE *f, CVstPlugin &plugin);

	template<typename T>
	static void Write(const T &v, FILE *f)
	{
		fwrite(&v, sizeof(T), 1, f);
	}

	static void WriteBE(uint32 v, FILE *f);
	static void WriteBE(float v, FILE *f);

#else
	static ErrorCode LoadFile(FileReader &, CVstPlugin &) { return invalidFile; }
	static bool SaveFile(const char *, CVstPlugin &, bool) { return false; }
#endif // NO_VST
};
