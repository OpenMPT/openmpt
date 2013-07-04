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

#include <string>
#include <vector>
#include <cstring>
#include <string.h>

namespace mpt { namespace String
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

	inline void SetNullTerminator(char *buffer, size_t size)
	//------------------------------------------------------
	{
		ASSERT(size > 0);
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

	inline void FixNullString(std::string & str)
	//------------------------------------------
	{
		for(std::size_t i = 0; i < str.length(); ++i)
		{
			if(str[i] == '\0')
			{
				// if we copied \0 in the middle of the buffer, terminate std::string here
				str.resize(i);
				break;
			}
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
		spacePaddedNull
	};


	// Copy a string from srcBuffer to destBuffer using a given read mode.
	// Used for reading strings from files.
	// Only use this version of the function if the size of the source buffer is variable.
	template <ReadWriteMode mode>
	void Read(std::string &dest, const char *srcBuffer, const size_t srcSize)
	//-----------------------------------------------------------------------
	{
		ASSERT(srcSize > 0);

		dest.clear();

		if(mode == nullTerminated || mode == maybeNullTerminated)
		{
			// Copy null-terminated string
			// We cannot use std::string::assign(const char*, size_t) because that would not stop at \0s in the middle of the buffer.
			for(const char *src = srcBuffer; src != srcBuffer + srcSize && *src; ++src)
			{
				dest.push_back(*src);
			}

			if(mode == nullTerminated)
			{
				// We assume that the last character of the source buffer is null.
				if(dest.length() == srcSize)
				{
					dest.resize(dest.length() - 1);
				}
			}

		} else if(mode == spacePadded || mode == spacePaddedNull)
		{
			// Copy string over, but convert null characters to spaces.
			for(const char *src = srcBuffer; src != srcBuffer + srcSize; ++src)
			{
				char c = *src;
				if(c == '\0')
				{
					c = ' ';
				}
				dest.push_back(c);
			}

			if(mode == spacePaddedNull)
			{
				if(dest.length() == srcSize)
				{
					dest.resize(dest.length() - 1);
				}
			}

			// Trim trailing spaces.
			dest = mpt::String::RTrim(dest);

		}
	}

	// Used for reading strings from files.
	// Preferrably use this version of the function, it is safer.
	template <ReadWriteMode mode, size_t srcSize>
	void Read(std::string &dest, const char (&srcBuffer)[srcSize])
	//------------------------------------------------------------
	{
		STATIC_ASSERT(srcSize > 0);
		Read<mode>(dest, srcBuffer, srcSize);
	}


	// Copy a string from srcBuffer to destBuffer using a given read mode.
	// Used for reading strings from files.
	// Only use this version of the function if the size of the source buffer is variable.
	template <ReadWriteMode mode, size_t destSize>
	void Read(char (&destBuffer)[destSize], const char *srcBuffer, const size_t srcSize)
	//----------------------------------------------------------------------------------
	{
		STATIC_ASSERT(destSize > 0);
		//ASSERT(srcSize > 0);

		const size_t maxSize = MIN(destSize, srcSize);
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
				destBuffer[MIN(destSize - 1, srcSize)] = '\0';
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

	// Used for reading strings from files.
	// Preferrably use this version of the function, it is safer.
	template <ReadWriteMode mode, size_t destSize, size_t srcSize>
	void Read(char (&destBuffer)[destSize], const char (&srcBuffer)[srcSize])
	//-----------------------------------------------------------------------
	{
		STATIC_ASSERT(destSize > 0);
		STATIC_ASSERT(srcSize > 0);
		Read<mode, destSize>(destBuffer, srcBuffer, srcSize);
	}


	// Copy a string from srcBuffer to destBuffer using a given write mode.
	// You should only use this function if src and dest are dynamically sized,
	// otherwise use one of the safer overloads below.
	template <ReadWriteMode mode>
	void Write(char *destBuffer, const size_t destSize, const char *srcBuffer, const size_t srcSize)
	//----------------------------------------------------------------------------------------------
	{
		ASSERT(destSize > 0);

		const size_t maxSize = MIN(destSize, srcSize);
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
			SetNullTerminator(destBuffer, destSize);
		}
	}

	// Copy a string from srcBuffer to a dynamically sized std::vector destBuffer using a given write mode.
	// Used for writing strings to files.
	// Only use this version of the function if the size of the source buffer is variable and the destination buffer also has variable size.
	template <ReadWriteMode mode>
	void Write(std::vector<char> &destBuffer, const char *srcBuffer, const size_t srcSize)
	//------------------------------------------------------------------------------------
	{
		ASSERT(destBuffer.size() > 0);
		Write<mode>(destBuffer.data(), destBuffer.size(), srcBuffer, srcSize);
	}

	// Copy a string from srcBuffer to destBuffer using a given write mode.
	// Used for writing strings to files.
	// Only use this version of the function if the size of the source buffer is variable.
	template <ReadWriteMode mode, size_t destSize>
	void Write(char (&destBuffer)[destSize], const char *srcBuffer, const size_t srcSize)
	//-----------------------------------------------------------------------------------
	{
		STATIC_ASSERT(destSize > 0);
		Write<mode>(destBuffer, destSize, srcBuffer, srcSize);
	}

	// Copy a string from srcBuffer to destBuffer using a given write mode.
	// Used for writing strings to files.
	// Preferrably use this version of the function, it is safer.
	template <ReadWriteMode mode, size_t destSize, size_t srcSize>
	void Write(char (&destBuffer)[destSize], const char (&srcBuffer)[srcSize])
	//------------------------------------------------------------------------
	{
		STATIC_ASSERT(destSize > 0);
		STATIC_ASSERT(srcSize > 0);
		Write<mode, destSize>(destBuffer, srcBuffer, srcSize);
	}

	template <ReadWriteMode mode>
	void Write(char *destBuffer, const size_t destSize, const std::string &src)
	//-------------------------------------------------------------------------
	{
		ASSERT(destSize > 0);
		Write<mode>(destBuffer, destSize, src.c_str(), src.length());
	}

	template <ReadWriteMode mode>
	void Write(std::vector<char> &destBuffer, const std::string &src)
	//---------------------------------------------------------------
	{
		ASSERT(destBuffer.size() > 0);
		Write<mode>(destBuffer, src.c_str(), src.length());
	}

	template <ReadWriteMode mode, size_t destSize>
	void Write(char (&destBuffer)[destSize], const std::string &src)
	//--------------------------------------------------------------
	{
		STATIC_ASSERT(destSize > 0);
		Write<mode, destSize>(destBuffer, src.c_str(), src.length());
	}


	// Copy from a char array to a fixed size char array.
	template <size_t destSize>
	void CopyN(char (&destBuffer)[destSize], const char *srcBuffer, const size_t srcSize = SIZE_MAX)
	//----------------------------------------------------------------------------------------------
	{
		const size_t copySize = MIN(destSize - 1, srcSize);
		std::strncpy(destBuffer, srcBuffer, copySize);
		destBuffer[copySize] = '\0';
	}

	// Copy at most srcSize characters from srcBuffer to a std::string.
	static inline void CopyN(std::string &dest, const char *srcBuffer, const size_t srcSize = SIZE_MAX)
	//-------------------------------------------------------------------------------------------------
	{
		dest.assign(srcBuffer, srcBuffer + mpt::strnlen(srcBuffer, srcSize));
	}


	// Copy from one fixed size char array to another one.
	template <size_t destSize, size_t srcSize>
	void Copy(char (&destBuffer)[destSize], const char (&srcBuffer)[srcSize])
	//-----------------------------------------------------------------------
	{
		CopyN(destBuffer, srcBuffer, srcSize);
	}

	// Copy from a std::string to a fixed size char array.
	template <size_t destSize>
	void Copy(char (&destBuffer)[destSize], const std::string &src)
	//-------------------------------------------------------------
	{
		CopyN(destBuffer, src.c_str(), src.length());
	}

	// Copy from a fixed size char array to a std::string.
	template <size_t srcSize>
	void Copy(std::string &dest, const char (&srcBuffer)[srcSize])
	//----------------------------------------------------------------------------
	{
		CopyN(dest, srcBuffer, srcSize);
	}

	// Copy from a std::string to a std::string.
	static inline void Copy(std::string &dest, const std::string &src)
	//----------------------------------------------------------------
	{
		dest.assign(src);
	}


} } // namespace mpt::String

