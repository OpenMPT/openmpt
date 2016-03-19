/*
 * unlha.cpp
 * ---------
 * Purpose: Implementation file for extracting modules from .lha archives, making use of lhasa
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "unlha.h"

#ifdef MPT_WITH_LHASA
#include "lhasa.h"
#endif // MPT_WITH_LHASA


OPENMPT_NAMESPACE_BEGIN


#ifdef MPT_WITH_LHASA


static int LHAreadFileReader(void *handle, void *buf, size_t buf_len)
{
	FileReader *f = reinterpret_cast<FileReader*>(handle);
	int read_len = mpt::saturate_cast<int>(buf_len);
	int result = mpt::saturate_cast<int>(f->ReadRaw(mpt::void_cast<mpt::byte*>(buf), read_len));
	if(result == 0)
	{
		return -1;
	}
	return result;
}

static int LHAskipFileReader(void *handle, size_t bytes)
{
	FileReader *f = reinterpret_cast<FileReader*>(handle);
	if(f->CanRead(bytes))
	{
		f->Skip(bytes);
		return 1;
	}
	return 0;
}

static void LHAcloseFileReader(void * /*handle*/)
{
	return;
}

static LHAInputStreamType vtable =
{
	LHAreadFileReader,
	LHAskipFileReader,
	LHAcloseFileReader
};


CLhaArchive::CLhaArchive(FileReader &file) : ArchiveBase(file), inputstream(nullptr), reader(nullptr), firstfile(nullptr)
//-----------------------------------------------------------------------------------------------------------------------
{
	OpenArchive();
	for(LHAFileHeader *fileheader = firstfile; fileheader; fileheader = lha_reader_next_file(reader))
	{
		ArchiveFileInfo info;
		info.name = mpt::PathString::FromWide(mpt::ToWide(mpt::CharsetISO8859_1, fileheader->filename));
		info.size = fileheader->length;
		info.type = ArchiveFileNormal;
		contents.push_back(info);
	}
	CloseArchive();
}


CLhaArchive::~CLhaArchive()
//-------------------------
{
	return;
}


void CLhaArchive::OpenArchive()
//-----------------------------
{
	inFile.Rewind();
	inputstream = lha_input_stream_new(&vtable, &inFile);
	if(inputstream)
	{
		reader = lha_reader_new(inputstream);
	}
	if(reader)
	{
		lha_reader_set_dir_policy(reader, LHA_READER_DIR_END_OF_DIR);
		firstfile = lha_reader_next_file(reader);
	}
}


void CLhaArchive::CloseArchive()
//------------------------------
{
	if(reader)
	{
		lha_reader_free(reader);
		reader = nullptr;
	}
	if(inputstream)
	{
		lha_input_stream_free(inputstream);
		inputstream = nullptr;
	}
}


bool CLhaArchive::ExtractFile(std::size_t index)
//----------------------------------------------
{
	if(index >= contents.size())
	{
		return false;
	}
	data.clear();
	OpenArchive();
	const std::size_t bufSize = 4096;
	std::size_t i = 0;
	for(LHAFileHeader *fileheader = firstfile; fileheader; fileheader = lha_reader_next_file(reader))
	{
		if(index == i)
		{
			data.clear();
			std::size_t countRead = 0;
			do
			{
				try
				{
					data.resize(data.size() + bufSize);
				} catch(...)
				{
					CloseArchive();
					return false;
				}
				countRead = lha_reader_read(reader, &data[data.size() - bufSize], bufSize);
				if(countRead < bufSize)
				{
					try
					{
						data.resize(data.size() - (bufSize - countRead));
					} catch(...)
					{
						CloseArchive();
						return false;
					}
				}
			} while(countRead > 0);
		}
		++i;
	}
	CloseArchive();
	return data.size() > 0;
}


#endif // MPT_WITH_LHASA


OPENMPT_NAMESPACE_END
