/*
 * openmpt-callback.hpp
 * --------------------
 * Purpose: Wrapper functions for file reading.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

struct RARFileCallbacks
{
	typedef size_t (CALLBACK *RAR_READRAW) (void *file, char *data, size_t size);
	typedef bool (CALLBACK *RAR_SEEK) (void *file, size_t offset);
	typedef size_t (CALLBACK *RAR_GETPOSITION) (void *file);
	typedef size_t (CALLBACK *RAR_GETLENGTH) (void *file);

	RAR_READRAW ReadRaw;
	RAR_SEEK Seek;
	RAR_GETPOSITION GetPosition;
	RAR_GETLENGTH GetLength;

	void *file;

	RARFileCallbacks(RAR_READRAW readFunc, RAR_SEEK seekFunc, RAR_GETPOSITION getPosFunc, RAR_GETLENGTH getLenFunc, void *f)
		: ReadRaw(readFunc), Seek(seekFunc), GetPosition(getPosFunc), GetLength(getLenFunc), file(f) { }
};
