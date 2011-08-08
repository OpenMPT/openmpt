/*
 * StringFixer.h
 * -------------
 * Purpose: Various functions for "fixing" char array strings for writing to or
 *          reading from module files, or for securing char arrays in general.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */


#ifndef STRINGFIXER_H
#define STRINGFIXER_H
#pragma once


// Sets last character to null in given char array.
// Size of the array must be known at compile time.
template <size_t size>
inline void SetNullTerminator(char (&buffer)[size])
//-------------------------------------------------
{
	STATIC_ASSERT(size > 0);
	buffer[size-1] = 0;
}


// Convert a 0-terminated string to a space-padded string
template <size_t size>
void NullToSpaceString(char (&buffer)[size])
//------------------------------------------
{
	STATIC_ASSERT(size > 0);
	size_t pos = size;
	while (pos-- > 0)
		if (buffer[pos] == 0)
			buffer[pos] = 32;
	buffer[size - 1] = 0;
}


// Convert a space-padded string to a 0-terminated string
template <size_t size>
void SpaceToNullString(char (&buffer)[size])
//------------------------------------------
{
	STATIC_ASSERT(size > 0);
	// First, remove any Nulls
	NullToSpaceString(buffer);
	size_t pos = size;
	while (pos-- > 0)
	{
		if (buffer[pos] == 32)
			buffer[pos] = 0;
		else if(buffer[pos] != 0)
			break;
	}
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


// Convert a space-padded string to a 0-terminated string. STATIC VERSION! (use this if the maximum string length is known)
// Additional template parameter to specifify the max length of the final string,
// not including null char (useful for e.g. mod loaders)
template <size_t length, size_t size>
void SpaceToNullStringFixed(char (&buffer)[size])
//------------------------------------------------
{
	STATIC_ASSERT(size > 0);
	STATIC_ASSERT(length < size);
	// Remove Nulls in string
	SpaceToNullString(buffer);
	// Overwrite trailing chars
	for(size_t pos = length; pos < size; pos++)
	{
		buffer[pos] = 0;
	}
}


// Convert a space-padded string to a 0-terminated string. DYNAMIC VERSION!
// Additional function parameter to specifify the max length of the final string,
// not including null char (useful for e.g. mod loaders)
template <size_t size>
void SpaceToNullStringFixed(char (&buffer)[size], size_t length)
//--------------------------------------------------------------
{
	STATIC_ASSERT(size > 0);
	ASSERT(length < size);
	// Remove Nulls in string
	SpaceToNullString(buffer);
	// Overwrite trailing chars
	for(size_t pos = length; pos < size; pos++)
	{
		buffer[pos] = 0;
	}
}


#endif // STRINGFIXER_H
