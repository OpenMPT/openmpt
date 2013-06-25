
#include "stdafx.h"

#include "unlha.h"

#include "lhasa.h"


static int LHAreadFileReader(void *handle, void *buf, size_t buf_len)
{
	FileReader *f = reinterpret_cast<FileReader*>(handle);
	int result = f->ReadRaw((char*)buf, buf_len);
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

static void LHAcloseFileReader(void * /*handle*/ )
{
	//FileReader *f = reinterpret_cast<FileReader*>(handle);
}

static LHAInputStreamType vtable = {
	LHAreadFileReader,
	LHAskipFileReader,
	LHAcloseFileReader
};

static inline std::string get_extension( std::string filename ) {
	if ( filename.find_last_of( "." ) != std::string::npos ) {
		return filename.substr( filename.find_last_of( "." ) + 1 );
	}
	return "";
}


CLhaArchive::CLhaArchive(FileReader file_ ) : file(file_), inputstream(nullptr), reader(nullptr), firstfile(nullptr)
//------------------------------------------------------------------------------------------------------------------
{
	inputstream = lha_input_stream_new(&vtable, &file);
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


CLhaArchive::~CLhaArchive()
//-------------------------
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


bool CLhaArchive::IsArchive()
//---------------------------
{
	return firstfile != nullptr;
}


bool CLhaArchive::ExtractFile()
//-----------------------------
{
	const std::size_t bufSize = 4096;
	for(LHAFileHeader *fileheader = firstfile; fileheader; fileheader = lha_reader_next_file(reader))
	{
		// get the biggest file
		if(fileheader->length >= data.size())
		{
			data.clear();
			std::size_t countRead = 0;
			do
			{
				data.resize(data.size() + bufSize);
				countRead = lha_reader_read(reader, &data[data.size() - bufSize], bufSize);
				if(countRead < bufSize)
				{
					data.resize(data.size() - (bufSize - countRead));
				}
			} while(countRead > 0);
		}
	}
	return data.size() > 0;
}


bool CLhaArchive::ExtractFile(const std::vector<const char *> &extensions)
//------------------------------------------------------------------------
{
	const std::size_t bufSize = 4096;
	for(LHAFileHeader *fileheader = firstfile; fileheader; fileheader = lha_reader_next_file(reader))
	{
		if(fileheader->filename)
		{
			std::string ext = get_extension(fileheader->filename);
			if(!ext.empty())
			{
				std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
				if(std::find(extensions.begin(), extensions.end(), ext) != extensions.end())
				{
					data.clear();
					std::size_t countRead = 0;
					do
					{
						data.resize(data.size() + bufSize);
						countRead = lha_reader_read(reader, &data[data.size() - bufSize], bufSize);
						if(countRead < bufSize)
						{
							data.resize(data.size() - (bufSize - countRead));
						}
					} while(countRead > 0);
					return true;
				}
			}
		}
	}
	return false;
}


FileReader CLhaArchive::GetOutputFile() const
//-------------------------------------------
{
	if(data.size() == 0)
	{
		return FileReader();
	}
	return FileReader(&data[0], data.size());
}
