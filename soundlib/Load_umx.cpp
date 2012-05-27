/*
 * Load_umx.cpp
 * ------------
 * Purpose: UMX (Unreal Music) and UAX (Unreal Sounds) module ripper
 * Notes  : Obviously, this code only rips modules from older Unreal Engine games, such as Unreal 1, Unreal Tournament 1 and Deus Ex.
 *          For UAX sound packages, the sounds are read into module sample slots.
 * Authors: Johannes Schultz (inspired by code from http://wiki.beyondunreal.com/Legacy:Package_File_Format)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

#pragma pack(push, 1)

// UMX File Header
struct UMXFileHeader
{
	// Magic Bytes
	enum UMXMagic
	{
		magicBytes = 0x9E2A83C1,
	};

	uint32 magic;
	uint16 packageVersion;
	uint16 licenseMode;
	uint32 flags;
	uint32 nameCount;
	uint32 nameOffset;
	uint32 exportCount;
	uint32 exportOffset;
	uint32 importCount;
	uint32 importOffset;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(magic);
		SwapBytesLE(packageVersion);
		// Don't need the rest.
		SwapBytesLE(nameCount);
		SwapBytesLE(nameOffset);
		SwapBytesLE(exportCount);
		SwapBytesLE(exportOffset);
		SwapBytesLE(importCount);
		SwapBytesLE(importOffset);
	}
};

#pragma pack(pop)


// Read compressed unreal integers - similar to MIDI integers, but signed values are possible.
int32 ReadUMXIndex(FileReader &chunk)
//-----------------------------------
{
	enum
	{
		signMask		= 0x80,	// Highest bit of first byte indicates if value is signed
		valueMask1		= 0x3F,	// Low 6 bits of first byte are actual value
		continueMask1	= 0x40,	// Second-highest bit of first byte indicates if further bytes follow
		valueMask		= 0x7F,	// Low 7 bits of following bytes are actual value
		continueMask	= 0x80,	// Highest bit of following bytes indicates if further bytes follow
	};

	// Read first byte
	uint8 b = chunk.ReadUint8();
	bool isSigned = (b & signMask) != 0;
	int32 result = (b & valueMask1);
	int shift = 6;

	if(b & continueMask1)
	{
		// Read remaining bytes
		do
		{
			b = chunk.ReadUint8();
			int32 data = static_cast<int32>(b) & valueMask;
			data <<= shift;
			result |= data;
			shift += 7;
		} while((b & continueMask) != 0 && (shift < 32));
	}

	if(isSigned)
	{
		result = -result;
	}
	return result;
}


// Read an entry from the name table.
std::string ReadUMXNameTableEntry(FileReader &chunk, uint16 packageVersion)
//-------------------------------------------------------------------------
{
	std::string name;
	if(packageVersion >= 64)
	{
		// String length
		int32 length = ReadUMXIndex(chunk);
		if(length <= 0)
		{
			return "";
		}
		name.reserve(length);
	}

	// Simple zero-terminated string
	uint8 chr;
	while((chr = chunk.ReadUint8()) != 0)
	{
		// Convert string to lower case
		if(chr >= 'A' && chr <= 'Z')
		{
			chr = chr - 'A' + 'a';
		}
		name.append(1, static_cast<char>(chr));
	}

	chunk.Skip(4);	// Object flags
	return name;
}


// Read an entry from the import table.
int32 ReadUMXImportTableEntry(FileReader &chunk)
//----------------------------------------------
{
	ReadUMXIndex(chunk);		// Class package
	ReadUMXIndex(chunk);		// Class name
	chunk.Skip(4);				// Package
	return ReadUMXIndex(chunk);	// Object name (offset into the name table)
}


// Read an entry from the export table.
void ReadUMXExportTableEntry(FileReader &chunk, int32 &objClass, int32 &objOffset, int32 &objSize, int32 &objName)
//----------------------------------------------------------------------------------------------------------------
{
	objClass = ReadUMXIndex(chunk);	// Object class
	ReadUMXIndex(chunk);			// Object parent
	chunk.Skip(4);					// Internal package / group of the object
	objName = ReadUMXIndex(chunk);	// Object name (offset into the name table)
	chunk.Skip(4);					// Object flags
	objSize = ReadUMXIndex(chunk);
	if(objSize > 0)
	{
		objOffset = ReadUMXIndex(chunk);
	}
}


bool CSoundFile::ReadUMX(FileReader &file)
//----------------------------------------
{
	file.Rewind();
	UMXFileHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader)
		|| fileHeader.magic != UMXFileHeader::magicBytes
		|| !file.Seek(fileHeader.nameOffset))
	{
		return false;
	}

	// Read name table
	std::vector<std::string> names;
	names.reserve(fileHeader.nameCount);
	for(uint32 i = 0; i < fileHeader.nameCount; i++)
	{
		names.push_back(ReadUMXNameTableEntry(file, fileHeader.packageVersion));
	}

	// Read import table
	if(!file.Seek(fileHeader.importOffset))
	{
		return false;
	}

	std::vector<int32> classes;
	classes.reserve(fileHeader.importCount);
	for(uint32 i = 0; i < fileHeader.importCount; i++)
	{
		int32 objName = ReadUMXImportTableEntry(file);
		if(static_cast<size_t>(objName) < names.size())
		{
			classes.push_back(objName);
		}
	}

	// Read export table
	if(!file.Seek(fileHeader.exportOffset))
	{
		return false;
	}

	// Now we can be pretty sure that we're doing the right thing.
	m_nType = MOD_TYPE_UMX;
	m_nSamples = 0;
	m_nInstruments = 0;

	for(uint32 i = 0; i < fileHeader.exportCount; i++)
	{
		int32 objClass, objOffset, objSize, objName;
		ReadUMXExportTableEntry(file, objClass, objOffset, objSize, objName);

		if(objSize <= 0 || objClass >= 0)
		{
			continue;
		}

		// Look up object class name (we only want music and sounds).
		objClass = -objClass - 1;
		bool isMusic = false, isSound = false;
		if(static_cast<size_t>(objClass) < classes.size())
		{
			const char *objClassName = names[classes[objClass]].c_str();
			isMusic = !strcmp(objClassName, "music");
			isSound = !strcmp(objClassName, "sound");
		}
		if(!isMusic && !isSound)
		{
			continue;
		}

		FileReader chunk = file.GetChunk(objOffset, objSize);

		if(chunk.IsValid())
		{
			// Read object properties
			size_t propertyName = static_cast<size_t>(ReadUMXIndex(chunk));
			if(propertyName >= names.size() || strcmp(names[propertyName].c_str(), "none"))
			{
				// Can't bother to implement property reading, as no UMX or UAX files I've seen so far use properties for the relevant objects.
				// If it should be necessary to implement this, check CUnProperty.cpp in http://ut-files.com/index.php?dir=Utilities/&file=utcms_source.zip
				ASSERT(false);
				continue;
			}

			if(fileHeader.packageVersion >= 120)
			{
				// UT2003 Packages
				ReadUMXIndex(chunk);
				chunk.Skip(8);
			} else if(fileHeader.packageVersion >= 100)
			{
				// AAO Packages
				chunk.Skip(4);
				ReadUMXIndex(chunk);
				chunk.Skip(4);
			} else if(fileHeader.packageVersion >= 62)
			{
				// UT Packages
				// Mech8.umx and a few other UT tunes have packageVersion = 62. 
				// In CUnSound.cpp, the condition above reads "packageVersion >= 63" but if that is used, those tunes won't load properly.
				ReadUMXIndex(chunk);
				chunk.Skip(4);
			} else
			{
				// Old Unreal Packagaes
				ReadUMXIndex(chunk);
			}
			int32 size = ReadUMXIndex(chunk);

			FileReader fileChunk = chunk.GetChunk(size);
			// TODO: Use FileReader for those file types
			const BYTE *data = reinterpret_cast<const BYTE *>(fileChunk.GetRawData());

			if(isMusic)
			{
				// Read as module
				if(ReadIT(data, fileChunk.GetLength())
					|| ReadXM(data, fileChunk.GetLength())
					|| ReadS3M(fileChunk)
					|| ReadWav(fileChunk)
					|| ReadMod(fileChunk))
				{
					return true;
				}
			} else if(isSound && GetNumSamples()  + 1 < MAX_SAMPLES)
			{
				// Read as sample
				if(ReadSampleFromFile(GetNumSamples() + 1, (LPBYTE)data, fileChunk.GetLength()))
				{
					m_nSamples++;
					if(static_cast<size_t>(objName) < names.size())
					{
						strncpy(m_szNames[GetNumSamples()], names[objName].c_str(), MAX_SAMPLENAME - 1);
					}
				}
			}
		}
	}

	if(m_nSamples != 0)
	{
		m_nChannels = 4;
		Patterns.Insert(0, 64);
		Order[0] = 0;
		return true;
	} else
	{
		m_nType = MOD_TYPE_NONE;
		return false;
	}
}
