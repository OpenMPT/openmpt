/*
 * Message.cpp
 * -----------
 * Purpose: Various functions concerning song messages (allocating, reading from file...)
 * Notes  : Those functions should offer a rather high level of abstraction compared to
 *          previous ways of reading the song messages. There are still many things to do,
 *          though. Future versions of ReadMessage() could f.e. offer charset conversion
 *          and the code is not yet ready for unicode.
 *          Some functions for preparing the message text to be written to a file would
 *          also be handy.
 * Authors: OpenMPT Devs
 *
 */

#include "stdafx.h"
#include "Sndfile.h"


// Allocate memory for song message.
// [in]  length: text length in characters, without possible trailing null terminator.
// [out] returns true on success.
bool CSoundFile::AllocateMessage(size_t length)
//---------------------------------------------
{
	FreeMessage();
	m_lpszSongComments = new char[length + 1];	// + 1 for trailing null
	if(m_lpszSongComments == nullptr)
	{
		return false;
	} else
	{
		memset(m_lpszSongComments, 0, length + 1);
		return true;
	}
}


// Free previously allocated song message memory
void CSoundFile::FreeMessage()
//----------------------------
{
	if(m_lpszSongComments)
	{
		delete[] m_lpszSongComments;
		m_lpszSongComments = nullptr;
	}
}


// Read song message from a mapped file.
// [in]  data: pointer to the data in memory that is going to be read
// [in]  length: number of characters that should be read, not including a possible trailing null terminator (it is automatically appended).
// [in]  lineEnding: line ending formatting of the text in memory.
// [in]  pTextConverter: Pointer to a callback function which can be used to pre-process the read characters, if necessary (nullptr otherwise).
// [out] returns true on success.
bool CSoundFile::ReadMessage(const BYTE *data, const size_t length, enmLineEndings lineEnding, void (*pTextConverter)(char &))
//----------------------------------------------------------------------------------------------------------------------------
{
	char c;

	// Simple line-ending detection algorithm. VERY simple.
	if(lineEnding == leAutodetect)
	{
		char cprev = 0;
		size_t nCR = 0, nLF = 0, nCRLF = 0;
		// find CRs, LFs and CRLFs
		for(size_t i = 0; i < length; i++)
		{
			c = data[i];
			if(pTextConverter != nullptr)
				pTextConverter(c);

			if(c == '\r') nCR++;
			else if(c == '\n') nLF++;

			if(i && cprev == '\r' && c == '\n') nCRLF++;
			cprev = c;
		}
		// evaluate findings
		if(nCR == nLF && nCR == nCRLF)
			lineEnding = leCRLF;
		else if(nCR && !nLF)
			lineEnding = leCR;
		else if(!nCR && nLF)
			lineEnding = leLF;
		else
			lineEnding = leMixed;
	}

	size_t final_length = 0;
	// calculate the final amount of characters to be allocated.
	for(size_t i = 0; i < length; i++)
	{
		c = data[i];
		if(pTextConverter != nullptr)
			pTextConverter(c);

		switch(c)
		{
		case '\r':
			if(lineEnding != leLF) final_length++;
			break;
		case '\n':
			if(lineEnding != leCR && lineEnding != leCRLF) final_length++;
			break;
		default:
			final_length++;
			break;
		}
	}

	if(!AllocateMessage(final_length))
		return false;

	size_t cpos = 0;
	for(size_t i = 0; i < length; i++, cpos++)
	{
		c = data[i];
		if(pTextConverter != nullptr)
			pTextConverter(c);

		switch(c)
		{
		case '\r':
			if(lineEnding != leLF)
				m_lpszSongComments[cpos] = '\r';
			else
				m_lpszSongComments[cpos] = ' ';
			if(lineEnding == leCRLF) i++;	// skip the LF
			break;
		case '\n':
			if(lineEnding != leCR && lineEnding != leCRLF)
				m_lpszSongComments[cpos] = '\r';
			else
				m_lpszSongComments[cpos] = ' ';
			break;
		case '\0':
			m_lpszSongComments[cpos] = ' ';
			break;
		default:
			m_lpszSongComments[cpos] = c;
			break;
		}
	}

	return true;
}


// Read comments with fixed line length from a mapped file.
// [in]  data: pointer to the data in memory that is going to be read
// [in]  length: number of characters that should be read, not including a possible trailing null terminator (it is automatically appended).
// [in]  lineLength: The fixed length of a line.
// [in]  lineEndingLength: The padding space between two fikxed lines. (there could for example be a null char after every line)
// [in]  pTextConverter: Pointer to a callback function which can be used to pre-process the read characters, if necessary (nullptr otherwise).
// [out] returns true on success.
bool CSoundFile::ReadFixedLineLengthMessage(const BYTE *data, const size_t length, const size_t lineLength, const size_t lineEndingLength, void (*pTextConverter)(char &))
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	if(lineLength == 0)
		return false;

	const size_t num_lines = (length / (lineLength + lineEndingLength));
	const size_t final_length = num_lines * (lineLength + 1);
	if(!AllocateMessage(final_length))
		return false;

	for(size_t line = 0, fpos = 0, cpos = 0; line < num_lines; line++, fpos += (lineLength + lineEndingLength), cpos += (lineLength + 1))
	{
		memcpy(m_lpszSongComments + cpos, data + fpos, min(lineLength, length - fpos));
		m_lpszSongComments[cpos + lineLength] = '\r';

		// fix weird chars
		for(size_t lpos = 0; lpos < lineLength; lpos++)
		{
			// Pre-process text
			if(pTextConverter != nullptr) pTextConverter(m_lpszSongComments[cpos + lpos]);

			switch(m_lpszSongComments[cpos + lpos])
			{
			case '\0':
			case '\n':
			case '\r':
				m_lpszSongComments[cpos + lpos] = ' ';
				break;
			}
			
		}
	}
	return true;
}


// OLD and unused function. Do we still need it?
UINT CSoundFile::GetSongMessage(LPSTR s, UINT len, UINT linesize)
//---------------------------------------------------------------
{
	LPCSTR p = m_lpszSongComments;
	if (!p) return 0;
	UINT i = 2, ln=0;
	if ((len) && (s)) s[0] = '\x0D';
	if ((len > 1) && (s)) s[1] = '\x0A';
	while ((*p)	&& (i+2 < len))
	{
		BYTE c = (BYTE)*p++;
		if ((c == 0x0D) || ((c == ' ') && (ln >= linesize)))
		{ if (s) { s[i++] = '\x0D'; s[i++] = '\x0A'; } else i+= 2; ln=0; }
		else
			if (c >= 0x20) { if (s) s[i++] = c; else i++; ln++; }
	}
	if (s) s[i] = 0;
	return i;
}


// OLD and unused function. Do we still need it?
UINT CSoundFile::GetRawSongMessage(LPSTR s, UINT len, UINT linesize)
//------------------------------------------------------------------
{
	LPCSTR p = m_lpszSongComments;
	if (!p) return 0;
	UINT i = 0, ln=0;
	while ((*p)	&& (i < len-1))
	{
		BYTE c = (BYTE)*p++;
		if ((c == 0x0D)	|| (c == 0x0A))
		{
			if (ln) 
			{
				while (ln < linesize) { if (s) s[i] = ' '; i++; ln++; }
				ln = 0;
			}
		} else
			if ((c == ' ') && (!ln))
			{
				UINT k=0;
				while ((p[k]) && (p[k] >= ' '))	k++;
				if (k <= linesize)
				{
					if (s) s[i] = ' ';
					i++;
					ln++;
				}
			} else
			{
				if (s) s[i] = c;
				i++;
				ln++;
				if (ln == linesize) ln = 0;
			}
	}
	if (ln)
	{
		while ((ln < linesize) && (i < len))
		{
			if (s) s[i] = ' ';
			i++;
			ln++;
		}
	}
	if (s) s[i] = 0;
	return i;
}
