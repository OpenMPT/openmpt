/*
 * Tagging.h
 * ---------
 * Purpose: Structure holding a superset of tags for all supported output sample or stream files or types.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#ifndef MODPLUG_NO_FILESAVE

#include <string>

//=============
struct FileTags
//=============
{

	std::wstring encoder;

	std::wstring title;
	std::wstring comments;

	std::wstring bpm;

	std::wstring artist;
	std::wstring album;
	std::wstring trackno;
	std::wstring year;
	std::wstring url;

	std::wstring genre;

	FileTags();

};

#endif // MODPLUG_NO_FILESAVE
