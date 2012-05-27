/*
 * StringFixer.h
 * -------------
 * Purpose: Various functions for "fixing" char array strings for writing to or
 *          reading from module files, or for securing char arrays in general.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

namespace StringFixer
{

	// Sets last character to null in given char array.
	// Size of the array must be known at compile time.
	template <size_t size>
	void SetNullTerminator(char (&buffer)[size])
	//------------------------------------------
	{
		STATIC_ASSERT(size > 0);
		buffer[size - 1] = 0;
	}


	// Remove any chars after the first null char
	template <size_t size>
	void FixNullString(char (&buffer)[size])
	//--------------------------------------
	{
		STATIC_ASSERT(size > 0);
		SetNullTerminator(buffer);
		size_t pos = 0;
		// Find the first null char.
		while(buffer[pos] != '\0' && pos < size)
		{
			pos++;
		}
		// Remove everything after the null char.
		while(pos < size)
		{
			buffer[pos++] = '\0';
		}
	}


	enum ReadWriteMode
	{
		// Reading / Writing: Standard null-terminated string handling.
		nullTerminated,
		// Reading: Source string is not guaranteed to be null-terminated (if it fills the whole char array).
		// Writing: Destination string is not guaranteed to be null-terminated (if it fills the whole char array).
		maybeNullTerminated,
		// Reading: String may contain null characters anywhere. They should be treated as spaces.
		// Writing: A space-padded string is written.
		spacePadded,
		// Reading: String may contain null characters anywhere. The last character is ignored (it is supposed to be 0).
		// Writing: A space-padded string with a trailing null is written.
		spacePaddedNull,
	};


	// Copy a string from srcBuffer to destBuffer using a given read mode.
	// Used for reading strings from files.
	// Preferrably use this version of the function, it is safer.
	template <ReadWriteMode mode, size_t destSize, size_t srcSize>
	void ReadString(char (&destBuffer)[destSize], const char (&srcBuffer)[srcSize])
	//-----------------------------------------------------------------------------
	{
		STATIC_ASSERT(destSize > 0);
		STATIC_ASSERT(srcSize > 0);
		ReadString<mode, destSize>(destBuffer, srcBuffer, srcSize);
	}


	// Copy a string from srcBuffer to destBuffer using a given read mode.
	// Used for reading strings from files.
	// Only use this version of the function if the size of the source buffer is variable.
	template <ReadWriteMode mode, size_t destSize>
	void ReadString(char (&destBuffer)[destSize], const char *srcBuffer, const size_t srcSize)
	//----------------------------------------------------------------------------------------
	{
		STATIC_ASSERT(destSize > 0);
		//ASSERT(srcSize > 0);

		const size_t maxSize = min(destSize, srcSize);
		char *dst = destBuffer;
		const char *src = srcBuffer;

		if(mode == nullTerminated || mode == maybeNullTerminated)
		{
			// Copy null-terminated string and make sure that destination is null-terminated.
			size_t pos = maxSize;
			while(pos > 0)
			{
				pos--;
				if((*dst++ = *src++) == '\0')
				{
					break;
				}
			}
			// Fill rest of string with nulls.
			memset(dst, '\0', destSize - maxSize + pos);
			
			if(mode == nullTerminated)
			{
				// We assume that the last character of the source buffer is null.
				destBuffer[maxSize - 1] = '\0';
			} else
			{
				// Last character of source buffer may actually be a valid character.
				destBuffer[min(destSize - 1, srcSize)] = '\0';
			}

		} else if(mode == spacePadded || mode == spacePaddedNull)
		{
			// Copy string over, but convert null characters to spaces.
			size_t pos = maxSize;
			while(pos > 0)
			{
				*dst = *src;
				if(*dst == '\0')
				{
					*dst = ' ';
				}
				pos--;
				dst++;
				src++;
			}
			// Fill rest of string with nulls.
			memset(dst, '\0', destSize - maxSize);

			if(mode == spacePaddedNull && srcSize <= destSize)
			{
				// We assumed that the last character of the source buffer should be ignored, so make sure it's really null.
				destBuffer[srcSize - 1] = '\0';
			}

			// Trim trailing spaces.
			pos = maxSize;
			dst = destBuffer + pos - 1;
			while(pos > 0)
			{
				if(*dst == ' ')
				{
					*dst = '\0';
				} else if(*dst != '\0')
				{
					break;
				}
				pos--;
				dst--;
			}

			SetNullTerminator(destBuffer);
		}
	}


	// Copy a string from srcBuffer to destBuffer using a given write mode.
	// Used for writing strings to files.
	// Preferrably use this version of the function, it is safer.
	template <ReadWriteMode mode, size_t destSize, size_t srcSize>
	void WriteString(char (&destBuffer)[destSize], const char (&srcBuffer)[srcSize])
	//------------------------------------------------------------------------------
	{
		STATIC_ASSERT(destSize > 0);
		STATIC_ASSERT(srcSize > 0);
		WriteString<mode, destSize>(destBuffer, srcBuffer, srcSize);
	}

	// Copy a string from srcBuffer to destBuffer using a given write mode.
	// Used for writing strings to files.
	// Only use this version of the function if the size of the source buffer is variable.
	template <ReadWriteMode mode, size_t destSize>
	void WriteString(char (&destBuffer)[destSize], const char *srcBuffer, const size_t srcSize)
	//-----------------------------------------------------------------------------------------
	{
		STATIC_ASSERT(destSize > 0);
		ASSERT(srcSize > 0);

		const size_t maxSize = min(destSize, srcSize);
		char *dst = destBuffer;
		const char *src = srcBuffer;

		// First, copy over null-terminated string.
		size_t pos = maxSize;
		while(pos > 0)
		{
			if((*dst = *src) == '\0')
			{
				break;
			}
			pos--;
			dst++;
			src++;
		}

		if(mode == nullTerminated || mode == maybeNullTerminated)
		{
			// Fill rest of string with nulls.
			memset(dst, '\0', destSize - maxSize + pos);
		} else if(mode == spacePadded || mode == spacePaddedNull)
		{
			// Fill the rest of the destination string with spaces.
			memset(dst, ' ', destSize - maxSize + pos);
		}

		if(mode == nullTerminated || mode == spacePaddedNull)
		{
			// Make sure that destination is really null-terminated.
			SetNullTerminator(destBuffer);
		}
	}

};
