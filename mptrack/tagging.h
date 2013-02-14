/*
 * tagging.h
 * ---------
 * Purpose: ID3v2.4 / RIFF tags tagging class (for mp3 / wav / etc. support)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <string>
#include "Wav.h"

using std::string;

///////////////////////////////////////////////////////////////////////////////////////////////////
// ID3v2.4 Tags

#pragma pack(push, 1)

struct ID3v2Header
{
	uint8 signature[3];
	uint8 version[2];
	uint8 flags;
	uint32 size;
	// Total: 10 bytes
};

struct ID3v2Frame
{
	uint32 frameid;
	uint32 size;
	uint16 flags;
	// Total: 10 bytes
};

#pragma pack(pop)

// we will add some padding bytes to our id3v2 tag (extending tags will be easier this way)
#define ID3v2_PADDING 512

// charset... choose text ending accordingly.
// $00 = ISO-8859-1. Terminated with $00.
// $01 = UTF-16 with BOM. Terminated with $00 00.
// $02 = UTF-16BE without BOM. Terminated with $00 00.
// $03 = UTF-8. Terminated with $00.
#ifdef UNICODE
#define ID3v2_CHARSET '\3'
#define ID3v2_TEXTENDING '\0'
#else
#define ID3v2_CHARSET '\0'
#define ID3v2_TEXTENDING '\0'
#endif

//================
class CFileTagging
//================
{
public:
	// Write Tags
	void WriteID3v2Tags(FILE *f);
	void WriteWaveTags(WAVEDATAHEADER *wdh, WAVEFILEHEADER *wfh, FILE *f);

	// Tag data
	string title, artist, album, year, comments, genre, url, encoder, bpm;

	CFileTagging();


private:
	// Convert Integer to Synchsafe Integer (see ID3v2.4 specs)
	uint32 intToSynchsafe(uint32 in);
	// Write a frame
	void WriteID3v2Frame(char cFrameID[4], string sFramecontent, FILE *f);
	// Size of our tag
	uint32 totalID3v2Size;
};

