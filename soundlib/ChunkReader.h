/*
 * ChunkReader.h
 * -------------
 * Purpose: An extended FileReader to read Iff-like chunk-based file structures.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../common/FileReader.h"

#include <vector>


OPENMPT_NAMESPACE_BEGIN


//===================================
class ChunkReader : public FileReader
//===================================
{
public:

	ChunkReader(mpt::span<const mpt::byte> bytedata) : FileReader(bytedata) { }
	ChunkReader(const FileReader &other) : FileReader(other) { }

	template<typename T>
	//=================
	class ChunkListItem
	//=================
	{
	private:
		T chunkHeader;
		FileReader chunkData;

	public:
		ChunkListItem(const T &header, const FileReader &data) : chunkHeader(header), chunkData(data) { }

		ChunkListItem<T> &operator= (const ChunkListItem<T> &other)
		{ 
			chunkHeader = other.chunkHeader;
			chunkData = other.chunkData;
			return *this;
		}

		const T &GetHeader() const { return chunkHeader; }
		const FileReader &GetData() const { return chunkData; }
	};

	template<typename T>
	//=====================================================
	class ChunkList : public std::vector<ChunkListItem<T> >
	//=====================================================
	{
	public:

		// Check if the list contains a given chunk.
		bool ChunkExists(typename T::id_type id) const
		{
			for(auto iter = this->cbegin(); iter != this->cend(); iter++)
			{
				if(iter->GetHeader().GetID() == id)
				{
					return true;
				}
			}
			return false;
		}

		// Retrieve the first chunk with a given ID.
		FileReader GetChunk(typename T::id_type id) const
		{
			for(auto iter = this->cbegin(); iter != this->cend(); iter++)
			{
				if(iter->GetHeader().GetID() == id)
				{
					return iter->GetData();
				}
			}
			return FileReader();
		}

		// Retrieve all chunks with a given ID.
		std::vector<FileReader> GetAllChunks(typename T::id_type id) const
		{
			std::vector<FileReader> result;
			for(auto iter = this->cbegin(); iter != this->cend(); iter++)
			{
				if(iter->GetHeader().GetID() == id)
				{
					result.push_back(iter->GetData());
				}
			}
			return result;
		}
	};

	// Read a single "T" chunk.
	// T is required to have the methods GetID() and GetLength(), as well as an id_type typedef.
	// GetLength() must return the chunk size in bytes, and GetID() the chunk ID.
	// id_type must reflect the type that is returned by GetID().
	template<typename T>
	ChunkListItem<T> GetNextChunk(off_t padding)
	{
		T chunkHeader;
		off_t dataSize = 0;
		if(Read(chunkHeader))
		{
			dataSize = chunkHeader.GetLength();
		}
		ChunkListItem<T> resultItem(chunkHeader, ReadChunk(dataSize));

		// Skip padding bytes
		if(padding != 0 && dataSize % padding != 0)
		{
			Skip(padding - (dataSize % padding));
		}

		return resultItem;
	}

	// Read a series of "T" chunks until the end of file is reached.
	// T is required to have the methods GetID() and GetLength(), as well as an id_type typedef.
	// GetLength() must return the chunk size in bytes, and GetID() the chunk ID.
	// id_type must reflect the type that is returned by GetID().
	template<typename T>
	ChunkList<T> ReadChunks(off_t padding)
	{
		ChunkList<T> result;
		while(CanRead(sizeof(T)))
		{
			result.push_back(GetNextChunk<T>(padding));
		}

		return result;
	}

	// Read a series of "T" chunks until a given chunk ID is found.
	// T is required to have the methods GetID() and GetLength(), as well as an id_type typedef.
	// GetLength() must return the chunk size in bytes, and GetID() the chunk ID.
	// id_type must reflect the type that is returned by GetID().
	template<typename T>
	ChunkList<T> ReadChunksUntil(off_t padding, typename T::id_type stopAtID)
	{
		ChunkList<T> result;
		while(CanRead(sizeof(T)))
		{
			result.push_back(GetNextChunk<T>(padding));
			if(result.back().GetHeader().GetID() == stopAtID)
			{
				break;
			}
		}

		return result;
	}

};


OPENMPT_NAMESPACE_END
