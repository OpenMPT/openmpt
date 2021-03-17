/*
* UMXTools.h
* ------------
* Purpose: UMX/UAX (Unreal package) helper functions
* Notes  : (currently none)
* Authors: OpenMPT Devs (inspired by code from http://wiki.beyondunreal.com/Legacy:Package_File_Format)
* The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
*/


#include "stdafx.h"
#include "Loaders.h"
#include "UMXTools.h"


OPENMPT_NAMESPACE_BEGIN

namespace UMX
{

bool UMXFileHeader::IsValid() const
{
	return !std::memcmp(magic, "\xC1\x83\x2A\x9E", 4)
		&& nameOffset >= sizeof(UMXFileHeader)
		&& exportOffset >= sizeof(UMXFileHeader)
		&& importOffset >= sizeof(UMXFileHeader)
		&& nameCount > 0 && nameCount <= uint32_max / 5u
		&& exportCount > 0 && exportCount <= uint32_max / 8u
		&& importCount > 0 && importCount <= uint32_max / 4u
		&& uint32_max - nameCount * 5u >= nameOffset
		&& uint32_max - exportCount * 8u >= exportOffset
		&& uint32_max - importCount * 4u >= importOffset;
}


uint32 UMXFileHeader::GetMinimumAdditionalFileSize() const
{
	return std::max({nameOffset + nameCount * 5u, exportOffset + exportCount * 8u, importOffset + importCount * 4u}) - sizeof(UMXFileHeader);
}


CSoundFile::ProbeResult ProbeFileHeader(MemoryFileReader file, const uint64 *pfilesize, const char *requiredType)
{
	UMXFileHeader fileHeader;
	if(!file.ReadStruct(fileHeader))
	{
		return CSoundFile::ProbeWantMoreData;
	}
	if(!fileHeader.IsValid())
	{
		return CSoundFile::ProbeFailure;
	}
	if(requiredType != nullptr && !FindUMXNameTableEntryMemory(file, fileHeader, requiredType))
	{
		return CSoundFile::ProbeFailure;
	}
	return CSoundFile::ProbeAdditionalSize(file, pfilesize, fileHeader.GetMinimumAdditionalFileSize());
}


// Read compressed unreal integers - similar to MIDI integers, but signed values are possible.
template <typename Tfile>
static int32 ReadUMXIndexImpl(Tfile &chunk)
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

int32 ReadUMXIndex(FileReader &chunk)
{
	return ReadUMXIndexImpl(chunk);
}


// Returns true if the given nme exists in the name table.
template <typename TFile>
static bool FindUMXNameTableEntryImpl(TFile &file, const UMXFileHeader &fileHeader, const char *name)
{
	if(!name)
	{
		return false;
	}
	std::size_t nameLen = std::strlen(name);
	if(nameLen == 0)
	{
		return false;
	}
	bool result = false;
	const FileReader::off_t oldpos = file.GetPosition();
	if(file.Seek(fileHeader.nameOffset))
	{
		for(uint32 i = 0; i < fileHeader.nameCount && file.CanRead(5); i++)
		{
			if(fileHeader.packageVersion >= 64)
			{
				int32 length = ReadUMXIndexImpl(file);
				if(length <= 0)
				{
					continue;
				}
			}
			bool match = true;
			std::size_t pos = 0;
			char c = 0;
			while((c = file.ReadUint8()) != 0)
			{
				c = mpt::ToLowerCaseAscii(c);
				if(pos < nameLen)
				{
					match = match && (c == name[pos]);
				}
				pos++;
			}
			if(pos != nameLen)
			{
				match = false;
			}
			if(match)
			{
				result = true;
			}
			file.Skip(4);  // Object flags
		}
	}
	file.Seek(oldpos);
	return result;
}

bool FindUMXNameTableEntry(FileReader &file, const UMXFileHeader &fileHeader, const char *name)
{
	return FindUMXNameTableEntryImpl(file, fileHeader, name);
}

bool FindUMXNameTableEntryMemory(MemoryFileReader &file, const UMXFileHeader &fileHeader, const char *name)
{
	return FindUMXNameTableEntryImpl(file, fileHeader, name);
}


// Read an entry from the name table.
std::string ReadUMXNameTableEntry(FileReader &chunk, uint16 packageVersion)
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
		name.reserve(std::min(length, mpt::saturate_cast<int32>(chunk.BytesLeft())));
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


// Read complete name table.
std::vector<std::string> ReadUMXNameTable(FileReader &file, const UMXFileHeader &fileHeader)
{
	file.Seek(fileHeader.nameOffset);  // nameOffset and nameCount were validated when parsing header
	std::vector<std::string> names;
	names.reserve(fileHeader.nameCount);
	for(uint32 i = 0; i < fileHeader.nameCount && file.CanRead(5); i++)
	{
		names.push_back(ReadUMXNameTableEntry(file, fileHeader.packageVersion));
	}
	return names;
}


// Read an entry from the import table.
int32 ReadUMXImportTableEntry(FileReader &chunk, uint16 packageVersion)
{
	ReadUMXIndex(chunk);  // Class package
	ReadUMXIndex(chunk);  // Class name
	if(packageVersion >= 60)
	{
		chunk.Skip(4);  // Package
	} else
	{
		ReadUMXIndex(chunk);  // ??
	}
	return ReadUMXIndex(chunk);  // Object name (offset into the name table)
}


// Read import table.
std::vector<int32> ReadUMXImportTable(FileReader &file, const UMXFileHeader &fileHeader, const std::vector<std::string> &names)
{
	file.Seek(fileHeader.importOffset);  // importOffset and importCount were validated when parsing header
	std::vector<int32> classes;
	classes.reserve(fileHeader.importCount);
	for(uint32 i = 0; i < fileHeader.importCount && file.CanRead(4); i++)
	{
		int32 objName = ReadUMXImportTableEntry(file, fileHeader.packageVersion);
		if(static_cast<size_t>(objName) < names.size())
		{
			classes.push_back(objName);
		}
	}
	return classes;
}


// Read an entry from the export table.
void ReadUMXExportTableEntry(FileReader &chunk, int32 &objClass, int32 &objOffset, int32 &objSize, int32 &objName, uint16 packageVersion)
{
	objClass = ReadUMXIndex(chunk);  // Object class
	ReadUMXIndex(chunk);             // Object parent
	if(packageVersion >= 60)
	{
		chunk.Skip(4);  // Internal package / group of the object
	}
	objName = ReadUMXIndex(chunk);  // Object name (offset into the name table)
	chunk.Skip(4);                  // Object flags
	objSize = ReadUMXIndex(chunk);
	if(objSize > 0)
	{
		objOffset = ReadUMXIndex(chunk);
	}
}

}  // namespace UMX

OPENMPT_NAMESPACE_END
