/*
 * tagging.cpp
 * -----------
 * Purpose: ID3v2.4 / RIFF tags tagging class (for mp3 / wav / etc. support)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "tagging.h"
#include "../soundlib/Endianness.h"
#include "../common/version.h"

///////////////////////////////////////////////////
// CFileTagging - helper class for writing tags

CFileTagging::CFileTagging()
//--------------------------
{
	encoder = MptVersion::GetOpenMPTVersionStr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// ID3v2.4 Tags

// Convert Integer to Synchsafe Integer (see ID3v2.4 specs)
// Basically, it's a BigEndian integer, but the MSB of all bytes is 0.
// Thus, a 32-bit integer turns into a 28-bit integer.
uint32 CFileTagging::intToSynchsafe(uint32 in)
//--------------------------------------------
{
	uint32 out = 0, steps = 0;
	do
	{
		out |= (in & 0x7F) << steps;
		steps += 8;
	} while(in >>= 7);
	SwapBytesBE(out);
	return out;
}

// Write Tags
void CFileTagging::WriteID3v2Tags(FILE *f)
//----------------------------------------
{
	if(!f) return;
	
	ID3v2Header tHeader;
	UINT fOffset = ftell(f);

	totalID3v2Size = 0;

	// Correct header will be written later (tag size missing)
	tHeader.signature[0] = 'I';
	tHeader.signature[1] = 'D';
	tHeader.signature[2] = '3';
	tHeader.version[0] = 0x04; // Version 2.4.0
	tHeader.version[1] = 0x00; // Dito
	tHeader.flags = 0x00; // No flags
	fwrite(&tHeader, 1, sizeof(tHeader), f);

	// Write TIT2 (Title), TCOM / TPE1 (Composer), TALB (Album), TCON (Genre), TYER / TDRC (Date), WXXX (URL), TENC (Encoder), COMM (Comment)
	WriteID3v2Frame("TIT2", title, f);
	WriteID3v2Frame("TPE1", artist, f);
	WriteID3v2Frame("TCOM", artist, f);
	WriteID3v2Frame("TALB", album, f);
	WriteID3v2Frame("TCON", genre, f);
	//WriteID3v2Frame("TYER", year, f);		// Deprecated
	WriteID3v2Frame("TDRC", year, f);
	WriteID3v2Frame("TBPM", bpm, f);
	WriteID3v2Frame("WXXX", url, f);
	WriteID3v2Frame("TENC", encoder, f);
	WriteID3v2Frame("COMM", comments, f);

	// Write Padding
	for(size_t i = 0; i < ID3v2_PADDING; i++)
	{
		fputc(0, f);
	}
	totalID3v2Size += ID3v2_PADDING;

	// Write correct header (update tag size)
	tHeader.size = intToSynchsafe(totalID3v2Size);
	fseek(f, fOffset, SEEK_SET);
	fwrite(&tHeader, 1, sizeof(tHeader), f);
	fseek(f, totalID3v2Size, SEEK_CUR);

}

// Write a ID3v2 frame
void CFileTagging::WriteID3v2Frame(char cFrameID[4], std::string sFramecontent, FILE *f)
//--------------------------------------------------------------------------------------
{
	if(!cFrameID[0] || sFramecontent.empty() || !f) return;

	if(!memcmp(cFrameID, "COMM", 4))
	{
		// English language for comments - no description following (hence the text ending nullchar(s))
		// For language IDs, see http://en.wikipedia.org/wiki/ISO-639-2
		sFramecontent = "eng" + (ID3v2_TEXTENDING + sFramecontent);
	}
	if(!memcmp(cFrameID, "WXXX", 4))
	{
		// User-defined URL field (we have no description for the URL, so we leave it out)
		sFramecontent = ID3v2_TEXTENDING + sFramecontent;
	}
	sFramecontent = ID3v2_CHARSET + sFramecontent;
	sFramecontent += ID3v2_TEXTENDING;

	ID3v2Frame tFrame;

	memcpy(&tFrame.frameid, cFrameID, 4); // ID
	tFrame.size = intToSynchsafe(sFramecontent.size()); // Text size
	tFrame.flags = 0x0000; // No flags
	fwrite(&tFrame, 1, sizeof(tFrame), f);
	fwrite(sFramecontent.c_str(), 1, sFramecontent.size(), f);

	totalID3v2Size += (sizeof(tFrame) + sFramecontent.size());
}
