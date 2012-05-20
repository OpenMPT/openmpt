/*
 * ChunkReader.h
 * -------------
 * Purpose: An extended FileReader to read Iff-like chunk-based file structures.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "Loaders.h"
#include <vector>


//===================================
class ChunkReader : public FileReader
//===================================
{
public:

	ChunkReader(const char *data, size_t length) : FileReader(data, length) { }
	ChunkReader(const FileReader &other) : FileReader(other) { }

	template<typename T>
	//=================
	class ChunkListItem
	//=================
	{
	private:
		const T chunkHeader;
		ChunkReader chunkData;

	public:
		ChunkListItem(const T &header, const FileReader &data) : chunkHeader(header), chunkData(data) { }
		// VC2008 needs operator=, VC2010 doesn't...
		ChunkListItem<T>& operator= (const ChunkListItem<T> &other)
		{ 
			return MemCopy(*this, other);
		}

		const T &GetHeader() const { return chunkHeader; }
		FileReader &GetData() { return chunkData; }

		bool operator== (const ChunkListItem<T> &other)
		{
			return (GetHeader().GetID() == other.GetHeader().GetID());
		}
	};

	template<typename T>
	//=====================================================
	class ChunkList : public std::vector<ChunkListItem<T> >
	//=====================================================
	{
	public:

		// Check if the list contains a given chunk.
		template<typename IdType>
		bool ChunkExists(IdType id) const
		{
			for(const_iterator iter = begin(); iter != end(); iter++)
			{
				if(iter->GetHeader().GetID() == id)
				{
					return true;
				}
			}
			return false;
		}

		// Retrieve the first chunk with a given ID.
		template<typename IdType>
		FileReader GetChunk(IdType id)
		{
			for(iterator iter = begin(); iter != end(); iter++)
			{
				if(iter->GetHeader().GetID() == id)
				{
					return iter->GetData();
				}
			}
			return FileReader(nullptr, 0);
		}

		// Retrieve all chunks with a given ID.
		template<typename IdType>
		std::vector<FileReader> GetAllChunks(IdType id)
		{
			std::vector<FileReader> result;
			for(iterator iter = begin(); iter != end(); iter++)
			{
				if(iter->GetHeader().GetID() == id)
				{
					result.push_back(iter->GetData());
				}
			}
			return result;
		}

	};


	// Read a series of "T" chunks until the end of file is reached.
	template<typename T>
	ChunkList<T> ReadChunks(size_t padding)
	{
		ChunkList<T> result;
		while(BytesLeft())
		{
			T chunkHeader;
			if(!Read(chunkHeader))
			{
				break;
			}

			size_t dataSize = chunkHeader.GetLength();
			ChunkListItem<T> resultItem(chunkHeader, GetChunk(dataSize));
			result.push_back(resultItem);

			// Skip padding bytes
			if(padding != 0 && dataSize % padding != 0)
			{
				Skip(padding - (dataSize % padding));
			}
		}

		return result;
	}
};
