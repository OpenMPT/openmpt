/*
 * SampleFormatMP3.cpp
 * -------------------
 * Purpose: MP3 sample import.
 * Notes  :
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"
#ifndef MODPLUG_NO_FILESAVE
#include "../common/mptFileIO.h"
#endif
#include "../common/misc_util.h"
#include "Tagging.h"
#include "Loaders.h"
#include "ChunkReader.h"
#include "modsmp_ctrl.h"
#include "../soundbase/SampleFormatConverters.h"
#include "../soundbase/SampleFormatCopy.h"
#include "../soundlib/ModSampleCopy.h"
#include "../common/ComponentManager.h"
#ifdef MPT_ENABLE_MP3_SAMPLES
#include "MPEGFrame.h"
#endif // MPT_ENABLE_MP3_SAMPLES
#if defined(MPT_WITH_MINIMP3)
#include <minimp3/minimp3.h>
#endif // MPT_WITH_MINIMP3

// mpg123 must be last because of mpg123 large file support insanity
#if defined(MPT_WITH_MPG123)

#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>

#include <mpg123.h>

#endif


OPENMPT_NAMESPACE_BEGIN


///////////////////////////////////////////////////////////////////////////////////////////////////
// MP3 Samples

#if defined(MPT_WITH_MPG123)

#if MPT_COMPILER_MSVCCLANGC2
typedef _off_t mpg123_off_t;
#else // !MPT_COMPILER_MSVCCLANGC2
typedef off_t mpg123_off_t;
#endif // MPT_COMPILER_MSVCCLANGC2

typedef size_t mpg123_size_t;

typedef ssize_t mpg123_ssize_t;

class ComponentMPG123
#if defined(MPT_ENABLE_MPG123_DELAYLOAD)
	: public ComponentBundledDLL
#else
	: public ComponentBuiltin
#endif
{
	MPT_DECLARE_COMPONENT_MEMBERS

public:

	static mpg123_ssize_t FileReaderRead(void *fp, void *buf, mpg123_size_t count)
	{
		FileReader &file = *static_cast<FileReader *>(fp);
		size_t readBytes = std::min(count, static_cast<size_t>(file.BytesLeft()));
		file.ReadRaw(static_cast<char *>(buf), readBytes);
		return readBytes;
	}
	static mpg123_off_t FileReaderLSeek(void *fp, mpg123_off_t offset, int whence)
	{
		FileReader &file = *static_cast<FileReader *>(fp);
		FileReader::off_t oldpos = file.GetPosition();
		if(whence == SEEK_CUR) file.Seek(file.GetPosition() + offset);
		else if(whence == SEEK_END) file.Seek(file.GetLength() + offset);
		else file.Seek(offset);
		MPT_MAYBE_CONSTANT_IF(!Util::TypeCanHoldValue<mpg123_off_t>(file.GetPosition()))
		{
			file.Seek(oldpos);
			return static_cast<mpg123_off_t>(-1);
		}
		return static_cast<mpg123_off_t>(file.GetPosition());
	}

public:
	ComponentMPG123()
#if defined(MPT_ENABLE_MPG123_DELAYLOAD)
		: ComponentBundledDLL(MPT_PATHSTRING("openmpt-mpg123"))
#else
		: ComponentBuiltin()
#endif
	{
		return;
	}
	bool DoInitialize() override
	{
#if defined(MPT_ENABLE_MPG123_DELAYLOAD)
		if(!ComponentBundledDLL::DoInitialize())
		{
			return false;
		}
#endif
		if(mpg123_init() != 0)
		{
			return false;
		}
		return true;
	}
	virtual ~ComponentMPG123()
	{
		if(IsAvailable())
		{
			mpg123_exit();
		}
	}
};
MPT_REGISTERED_COMPONENT(ComponentMPG123, "Mpg123")

#endif // MPT_WITH_MPG123


bool CSoundFile::ReadMP3Sample(SAMPLEINDEX sample, FileReader &file, bool mo3Decode)
{
#if defined(MPT_WITH_MPG123) || defined(MPT_WITH_MINIMP3)

	// Check file for validity, or else mpg123 will happily munch many files that start looking vaguely resemble an MPEG stream mid-file.
	file.Rewind();
	while(file.CanRead(4))
	{
		uint8 magic[3];
		file.ReadArray(magic);

		if(!memcmp(magic, "ID3", 3))
		{
			// Skip ID3 tags
			uint8 header[7];
			file.ReadArray(header);

			uint32 size = 0;
			for(int i = 3; i < 7; i++)
			{
				if(header[i] & 0x80)
					return false;
				size = (size << 7) | header[i];
			}
			file.Skip(size);
		} else if(!memcmp(magic, "APE", 3) && file.ReadMagic("TAGEX"))
		{
			// Skip APE tags
			uint32 size = file.ReadUint32LE();
			file.Skip(16 + size);
		} else if(!memcmp(magic, "\x00\x00\x00", 3) || !memcmp(magic, "\xFF\x00\x00", 3))
		{
			// Some MP3 files are padded with zeroes...
		} else if(magic[0] == 0)
		{
			// This might be some padding, followed by an MPEG header, so try again.
			file.SkipBack(2);
		} else if(MPEGFrame::IsMPEGHeader(magic))
		{
			// This is what we want!
			break;
		} else
		{
			// This, on the other hand, isn't.
			return false;
		}
	}

#endif // MPT_WITH_MPG123 || MPT_WITH_MINIMP3

#if defined(MPT_WITH_MPG123)

	ComponentHandle<ComponentMPG123> mpg123;
	if(!IsComponentAvailable(mpg123))
	{
		return false;
	}

	struct MPG123Handle
	{
		mpg123_handle *mh;
		MPG123Handle() : mh(mpg123_new(0, nullptr)) { }
		~MPG123Handle() { mpg123_delete(mh); }
		operator mpg123_handle *() { return mh; }
	};
	MPG123Handle mh;
	if(!mh)
	{
		return false;
	}
	file.Rewind();

	long rate; int nchannels, encoding;
	SmpLength length;
	// Set up decoder...
	if(mpg123_param(mh, MPG123_ADD_FLAGS, MPG123_QUIET, 0.0))
	{
		return false;
	}
	if(mpg123_replace_reader_handle(mh, ComponentMPG123::FileReaderRead, ComponentMPG123::FileReaderLSeek, 0))
	{
		return false;
	}
	if(mpg123_open_handle(mh, &file))
	{
		return false;
	}
	if(mpg123_scan(mh))
	{
		return false;
	}
	if(mpg123_getformat(mh, &rate, &nchannels, &encoding))
	{
		return false;
	}
	if(!nchannels || nchannels > 2 || (encoding & (MPG123_ENC_16 | MPG123_ENC_SIGNED)) != (MPG123_ENC_16 | MPG123_ENC_SIGNED))
	{
		return false;
	}
	length = mpg123_length(mh);
	if(length == 0)
	{
		return false;
	}

	DestroySampleThreadsafe(sample);
	if(!mo3Decode)
	{
		strcpy(m_szNames[sample], "");
		Samples[sample].Initialize();
		Samples[sample].nC5Speed = rate;
	}
	Samples[sample].nLength = length;

	Samples[sample].uFlags.set(CHN_16BIT);
	Samples[sample].uFlags.set(CHN_STEREO, nchannels == 2);
	Samples[sample].AllocateSample();

	if(Samples[sample].pSample != nullptr)
	{
		mpg123_size_t ndecoded = 0;
		mpg123_read(mh, static_cast<unsigned char *>(Samples[sample].pSample), Samples[sample].GetSampleSizeInBytes(), &ndecoded);
	}

	if(!mo3Decode)
	{
		Samples[sample].Convert(MOD_TYPE_IT, GetType());
		Samples[sample].PrecomputeLoops(*this, false);
	}
	return Samples[sample].pSample != nullptr;

#elif defined(MPT_WITH_MINIMP3)

	file.Rewind();
	FileReader::PinnedRawDataView rawDataView = file.GetPinnedRawDataView();
	int64 bytes_left = rawDataView.size();
	const uint8 *stream_pos = mpt::byte_cast<const uint8 *>(rawDataView.data());

	std::vector<int16> raw_sample_data;

	mp3dec_t mp3;
	std::memset(&mp3, 0, sizeof(mp3dec_t));
	mp3dec_init(&mp3);
	
	int rate = 0;
	int channels = 0;

	mp3dec_frame_info_t info;
	std::memset(&info, 0, sizeof(mp3dec_frame_info_t));
	do
	{
		int16 sample_buf[MINIMP3_MAX_SAMPLES_PER_FRAME];
		int frame_samples = mp3dec_decode_frame(&mp3, stream_pos, mpt::saturate_cast<int>(bytes_left), sample_buf, &info);
		if(frame_samples < 0 || info.frame_bytes < 0) break; // internal error in minimp3
		if(frame_samples > 0 && info.frame_bytes == 0) break; // internal error in minimp3
		if(frame_samples == 0 && info.frame_bytes == 0) break; // end of stream, no progress
		if(frame_samples == 0 && info.frame_bytes > 0) MPT_DO { } MPT_WHILE_0; // decoder skipped non-mp3 data
		if(frame_samples > 0 && info.frame_bytes > 0) MPT_DO { } MPT_WHILE_0; // normal
		if(info.frame_bytes > 0)
		{
			if(rate != 0 && rate != info.hz) break; // inconsistent stream
			if(channels != 0 && channels != info.channels) break; // inconsistent stream
			rate = info.hz;
			channels = info.channels;
			if(rate <= 0) break; // broken stream
			if(channels != 1 && channels != 2) break; // broken stream
			stream_pos += mpt::clamp(info.frame_bytes, 0, mpt::saturate_cast<int>(bytes_left));
			bytes_left -= mpt::clamp(info.frame_bytes, 0, mpt::saturate_cast<int>(bytes_left));
			if(frame_samples > 0)
			{
				try
				{
					raw_sample_data.insert(raw_sample_data.end(), sample_buf, sample_buf + frame_samples * channels);
				} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
				{
					MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
					break;
				}
			}
		}
	} while(bytes_left > 0);

	if(rate == 0 || channels == 0 || raw_sample_data.empty())
	{
		return false;
	}

	DestroySampleThreadsafe(sample);
	if(!mo3Decode)
	{
		strcpy(m_szNames[sample], "");
		Samples[sample].Initialize();
		Samples[sample].nC5Speed = rate;
	}
	Samples[sample].nLength = mpt::saturate_cast<SmpLength>(raw_sample_data.size() / channels);

	Samples[sample].uFlags.set(CHN_16BIT);
	Samples[sample].uFlags.set(CHN_STEREO, channels == 2);
	Samples[sample].AllocateSample();

	if(Samples[sample].pSample != nullptr)
	{
		std::copy(raw_sample_data.begin(), raw_sample_data.end(), Samples[sample].pSample16);
	}

	if(!mo3Decode)
	{
		Samples[sample].Convert(MOD_TYPE_IT, GetType());
		Samples[sample].PrecomputeLoops(*this, false);
	}
	return Samples[sample].pSample != nullptr;

#else

	MPT_UNREFERENCED_PARAMETER(sample);
	MPT_UNREFERENCED_PARAMETER(file);
	MPT_UNREFERENCED_PARAMETER(mo3Decode);

#endif // MPT_WITH_MPG123 || MPT_WITH_MINIMP3

	return false;
}


bool CSoundFile::CanReadMP3()
{
	bool result = false;
	#if defined(MPT_WITH_MPG123)
		if(!result)
		{
			ComponentHandle<ComponentMPG123> mpg123;
			if(IsComponentAvailable(mpg123))
			{
				result = true;
			}
		}
	#endif
	#if defined(MPT_WITH_MINIMP3)
		if(!result)
		{
			result = true;
		}
	#endif
	#if defined(MPT_WITH_MEDIAFOUNDATION)
		if(!result)
		{
			if(CanReadMediaFoundation())
			{
				result = true;
			}
		}
	#endif
	return result;
}


OPENMPT_NAMESPACE_END
