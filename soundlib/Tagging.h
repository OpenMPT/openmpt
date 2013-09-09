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

	// Tag data
	std::wstring title, artist, album, year, comments, genre, url, encoder, bpm, trackno;

	FileTags();

};

#endif // MODPLUG_NO_FILESAVE
