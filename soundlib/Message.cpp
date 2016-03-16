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
#include "Message.h"
#include "../common/FileReader.h"

OPENMPT_NAMESPACE_BEGIN

// Read song message from a mapped file.
// [in]  data: pointer to the data in memory that is going to be read
// [in]  length: number of characters that should be read, not including a possible trailing null terminator (it is automatically appended).
// [in]  lineEnding: line ending formatting of the text in memory.
// [out] returns true on success.
bool SongMessage::Read(const mpt::byte *data, size_t length, LineEnding lineEnding)
//---------------------------------------------------------------------------------
{
	const char *str = mpt::byte_cast<const char *>(data);
	while(length != 0 && str[length - 1] == '\0')
	{
		// Ignore trailing null character.
		length--;
	}

	// Simple line-ending detection algorithm. VERY simple.
	if(lineEnding == leAutodetect)
	{
		char cprev = 0;
		size_t nCR = 0, nLF = 0, nCRLF = 0;
		// find CRs, LFs and CRLFs
		for(size_t i = 0; i < length; i++)
		{
			char c = str[i];

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
		char c = str[i];

		if(c != '\n' || lineEnding != leCRLF)
			finalLength++;
	}

	resize(finalLength);

	size_t cpos = 0;
	for(size_t i = 0; i < length; i++, cpos++)
	{
		char c = str[i];

		switch(c)
		{
		case '\r':
			if(lineEnding != leLF)
				at(cpos) = InternalLineEnding;
			else
				at(cpos) = ' ';
			if(lineEnding == leCRLF) i++;	// skip the LF
			break;
		case '\n':
			if(lineEnding != leCR && lineEnding != leCRLF)
				at(cpos) = InternalLineEnding;
			else
				at(cpos) = ' ';
			break;
		case '\0':
			at(cpos) = ' ';
			break;
		default:
			at(cpos) = c;
			break;
		}
	}

	return true;
}


bool SongMessage::Read(FileReader &file, const size_t length, LineEnding lineEnding)
//----------------------------------------------------------------------------------
{
	FileReader::off_t readLength = std::min(static_cast<FileReader::off_t>(length), file.BytesLeft());
	FileReader::PinnedRawDataView fileView = file.ReadPinnedRawDataView(readLength);
	bool success = Read(fileView.data(), fileView.size(), lineEnding);
	return success;
}


// Read comments with fixed line length from a mapped file.
// [in]  data: pointer to the data in memory that is going to be read
// [in]  length: number of characters that should be read, not including a possible trailing null terminator (it is automatically appended).
// [in]  lineLength: The fixed length of a line.
// [in]  lineEndingLength: The padding space between two fixed lines. (there could for example be a null char after every line)
// [out] returns true on success.
bool SongMessage::ReadFixedLineLength(const mpt::byte *data, const size_t length, const size_t lineLength, const size_t lineEndingLength)
//---------------------------------------------------------------------------------------------------------------------------------------
{
	const char *str = mpt::byte_cast<const char *>(data);
	if(lineLength == 0)
		return false;

	const size_t numLines = (length / (lineLength + lineEndingLength));
	const size_t finalLength = numLines * (lineLength + 1);
	clear();
	reserve(finalLength);

	for(size_t line = 0, fpos = 0, cpos = 0; line < numLines; line++, fpos += (lineLength + lineEndingLength), cpos += (lineLength + 1))
	{
		append(str + fpos, std::min(lineLength, length - fpos));
		append(1, InternalLineEnding);

		// fix weird chars
		for(size_t lpos = 0; lpos < lineLength; lpos++)
		{
			switch(at(cpos + lpos))
			{
			case '\0':
			case '\n':
			case '\r':
				at(cpos + lpos) = ' ';
				break;
			}

		}
	}
	return true;
}


bool SongMessage::ReadFixedLineLength(FileReader &file, const size_t length, const size_t lineLength, const size_t lineEndingLength)
//----------------------------------------------------------------------------------------------------------------------------------
{
	FileReader::off_t readLength = std::min(static_cast<FileReader::off_t>(length), file.BytesLeft());
	FileReader::PinnedRawDataView fileView = file.ReadPinnedRawDataView(readLength);
	bool success = ReadFixedLineLength(fileView.data(), fileView.size(), lineLength, lineEndingLength);
	return success;
}


// Retrieve song message.
// [in]  lineEnding: line ending formatting of the text in memory.
// [out] returns formatted song message.
std::string SongMessage::GetFormatted(const LineEnding lineEnding) const
//----------------------------------------------------------------------
{
	std::string comments;

	if(empty())
	{
		return comments;
	}

	const size_t len = length();
	comments.resize(len);

	size_t writePos = 0;
	for(size_t i = 0; i < len; i++)
	{
		if(at(i) == InternalLineEnding)
		{
			switch(lineEnding)
			{
			case leCR:
			default:
				comments.at(writePos++) = '\r';
				break;
			case leCRLF:
				comments.at(writePos++) = '\r';
				MPT_FALLTHROUGH;
			case leLF:
				comments.at(writePos++) = '\n';
				break;
			}
		} else
		{
			char c = at(i);
			comments.at(writePos++) = c;
		}
	}
	return comments;
}


OPENMPT_NAMESPACE_END
