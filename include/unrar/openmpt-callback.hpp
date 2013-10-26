/*
 * openmpt-callback.hpp
 * --------------------
 * Purpose: Wrapper functions for FileReader, so that it can be used easily in UnRAR.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

class FileReader;

struct RARFileCallbacks
{
	typedef size_t (CALLBACK *RAR_READRAW) (FileReader &file, char *data, size_t size);
	typedef bool (CALLBACK *RAR_SEEK) (FileReader &file, size_t offset);
	typedef size_t (CALLBACK *RAR_GETPOSITION) (FileReader &file);
	typedef size_t (CALLBACK *RAR_GETLENGTH) (FileReader &file);

	RAR_READRAW ReadRaw;
	RAR_SEEK Seek;
	RAR_GETPOSITION GetPosition;
	RAR_GETLENGTH GetLength;

	FileReader &file;

	RARFileCallbacks(RAR_READRAW readFunc, RAR_SEEK seekFunc, RAR_GETPOSITION getPosFunc, RAR_GETLENGTH getLenFunc, FileReader &f)
		: ReadRaw(readFunc), Seek(seekFunc), GetPosition(getPosFunc), GetLength(getLenFunc), file(f) { }
};
