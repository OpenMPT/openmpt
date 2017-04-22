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
extern "C" {
#include <minimp3/minimp3.h>
}
#endif // MPT_WITH_MINIMP3

#if defined(MPT_WITH_MPG123) || defined(MPT_ENABLE_MPG123_DYNBIND)
#define MPT_MPG123_LATEST_KNOWN_API_VERSION 43
#endif

// mpg123 must be last because of mpg123 largfile support insanity
#if defined(MPT_WITH_MPG123)

#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>

#include <mpg123.h>
#if (MPT_COMPILER_MSVC || MPT_COMPILER_MSVCCLANGC2) && defined(MPT_WITH_MPG123)
// MSVC builds use mpg123.h.in which does not provide the autoconf symbol @API_VERSION@, which in turn causes MPG123_API_VERSION to be bogus.
#undef MPG123_API_VERSION
#define MPG123_API_VERSION MPT_MPG123_LATEST_KNOWN_API_VERSION
#endif

// check for libmpg123 version

#if !defined(MPG123_API_VERSION)
#error "libmpg123 API version unknown. Assuming too old."
#else

#if (MPG123_API_VERSION < 29)
#error "libmpg123 API version too old."

#else // modern
// OK

#endif // MPG123_API_VERSION
#endif // MPG123_API_VERSION

#elif defined(MPT_ENABLE_MPG123_DYNBIND)

#include <stddef.h>

#endif


OPENMPT_NAMESPACE_BEGIN


///////////////////////////////////////////////////////////////////////////////////////////////////
// MP3 Samples

#if defined(MPT_WITH_MPG123) || defined(MPT_ENABLE_MPG123_DYNBIND)

#if MPT_COMPILER_MSVCCLANGC2
typedef _off_t mpg123_off_t;
#else // !MPT_COMPILER_MSVCCLANGC2
typedef off_t mpg123_off_t;
#endif // MPT_COMPILER_MSVCCLANGC2

typedef size_t mpg123_size_t;

#if defined(MPT_WITH_MPG123)
typedef ssize_t mpg123_ssize_t;
#elif defined(MPT_ENABLE_MPG123_DYNBIND)
#if MPT_COMPILER_MSVC || MPT_COMPILER_MSVCCLANGC2
typedef ptrdiff_t mpg123_ssize_t;
#else // MPT_COMPILER
typedef ssize_t mpg123_ssize_t;
#endif // MPT_COMPILER
#endif

#if !defined(MPT_WITH_MPG123) && defined(MPT_ENABLE_MPG123_DYNBIND)

	enum mpg123_parms
	{
		MPG123_VERBOSE = 0,
		MPG123_FLAGS,
		MPG123_ADD_FLAGS,
		MPG123_FORCE_RATE,
		MPG123_DOWN_SAMPLE,
		MPG123_RVA,
		MPG123_DOWNSPEED,
		MPG123_UPSPEED,
		MPG123_START_FRAME,
		MPG123_DECODE_FRAMES,
		MPG123_ICY_INTERVAL,
		MPG123_OUTSCALE,
		MPG123_TIMEOUT,
		MPG123_REMOVE_FLAGS,
		MPG123_RESYNC_LIMIT,
		MPG123_INDEX_SIZE,
		MPG123_PREFRAMES,
		MPG123_FEEDPOOL,
		MPG123_FEEDBUFFER
	};

	enum mpg123_param_flags
	{
		MPG123_QUIET = 0x20
	};

	enum mpg123_enc_enum
	{
		MPG123_ENC_16 = 0x040, MPG123_ENC_SIGNED = 0x080
	};

	typedef struct {int foo;} mpg123_handle;

#endif // !MPT_WITH_MPG123 && MPT_ENABLE_MPG123_DYNBIND

class ComponentMPG123
#if defined(MPT_WITH_MPG123)
	: public ComponentBuiltin
#elif defined(MPT_ENABLE_MPG123_DYNBIND)
	: public ComponentLibrary
#endif
{
	MPT_DECLARE_COMPONENT_MEMBERS

private:

#if MPT_OS_WINDOWS
#if defined(MPT_WITH_MPG123)
#elif defined(MPT_ENABLE_MPG123_DYNBIND)
	mpt::Library MSVCRT;
	mpt::Library SHLWAPI;
#endif // MPT_WITH_MPG123 || MPT_ENABLE_MPG123_DYNBIND
#endif // MPT_OS_WINDOWS

public:

	int (*mpg123_init )(void);
	void (*mpg123_exit )(void);
	mpg123_handle* (*mpg123_new )(const char*,int*);
	void (*mpg123_delete )(mpg123_handle*);
	int (*mpg123_param )(mpg123_handle*, enum mpg123_parms, long, double);
	int (*mpg123_open_handle )(mpg123_handle*, void*);
#if !defined(MPT_WITH_MPG123) && defined(MPT_ENABLE_MPG123_DYNBIND)
	int (*mpg123_open_handle_64 )(mpg123_handle*, void*);
#endif // !MPT_WITH_MPG123 && MPT_ENABLE_MPG123_DYNBIND
	int (*mpg123_replace_reader_handle)(mpg123_handle*,
		mpg123_ssize_t(*r_read)(void *, void *, mpg123_size_t),
		mpg123_off_t(*r_lseek)(void *, mpg123_off_t, int),
		void(*cleanup)(void *));
#if !defined(MPT_WITH_MPG123) && defined(MPT_ENABLE_MPG123_DYNBIND)
	int (*mpg123_replace_reader_handle_64)(mpg123_handle*,
		mpg123_ssize_t(*r_read)(void *, void *, mpg123_size_t),
		mpg123_off_t(*r_lseek)(void *, mpg123_off_t, int),
		void(*cleanup)(void *));
#endif // !MPT_WITH_MPG123 && MPT_ENABLE_MPG123_DYNBIND
	int (*mpg123_read )(mpg123_handle*, unsigned char*, mpg123_size_t, mpg123_size_t*);
	int (*mpg123_getformat )(mpg123_handle*, long*, int*, int*);
	int (*mpg123_scan )(mpg123_handle*);
	mpg123_off_t (*mpg123_length )(mpg123_handle*);
#if !defined(MPT_WITH_MPG123) && defined(MPT_ENABLE_MPG123_DYNBIND)
	mpg123_off_t (*mpg123_length_64 )(mpg123_handle*);
#endif // !MPT_WITH_MPG123 && MPT_ENABLE_MPG123_DYNBIND

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
		if(whence == SEEK_CUR) file.Seek(file.GetPosition() + offset);
		else if(whence == SEEK_END) file.Seek(file.GetLength() + offset);
		else file.Seek(offset);
		return file.GetPosition();
	}

public:
	ComponentMPG123()
#if defined(MPT_WITH_MPG123)
		: ComponentBuiltin()
#elif defined(MPT_ENABLE_MPG123_DYNBIND)
		: ComponentLibrary(ComponentTypeForeign)
#endif
	{
		return;
	}
	bool DoInitialize()
	{
#if defined(MPT_WITH_MPG123)
		MPT_GLOBAL_BIND("mpg123", mpg123_init);
		MPT_GLOBAL_BIND("mpg123", mpg123_exit);
		MPT_GLOBAL_BIND("mpg123", mpg123_new);
		MPT_GLOBAL_BIND("mpg123", mpg123_delete);
		MPT_GLOBAL_BIND("mpg123", mpg123_param);
		MPT_GLOBAL_BIND("mpg123", mpg123_open_handle);
		MPT_GLOBAL_BIND("mpg123", mpg123_replace_reader_handle);
		MPT_GLOBAL_BIND("mpg123", mpg123_read);
		MPT_GLOBAL_BIND("mpg123", mpg123_getformat);
		MPT_GLOBAL_BIND("mpg123", mpg123_scan);
		MPT_GLOBAL_BIND("mpg123", mpg123_length);
#elif defined(MPT_ENABLE_MPG123_DYNBIND)
		#if defined(MODPLUG_TRACKER)
			AddLibrary("mpg123", mpt::LibraryPath::AppFullName(MPT_PATHSTRING("openmpt-mpg123")));
		#endif // MODPLUG_TRACKER
		#if MPT_OS_WINDOWS
			// preload MSVCRT.DLL in order to prevent DLL preloading injection attacks from the current working directory,
			// because the stock binary packages of libmpg123 link against it
			MSVCRT = mpt::Library(mpt::LibraryPath::System(MPT_PATHSTRING("MSVCRT")));
			#if defined(LIBOPENMPT_BUILD)
				// require successful dependency loading for libopenmpt
				if(!MSVCRT.IsValid())
				{
					return false;
				}
			#endif // LIBOPENMPT_BUILD
			// preload shlwapi.dll for the same reasons
			if(!SHLWAPI.IsValid()) SHLWAPI = mpt::Library(mpt::LibraryPath::System(MPT_PATHSTRING("shlwapi")));
			#if defined(LIBOPENMPT_BUILD)
				// require successful dependency loading for libopenmpt
				if(!SHLWAPI.IsValid())
				{
					return false;
				}
			#endif // LIBOPENMPT_BUILD
		#endif // MPT_OS_WINDOWS
		#if defined(MODPLUG_TRACKER)
			AddLibrary("mpg123", mpt::LibraryPath::AppDataFullName(MPT_PATHSTRING("libmpg123-0"), GetComponentPath()));
			AddLibrary("mpg123", mpt::LibraryPath::AppDataFullName(MPT_PATHSTRING("libmpg123"), GetComponentPath()));
			AddLibrary("mpg123", mpt::LibraryPath::AppDataFullName(MPT_PATHSTRING("mpg123-0"), GetComponentPath()));
			AddLibrary("mpg123", mpt::LibraryPath::AppDataFullName(MPT_PATHSTRING("mpg123"), GetComponentPath()));
			AddLibrary("mpg123", mpt::LibraryPath::AppFullName(MPT_PATHSTRING("libmpg123-0")));
			AddLibrary("mpg123", mpt::LibraryPath::AppFullName(MPT_PATHSTRING("libmpg123")));
			AddLibrary("mpg123", mpt::LibraryPath::AppFullName(MPT_PATHSTRING("mpg123-0")));
			AddLibrary("mpg123", mpt::LibraryPath::AppFullName(MPT_PATHSTRING("mpg123")));
		#elif defined(LIBOPENMPT_BUILD)
			#if MPT_OS_WINDOWS
				// libopenmpt on Windows only loads the official builds
				AddLibrary("mpg123", mpt::LibraryPath::AppFullName(MPT_PATHSTRING("libmpg123-0")));
			#else // !MPT_OS_WINDOWS
				AddLibrary("mpg123", mpt::LibraryPath::System(MPT_PATHSTRING("mpg123")));
			#endif // MPT_OS_WINDOWS
		#endif
		MPT_COMPONENT_BIND("mpg123", mpg123_init);
		MPT_COMPONENT_BIND("mpg123", mpg123_exit);
		MPT_COMPONENT_BIND("mpg123", mpg123_new);
		MPT_COMPONENT_BIND("mpg123", mpg123_delete);
		MPT_COMPONENT_BIND("mpg123", mpg123_param);
		MPT_COMPONENT_BIND_OPTIONAL("mpg123", mpg123_open_handle);
		MPT_COMPONENT_BIND_OPTIONAL("mpg123", mpg123_open_handle_64);
		if(!mpg123_open_handle && !mpg123_open_handle_64)
		{
			return false;
		}
		MPT_COMPONENT_BIND_OPTIONAL("mpg123", mpg123_replace_reader_handle);
		MPT_COMPONENT_BIND_OPTIONAL("mpg123", mpg123_replace_reader_handle_64);
		if(!mpg123_replace_reader_handle && !mpg123_replace_reader_handle_64)
		{
			return false;
		}
		MPT_COMPONENT_BIND("mpg123", mpg123_read);
		MPT_COMPONENT_BIND("mpg123", mpg123_getformat);
		MPT_COMPONENT_BIND("mpg123", mpg123_scan);
		MPT_COMPONENT_BIND_OPTIONAL("mpg123", mpg123_length);
		MPT_COMPONENT_BIND_OPTIONAL("mpg123", mpg123_length_64);
		if(!mpg123_length && !mpg123_length_64)
		{
			return false;
		}
		#if MPT_COMPILER_MSVC
			if(!mpg123_open_handle || !mpg123_replace_reader_handle || !mpg123_length)
			{
				return false;
			}
		#endif
		if(HasBindFailed())
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

#endif // MPT_WITH_MPG123 || MPT_ENABLE_MPG123_DYNBIND


bool CSoundFile::ReadMP3Sample(SAMPLEINDEX sample, FileReader &file, bool mo3Decode)
//----------------------------------------------------------------------------------
{
#if defined(MPT_WITH_MPG123) || defined(MPT_ENABLE_MPG123_DYNBIND) || defined(MPT_WITH_MINIMP3)

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

#endif // MPT_WITH_MPG123 || MPT_ENABLE_MPG123_DYNBIND || MPT_WITH_MINIMP3

#if defined(MPT_WITH_MINIMP3)

	file.Rewind();
	FileReader::PinnedRawDataView rawDataView = file.GetPinnedRawDataView();
	int64 bytes_left = rawDataView.size();
	const uint8 *stream_pos = mpt::byte_cast<const uint8 *>(rawDataView.data());

	std::vector<int16> raw_sample_data;

	mp3_decoder_t *mp3 = reinterpret_cast<mp3_decoder_t *>(mp3_create()); // workaround minimp3 header typo

	int rate = 0;
	int channels = 0;

	mp3_info_t info;
	int frame_size = 0;
	do
	{
		int16 sample_buf[MP3_MAX_SAMPLES_PER_FRAME];
		frame_size = mp3_decode(mp3, const_cast<uint8 *>(stream_pos), bytes_left, sample_buf, &info); // workaround lack of const qualifier in mp3_decode (all internal functions have the required const correctness)
		if(rate != 0 && rate != info.sample_rate) break; // inconsistent stream
		if(channels != 0 && channels != info.channels) break; // inconsistent stream
		rate = info.sample_rate;
		channels = info.channels;
		if(rate <= 0) break; // broken stream
		if(channels != 1 && channels != 2) break; // broken stream
		stream_pos += frame_size;
		bytes_left -= frame_size;
		if(info.audio_bytes >= 0)
		{
			try
			{
				raw_sample_data.insert(raw_sample_data.end(), sample_buf, sample_buf + (info.audio_bytes / sizeof(int16)));
			} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
			{
				MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
				break;
			}
		}
	} while((bytes_left >= 0) && (frame_size > 0));

	mp3_free(mp3);

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
	Samples[sample].nLength = raw_sample_data.size() / channels;

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

#elif defined(MPT_WITH_MPG123) || defined(MPT_ENABLE_MPG123_DYNBIND)

	ComponentHandle<ComponentMPG123> mpg123;
	if(!IsComponentAvailable(mpg123))
	{
		return false;
	}

	mpg123_handle *mh;
	int err;
	if((mh = mpg123->mpg123_new(0, &err)) == nullptr) return false;
	file.Rewind();

	long rate; int nchannels, encoding;
	SmpLength length;
	// Set up decoder...
	if(mpg123->mpg123_param(mh, MPG123_ADD_FLAGS, MPG123_QUIET, 0.0))
	{
		mpg123->mpg123_delete(mh);
		return false;
	}
#if !defined(MPT_WITH_MPG123) && defined(MPT_ENABLE_MPG123_DYNBIND) && !MPT_COMPILER_MSVC
	if(mpg123->mpg123_replace_reader_handle_64 && mpg123->mpg123_replace_reader_handle_64(mh, ComponentMPG123::FileReaderRead, ComponentMPG123::FileReaderLSeek, 0))
	{
		mpg123->mpg123_delete(mh);
		return false;
	} else
#endif // !MPT_WITH_MPG123 && MPT_ENABLE_MPG123_DYNBIND
	if(mpg123->mpg123_replace_reader_handle(mh, ComponentMPG123::FileReaderRead, ComponentMPG123::FileReaderLSeek, 0))
	{
		mpg123->mpg123_delete(mh);
		return false;
	}
#if !defined(MPT_WITH_MPG123) && defined(MPT_ENABLE_MPG123_DYNBIND) && !MPT_COMPILER_MSVC
	if(mpg123->mpg123_open_handle_64 && mpg123->mpg123_open_handle_64(mh, &file))
	{
		mpg123->mpg123_delete(mh);
		return false;
	} else
#endif // !MPT_WITH_MPG123 && MPT_ENABLE_MPG123_DYNBIND
	if(mpg123->mpg123_open_handle(mh, &file))
	{
		mpg123->mpg123_delete(mh);
		return false;
	}
	if(mpg123->mpg123_scan(mh))
	{
		mpg123->mpg123_delete(mh);
		return false;
	}
	if(mpg123->mpg123_getformat(mh, &rate, &nchannels, &encoding))
	{
		mpg123->mpg123_delete(mh);
		return false;
	}
	if(!nchannels || nchannels > 2
		|| (encoding & (MPG123_ENC_16 | MPG123_ENC_SIGNED)) != (MPG123_ENC_16 | MPG123_ENC_SIGNED)
		)
	{
		mpg123->mpg123_delete(mh);
		return false;
	}
#if !defined(MPT_WITH_MPG123) && defined(MPT_ENABLE_MPG123_DYNBIND) && !MPT_COMPILER_MSVC
	if(mpg123->mpg123_length_64)
	{
		length = mpg123->mpg123_length_64(mh);
	} else
#endif // !MPT_WITH_MPG123 && MPT_ENABLE_MPG123_DYNBIND
	{
		length = mpg123->mpg123_length(mh);
	}
	if(length == 0)
	{
		mpg123->mpg123_delete(mh);
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
		mpg123->mpg123_read(mh, static_cast<unsigned char *>(Samples[sample].pSample), Samples[sample].GetSampleSizeInBytes(), &ndecoded);
	}
	mpg123->mpg123_delete(mh);

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

#endif // MPT_WITH_MPG123 || MPT_ENABLE_MPG123_DYNBIND || MPT_WITH_MINIMP3

	return false;
}


bool CSoundFile::CanReadMP3()
//---------------------------
{
	bool result = false;
	#if defined(MPT_WITH_MPG123)
		if(!result)
		{
			result = true;
		}
	#endif
	#if defined(MPT_WITH_MINIMP3)
		if(!result)
		{
			result = true;
		}
	#endif
	#if defined(MPT_ENABLE_MPG123_DYNBIND)
		if(!result)
		{
			ComponentHandle<ComponentMPG123> mpg123;
			if(IsComponentAvailable(mpg123))
			{
				result = true;
			}
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
