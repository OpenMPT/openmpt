/*
 * Message.cpp
 * -----------
 * Purpose: Various functions for processing song messages (allocating, reading from file...)
 * Notes  : Those functions should offer a rather high level of abstraction compared to
 *          previous ways of reading the song messages. There are still many things to do,
 *          though. Future versions of ReadMessage() could e.g. offer charset conversion
 *          and the code is not yet ready for unicode.
 *          Some functions for preparing the message text to be written to a file would
 *          also be handy.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
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
	try
	{
		if(length == Util::MaxValueOfType(length))
		{
			return false;
		}
		length++;	// + 1 for trailing null
		m_lpszSongComments = new char[length];
		memset(m_lpszSongComments, 0, length);
		return true;
	} catch(MPTMemoryException)
	{
		m_lpszSongComments = nullptr;
		return false;
	}
}


// Free previously allocated song message memory
void CSoundFile::FreeMessage()
//----------------------------
{
	delete[] m_lpszSongComments;
	m_lpszSongComments = nullptr;
}


// Read song message from a mapped file.
// [in]  data: pointer to the data in memory that is going to be read
// [in]  length: number of characters that should be read, not including a possible trailing null terminator (it is automatically appended).
// [in]  lineEnding: line ending formatting of the text in memory.
// [in]  pTextConverter: Pointer to a callback function which can be used to pre-process the read characters, if necessary (nullptr otherwise).
// [out] returns true on success.
bool CSoundFile::ReadMessage(const BYTE *data, size_t length, enmLineEndings lineEnding, void (*pTextConverter)(char &))
//----------------------------------------------------------------------------------------------------------------------
{
	while(length != 0 && data[length - 1] == '\0')
	{
		// Ignore trailing null character.
		length--;
	}

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

	size_t finalLength = 0;
	// calculate the final amount of characters to be allocated.
	for(size_t i = 0; i < length; i++)
	{
		c = data[i];
		if(pTextConverter != nullptr)
			pTextConverter(c);

		if(c != '\n' || lineEnding != leCRLF)
			finalLength++;
	}

	if(!AllocateMessage(finalLength))
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
				m_lpszSongComments[cpos] = INTERNAL_LINEENDING;
			else
				m_lpszSongComments[cpos] = ' ';
			if(lineEnding == leCRLF) i++;	// skip the LF
			break;
		case '\n':
			if(lineEnding != leCR && lineEnding != leCRLF)
				m_lpszSongComments[cpos] = INTERNAL_LINEENDING;
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
// [in]  lineEndingLength: The padding space between two fixed lines. (there could for example be a null char after every line)
// [in]  pTextConverter: Pointer to a callback function which can be used to pre-process the read characters, if necessary (nullptr otherwise).
// [out] returns true on success.
bool CSoundFile::ReadFixedLineLengthMessage(const BYTE *data, const size_t length, const size_t lineLength, const size_t lineEndingLength, void (*pTextConverter)(char &))
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	if(lineLength == 0)
		return false;

	const size_t numLines = (length / (lineLength + lineEndingLength));
	const size_t finalLength = numLines * (lineLength + 1);
	if(!AllocateMessage(finalLength))
		return false;

	for(size_t line = 0, fpos = 0, cpos = 0; line < numLines; line++, fpos += (lineLength + lineEndingLength), cpos += (lineLength + 1))
	{
		memcpy(m_lpszSongComments + cpos, data + fpos, std::min(lineLength, length - fpos));
		m_lpszSongComments[cpos + lineLength] = INTERNAL_LINEENDING;

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


// Retrieve song message.
// [in]  lineEnding: line ending formatting of the text in memory.
// [in]  pTextConverter: Pointer to a callback function which can be used to post-process the written characters, if necessary (nullptr otherwise).
// [out] returns formatted song message.
mpt::String CSoundFile::GetSongMessage(const enmLineEndings lineEnding, void (*pTextConverter)(char &)) const
//-------------------------------------------------------------------------------------------------------
{
	mpt::String comments;

	if(m_lpszSongComments == nullptr)
	{
		return comments;
	}

	const size_t len = strlen(m_lpszSongComments);
	comments.reserve(len);

	for(size_t i = 0; i < len; i++)
	{
		if(m_lpszSongComments[i] == INTERNAL_LINEENDING)
		{
			switch(lineEnding)
			{
			case leCR:
			default:
				comments.append(1, '\r');
				break;
			case leCRLF:
				comments.append(1, 'r');
				// Intentional fall-through
			case leLF:
				comments.append(1, '\n');
				break;
			}
		} else
		{
			char c = m_lpszSongComments[i];
			// Pre-process text
			if(pTextConverter != nullptr) pTextConverter(c);
			comments.append(1, c);
		}
	}
	return comments;
}