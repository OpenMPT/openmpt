/*
 * Purpose: File tagging (ID3v2, RIFF + more in the future)
 * Authors: OpenMPT Devs
*/

#include "stdafx.h"
#include "mptrack.h"
#include "tagging.h"
#include "version.h"
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
uint32 CFileTagging::intToSynchsafe(uint32 iIn)
//---------------------------------------------
{
	uint32 iOut = 0, iSteps = 0;
	do
	{
		iOut |= (iIn & 0x7F) << iSteps;
		iSteps += 8;
	} while(iIn >>= 7);
	return BigEndian(iOut);
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

	// Write TIT2 (Title), TCOM / TPE1 (Composer), TALB (Album), TCON (Genre), TYER (Date), WXXX (URL), TENC (Encoder), COMM (Comment)
	WriteID3v2Frame("TIT2", title, f);
	WriteID3v2Frame("TPE1", artist, f);
	WriteID3v2Frame("TCOM", artist, f);
	WriteID3v2Frame("TALB", album, f);
	WriteID3v2Frame("TCON", genre, f);
	WriteID3v2Frame("TYER", year, f);
	WriteID3v2Frame("WXXX", url, f);
	WriteID3v2Frame("TENC", encoder, f);
	WriteID3v2Frame("COMM", comments, f);

	// Write Padding
	for(UINT i = 0; i < ID3v2_PADDING; i++)
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
	CHAR s[256];
	DWORD info_ofs, end_ofs;
	const DWORD zero = 0;

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
	// ICMT
	if (!comments.empty())
	{
		chunk.id_data = IFFID_ICMT;
		chunk.length = strlen(comments.c_str()) + 1;
		fwrite(&chunk, 1, sizeof(chunk), f);
		fwrite(comments.c_str(), 1, chunk.length, f);
		list.filesize += chunk.length + sizeof(chunk);
		if (chunk.length & 1)
		{
			fwrite(&zero, 1, 1, f);
			list.filesize++;
		}
	}
	for (UINT iCmt=0; iCmt<=6; iCmt++)
	{
		s[0] = 0;
		switch(iCmt)
		{
		// INAM
		case 0: memcpy(s, title.c_str(), 255); s[255] = 0; chunk.id_data = IFFID_INAM; break;
		// IART
		case 1: memcpy(s, artist.c_str(), 255); s[255] = 0; chunk.id_data = IFFID_IART; break;
		// IPRD
		case 2: memcpy(s, album.c_str(), 255); s[255] = 0; chunk.id_data = IFFID_IPRD; break;
		// ICOP
		case 3: memcpy(s, url.c_str(), 255); s[255] = 0; chunk.id_data = IFFID_ICOP; break;
		// IGNR
		case 4: memcpy(s, genre.c_str(), 255); s[255] = 0; chunk.id_data = IFFID_IGNR; break;
		// ISFT
		case 5: memcpy(s, encoder.c_str(), 255); s[255] = 0; chunk.id_data = IFFID_ISFT; break;
		// ICRD
		case 6: memcpy(s, year.c_str(), 4); s[4] = 0; strcat(s, "-01-01"); if (s[0] <= '0') s[0] = 0; chunk.id_data = IFFID_ICRD; break;
		}
		int l = strlen(s);
		while ((l > 0) && (s[l-1] == ' ')) s[--l] = 0;
		if (s[0])
		{
			chunk.length = strlen(s)+1;
			fwrite(&chunk, 1, sizeof(chunk), f);
			fwrite(s, 1, chunk.length, f);
			list.filesize += chunk.length + sizeof(chunk);
			if (chunk.length & 1)
			{
				fwrite(&zero, 1, 1, f);
				list.filesize++;
			}
		}
	}
	// Update INFO size
	end_ofs = ftell(f);
	fseek(f, info_ofs, SEEK_SET);
	fwrite(&list, 1, sizeof(list), f);
	fseek(f, end_ofs, SEEK_SET);
	wfh->filesize += list.filesize + 8;
}
