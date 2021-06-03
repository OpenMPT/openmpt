/*
 * mptIO.h
 * -------
 * Purpose: Basic functions for reading/writing binary and endian safe data to/from files/streams.
 * Notes  : Some useful functions for reading and writing are still missing.
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/utility.hpp"
#include "mpt/io/base.hpp"
#include "mpt/io/io.hpp"
#include "mpt/io/io_span.hpp"
#include "mpt/io/io_virtual_wrapper.hpp"
#include "openmpt/base/Endian.hpp"

#include "mptAssert.h"
#include <algorithm>
#include <array>
#include <iosfwd>
#include <limits>
#include <cstring>


OPENMPT_NAMESPACE_BEGIN



class IFileDataContainer {
public:
	typedef std::size_t pos_type;
protected:
	IFileDataContainer() = default;
public:
	IFileDataContainer(const IFileDataContainer&) = default;
	IFileDataContainer & operator=(const IFileDataContainer&) = default;
	virtual ~IFileDataContainer() = default;
public:
	virtual bool IsValid() const = 0;
	virtual bool HasFastGetLength() const = 0;
	virtual bool HasPinnedView() const = 0;
	virtual const std::byte *GetRawData() const = 0;
	virtual pos_type GetLength() const = 0;
	virtual mpt::byte_span Read(pos_type pos, mpt::byte_span dst) const = 0;

	virtual bool CanRead(pos_type pos, std::size_t length) const
	{
		pos_type dataLength = GetLength();
		if((pos == dataLength) && (length == 0))
		{
			return true;
		}
		if(pos >= dataLength)
		{
			return false;
		}
		return length <= dataLength - pos;
	}

	virtual std::size_t GetReadableLength(pos_type pos, std::size_t length) const
	{
		pos_type dataLength = GetLength();
		if(pos >= dataLength)
		{
			return 0;
		}
		return std::min(length, dataLength - pos);
	}
};


class FileDataContainerDummy : public IFileDataContainer {
public:
	FileDataContainerDummy() { }
public:
	bool IsValid() const override
	{
		return false;
	}

	bool HasFastGetLength() const override
	{
		return true;
	}

	bool HasPinnedView() const override
	{
		return true;
	}

	const std::byte *GetRawData() const override
	{
		return nullptr;
	}

	pos_type GetLength() const override
	{
		return 0;
	}
	mpt::byte_span Read(pos_type /* pos */ , mpt::byte_span dst) const override
	{
		return dst.first(0);
	}
};


class FileDataContainerWindow : public IFileDataContainer
{
private:
	std::shared_ptr<const IFileDataContainer> data;
	const pos_type dataOffset;
	const pos_type dataLength;
public:
	FileDataContainerWindow(std::shared_ptr<const IFileDataContainer> src, pos_type off, pos_type len) : data(src), dataOffset(off), dataLength(len) { }

	bool IsValid() const override
	{
		return data->IsValid();
	}
	bool HasFastGetLength() const override
	{
		return data->HasFastGetLength();
	}
	bool HasPinnedView() const override
	{
		return data->HasPinnedView();
	}
	const std::byte *GetRawData() const override
	{
		return data->GetRawData() + dataOffset;
	}
	pos_type GetLength() const override
	{
		return dataLength;
	}
	mpt::byte_span Read(pos_type pos, mpt::byte_span dst) const override
	{
		if(pos >= dataLength)
		{
			return dst.first(0);
		}
		return data->Read(dataOffset + pos, dst.first(std::min(dst.size(), dataLength - pos)));
	}
	bool CanRead(pos_type pos, std::size_t length) const override
	{
		if((pos == dataLength) && (length == 0))
		{
			return true;
		}
		if(pos >= dataLength)
		{
			return false;
		}
		return (length <= dataLength - pos);
	}
	pos_type GetReadableLength(pos_type pos, std::size_t length) const override
	{
		if(pos >= dataLength)
		{
			return 0;
		}
		return std::min(length, dataLength - pos);
	}
};


class FileDataContainerSeekable : public IFileDataContainer {

private:

	pos_type streamLength;

	mutable bool cached;
	mutable std::vector<std::byte> cache;

protected:

	FileDataContainerSeekable(pos_type length);

private:
	
	void CacheStream() const;

public:

	bool IsValid() const override;
	bool HasFastGetLength() const override;
	bool HasPinnedView() const override;
	const std::byte *GetRawData() const override;
	pos_type GetLength() const override;
	mpt::byte_span Read(pos_type pos, mpt::byte_span dst) const override;

private:

	virtual mpt::byte_span InternalReadSeekable(pos_type pos, mpt::byte_span dst) const = 0;

};


class FileDataContainerSeekableBuffered : public FileDataContainerSeekable {

private:

	enum : std::size_t {
		CHUNK_SIZE = mpt::IO::BUFFERSIZE_SMALL,
		BUFFER_SIZE = mpt::IO::BUFFERSIZE_NORMAL
	};
	enum : std::size_t {
		NUM_CHUNKS = BUFFER_SIZE / CHUNK_SIZE
	};
	struct chunk_info {
		pos_type ChunkOffset = 0;
		pos_type ChunkLength = 0;
		bool ChunkValid = false;
	};
	mutable std::vector<std::byte> m_Buffer = std::vector<std::byte>(BUFFER_SIZE);
	mpt::byte_span chunk_data(std::size_t chunkIndex) const
	{
		return mpt::byte_span(m_Buffer.data() + (chunkIndex * CHUNK_SIZE), CHUNK_SIZE);
	}
	mutable std::array<chunk_info, NUM_CHUNKS> m_ChunkInfo = {};
	mutable std::array<std::size_t, NUM_CHUNKS> m_ChunkIndexLRU = {};

	std::size_t InternalFillPageAndReturnIndex(pos_type pos) const;

protected:

	FileDataContainerSeekableBuffered(pos_type length);

private:

	mpt::byte_span InternalReadSeekable(pos_type pos, mpt::byte_span dst) const override;

	virtual mpt::byte_span InternalReadBuffered(pos_type pos, mpt::byte_span dst) const = 0;

};


class FileDataContainerStdStream
{

public:

	static bool IsSeekable(std::istream &stream);
	static IFileDataContainer::pos_type GetLength(std::istream &stream);

};


class FileDataContainerStdStreamSeekable : public FileDataContainerSeekableBuffered {

private:

	std::istream &stream;

public:

	FileDataContainerStdStreamSeekable(std::istream &s);

private:

	mpt::byte_span InternalReadBuffered(pos_type pos, mpt::byte_span dst) const override;

};


class FileDataContainerUnseekable : public IFileDataContainer {

private:

	mutable std::vector<std::byte> cache;
	mutable std::size_t cachesize;
	mutable bool streamFullyCached;

protected:

	FileDataContainerUnseekable();

private:

	enum : std::size_t {
		QUANTUM_SIZE = mpt::IO::BUFFERSIZE_SMALL,
		BUFFER_SIZE = mpt::IO::BUFFERSIZE_NORMAL
	};

	void EnsureCacheBuffer(std::size_t requiredbuffersize) const;
	void CacheStream() const;
	void CacheStreamUpTo(pos_type pos, pos_type length) const;

private:

	void ReadCached(pos_type pos, mpt::byte_span dst) const;

public:

	bool IsValid() const override;
	bool HasFastGetLength() const override;
	bool HasPinnedView() const override;
	const std::byte *GetRawData() const override;
	pos_type GetLength() const override;
	mpt::byte_span Read(pos_type pos, mpt::byte_span dst) const override;
	bool CanRead(pos_type pos, std::size_t length) const override;
	std::size_t GetReadableLength(pos_type pos, std::size_t length) const override;

private:

	virtual bool InternalEof() const = 0;
	virtual mpt::byte_span InternalReadUnseekable(mpt::byte_span dst) const = 0;

};


class FileDataContainerStdStreamUnseekable : public FileDataContainerUnseekable {

private:

	std::istream &stream;

public:

	FileDataContainerStdStreamUnseekable(std::istream &s);

private:

	bool InternalEof() const override;
	mpt::byte_span InternalReadUnseekable(mpt::byte_span dst) const override;

};



struct CallbackStream
{
	enum : int {
		SeekSet = 0,
		SeekCur = 1,
		SeekEnd = 2
	};
	void *stream;
	std::size_t (*read)( void * stream, void * dst, std::size_t bytes );
	int (*seek)( void * stream, int64 offset, int whence );
	int64 (*tell)( void * stream );
};


class FileDataContainerCallbackStreamSeekable : public FileDataContainerSeekable
{
private:
	CallbackStream stream;
public:
	FileDataContainerCallbackStreamSeekable(CallbackStream s);
private:
	mpt::byte_span InternalReadSeekable(pos_type pos, mpt::byte_span dst) const override;
};


class FileDataContainerCallbackStreamUnseekable : public FileDataContainerUnseekable
{
private:
	CallbackStream stream;
	mutable bool eof_reached;
public:
	FileDataContainerCallbackStreamUnseekable(CallbackStream s);
private:
	bool InternalEof() const override;
	mpt::byte_span InternalReadUnseekable(mpt::byte_span dst) const override;
};


class FileDataContainerCallbackStream
{
public:
	static bool IsSeekable(CallbackStream stream);
	static IFileDataContainer::pos_type GetLength(CallbackStream stream);
};


class FileDataContainerMemory
	: public IFileDataContainer
{

private:

	const std::byte *streamData;	// Pointer to memory-mapped file
	pos_type streamLength;		// Size of memory-mapped file in bytes

public:
	FileDataContainerMemory() : streamData(nullptr), streamLength(0) { }
	FileDataContainerMemory(mpt::const_byte_span data) : streamData(data.data()), streamLength(data.size()) { }

public:

	bool IsValid() const override
	{
		return streamData != nullptr;
	}

	bool HasFastGetLength() const override
	{
		return true;
	}

	bool HasPinnedView() const override
	{
		return true;
	}

	const std::byte *GetRawData() const override
	{
		return streamData;
	}

	pos_type GetLength() const override
	{
		return streamLength;
	}

	mpt::byte_span Read(pos_type pos, mpt::byte_span dst) const override
	{
		if(pos >= streamLength)
		{
			return dst.first(0);
		}
		pos_type avail = std::min(streamLength - pos, dst.size());
		std::copy(streamData + pos, streamData + pos + avail, dst.data());
		return dst.first(avail);
	}

	bool CanRead(pos_type pos, std::size_t length) const override
	{
		if((pos == streamLength) && (length == 0))
		{
			return true;
		}
		if(pos >= streamLength)
		{
			return false;
		}
		return (length <= streamLength - pos);
	}

	std::size_t GetReadableLength(pos_type pos, std::size_t length) const override
	{
		if(pos >= streamLength)
		{
			return 0;
		}
		return std::min(length, streamLength - pos);
	}

};



OPENMPT_NAMESPACE_END
