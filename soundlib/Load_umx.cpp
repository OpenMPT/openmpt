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

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

// UMX File Header
struct PACKED UMXFileHeader
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

STATIC_ASSERT(sizeof(UMXFileHeader) == 36);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


// Read compressed unreal integers - similar to MIDI integers, but signed values are possible.
static int32 ReadUMXIndex(FileReader &chunk)
//------------------------------------------
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
static std::string ReadUMXNameTableEntry(FileReader &chunk, uint16 packageVersion)
//--------------------------------------------------------------------------------
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
static int32 ReadUMXImportTableEntry(FileReader &chunk, uint16 packageVersion)
//----------------------------------------------------------------------------
{
	ReadUMXIndex(chunk);		// Class package
	ReadUMXIndex(chunk);		// Class name
	if(packageVersion >= 60)
	{
		chunk.Skip(4); // Package
	} else
	{
		ReadUMXIndex(chunk); // ??
	}
	return ReadUMXIndex(chunk);	// Object name (offset into the name table)
}


// Read an entry from the export table.
static void ReadUMXExportTableEntry(FileReader &chunk, int32 &objClass, int32 &objOffset, int32 &objSize, int32 &objName, uint16 packageVersion)
//----------------------------------------------------------------------------------------------------------------------------------------------
{
	objClass = ReadUMXIndex(chunk);	// Object class
	ReadUMXIndex(chunk);			// Object parent
	if(packageVersion >= 60)
	{
		chunk.Skip(4);					// Internal package / group of the object
	}
	objName = ReadUMXIndex(chunk);	// Object name (offset into the name table)
	chunk.Skip(4);					// Object flags
	objSize = ReadUMXIndex(chunk);
	if(objSize > 0)
	{
		objOffset = ReadUMXIndex(chunk);
	}
}


bool CSoundFile::ReadUMX(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
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
		int32 objName = ReadUMXImportTableEntry(file, fileHeader.packageVersion);
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
	InitializeGlobals();
	madeWithTracker = mpt::String::Print("Unreal Package v%1", fileHeader.packageVersion);
	
	for(uint32 i = 0; i < fileHeader.exportCount; i++)
	{
		int32 objClass, objOffset, objSize, objName;
		ReadUMXExportTableEntry(file, objClass, objOffset, objSize, objName, fileHeader.packageVersion);

		if(objSize <= 0 || objClass >= 0)
		{
			continue;
		}

		// Look up object class name (we only want music and sounds).
		objClass = -objClass - 1;
		bool isMusic = false;
#ifdef MODPLUG_TRACKER
		bool isSound = false;
#endif // MODPLUG_TRACKER
		if(static_cast<size_t>(objClass) < classes.size())
		{
			const char *objClassName = names[classes[objClass]].c_str();
			isMusic = !strcmp(objClassName, "music");
#ifdef MODPLUG_TRACKER
			isSound = !strcmp(objClassName, "sound");
#endif // MODPLUG_TRACKER
		}
		if(!isMusic
#ifdef MODPLUG_TRACKER
			&& !isSound
#endif // MODPLUG_TRACKER
			)
		{
			continue;
		} else if(loadFlags == onlyVerifyHeader)
		{
			return true;
		}

		FileReader chunk = file.GetChunk(objOffset, objSize);

		if(chunk.IsValid())
		{
			if(fileHeader.packageVersion < 40)
			{
				chunk.Skip(8); // 00 00 00 00 00 00 00 00
			}
			if(fileHeader.packageVersion < 60)
			{
				chunk.Skip(16); // 81 00 00 00 00 00 FF FF FF FF FF FF FF FF 00 00
			}
			// Read object properties
#if 0
			size_t propertyName = static_cast<size_t>(ReadUMXIndex(chunk));
			if(propertyName >= names.size() || names[propertyName] != "none")
			{
				// Can't bother to implement property reading, as no UMX files I've seen so far use properties for the relevant objects,
				// and only the UAX files in the Unreal 1997/98 beta seem to use this and still load just fine when ignoring it.
				// If it should be necessary to implement this, check CUnProperty.cpp in http://ut-files.com/index.php?dir=Utilities/&file=utcms_source.zip
				ASSERT(false);
				continue;
			}
#else
			ReadUMXIndex(chunk);
#endif

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

			if(isMusic)
			{
				// Read as module
				if(ReadIT(fileChunk, loadFlags)
					|| ReadXM(fileChunk, loadFlags)
					|| ReadS3M(fileChunk, loadFlags)
					|| ReadWav(fileChunk, loadFlags)
					|| ReadSTM(fileChunk, loadFlags)
					|| Read669(fileChunk, loadFlags)
					|| ReadFAR(fileChunk, loadFlags)
					|| ReadMod(fileChunk, loadFlags)
					|| ReadM15(fileChunk, loadFlags))
				{
					m_ContainerType = MOD_CONTAINERTYPE_UMX;
					return true;
				}
#ifdef MODPLUG_TRACKER
			} else if(isSound && GetNumSamples() < MAX_SAMPLES - 1)
			{
				// Read as sample
				if(ReadSampleFromFile(GetNumSamples() + 1, fileChunk, true))
				{
					if(static_cast<size_t>(objName) < names.size())
					{
						mpt::String::Copy(m_szNames[GetNumSamples()], names[objName]);
					}
				}
#endif // MODPLUG_TRACKER
			}
		}
	}

#ifdef MODPLUG_TRACKER
	if(m_nSamples != 0)
	{
		InitializeChannels();
		m_nType = MOD_TYPE_UAX;
		m_nChannels = 4;
		Patterns.Insert(0, 64);
		Order[0] = 0;
		return true;
	} else
#endif // MODPLUG_TRACKER
	{
		return false;
	}
}
