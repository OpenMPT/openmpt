/*
 * tagging.cpp
 * -----------
 * Purpose: ID3v2.4 / RIFF tags tagging class (for mp3 / wav / etc. support)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "tagging.h"
#include "../common/version.h"
#include "sndfile.h"

///////////////////////////////////////////////////
// CFileTagging - helper class for writing tags

CFileTagging::CFileTagging()
//--------------------------
{
	encoder = "OpenMPT " MPT_VERSION_STR;
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
	
	TAGID3v2HEADER tHeader;
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
void CFileTagging::WriteID3v2Frame(char cFrameID[4], string sFramecontent, FILE *f)
//---------------------------------------------------------------------------------
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

	TAGID3v2FRAME tFrame;

	memcpy(&tFrame.frameid, cFrameID, 4); // ID
	tFrame.size = intToSynchsafe(sFramecontent.size()); // Text size
	tFrame.flags = 0x0000; // No flags
	fwrite(&tFrame, 1, sizeof(tFrame), f);
	fwrite(sFramecontent.c_str(), 1, sFramecontent.size(), f);

	totalID3v2Size += (sizeof(tFrame) + sFramecontent.size());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// RIFF (WAVE) Tags

// Write RIFF tags
void CFileTagging::WriteWaveTags(WAVEDATAHEADER *wdh, WAVEFILEHEADER *wfh, FILE *f)
//---------------------------------------------------------------------------------
{
	if(!f || !wdh || !wfh) return;
	
	WAVEFILEHEADER list;
	WAVEDATAHEADER chunk;
	DWORD info_ofs, end_ofs;
	const uint8 zero = 0;

	struct
	{
		DWORD id;
		string *data;
	} chunks[] =
	{
		{ IFFID_ICMT, &comments },
		{ IFFID_INAM, &title },
		{ IFFID_IART, &artist },
		{ IFFID_IPRD, &album },
		{ IFFID_ICOP, &url },
		{ IFFID_IGNR, &genre },
		{ IFFID_ISFT, &encoder },
		{ IFFID_ICRD, &year },
	};

	info_ofs = ftell(f);
	if (info_ofs & 1)
	{
		wdh->length++;
		fwrite(&zero, 1, 1, f);
		info_ofs++;
	}
	list.id_RIFF = IFFID_LIST;
	list.id_WAVE = IFFID_INFO;
	list.filesize = 4;
	fwrite(&list, 1, sizeof(list), f);

	for(size_t iCmt = 0; iCmt < CountOf(chunks); iCmt++)
	{
		if(chunks[iCmt].data->empty())
		{
			continue;
		}

		string data = *chunks[iCmt].data;
		// Special case: Expand year to full date
		if(chunks[iCmt].id == IFFID_ICRD)
		{
			data += "-01-01";
		}

		chunk.id_data = chunks[iCmt].id;
		chunk.length = data.length() + 1;

		fwrite(&chunk, 1, sizeof(chunk), f);
		fwrite(data.c_str(), 1, chunk.length, f);
		list.filesize += chunk.length + sizeof(chunk);

		// Chunks must be even-sized
		if(chunk.length & 1)
		{
			fwrite(&zero, 1, 1, f);
			list.filesize++;
		}
	}
	// Update INFO size
	end_ofs = ftell(f);
	fseek(f, info_ofs, SEEK_SET);
	fwrite(&list, 1, sizeof(list), f);
	fseek(f, end_ofs, SEEK_SET);
	wfh->filesize += list.filesize + 8;
}
