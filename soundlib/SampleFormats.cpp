/*
 * SampleFormats.cpp
 * -----------------
 * Purpose: Code for loading various more or less common sample and instrument formats.
 * Notes  : Needs a lot of rewriting.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"
#include "mod_specifications.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/Moddoc.h"
#include "../mptrack/TrackerSettings.h"
#include "Dlsbank.h"
#endif //MODPLUG_TRACKER
#include "../soundlib/AudioCriticalSection.h"
#ifndef MODPLUG_NO_FILESAVE
#include "../common/mptFileIO.h"
#endif
#include "../common/misc_util.h"
#include "Tagging.h"
#include "ITTools.h"
#include "XMTools.h"
#include "S3MTools.h"
#include "WAVTools.h"
#include "../common/version.h"
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
//#include "../common/mptCRC.h"
#include "OggStream.h"
#ifdef MPT_WITH_OGG
#include <ogg/ogg.h>
#endif // MPT_WITH_OGG
#ifdef MPT_WITH_FLAC
#include <FLAC/stream_decoder.h>
#include <FLAC/stream_encoder.h>
#include <FLAC/metadata.h>
#endif // MPT_WITH_FLAC
#if defined(MPT_WITH_OPUSFILE)
#include <opusfile.h>
#endif
#if defined(MPT_WITH_VORBIS)
#include <vorbis/codec.h>
#endif
#if defined(MPT_WITH_VORBISFILE)
#include <vorbis/vorbisfile.h>
#endif
#ifdef MPT_WITH_STBVORBIS
#include <stb_vorbis/stb_vorbis.c>
#endif // MPT_WITH_STBVORBIS
#if defined(MPT_WITH_MINIMP3)
extern "C" {
#include <minimp3/minimp3.h>
}
#endif // MPT_WITH_MINIMP3
#if defined(MPT_WITH_MEDIAFOUNDATION)
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <Propvarutil.h>
#endif // MPT_WITH_MEDIAFOUNDATION

// mpg123 must be last because of mpg123 largfile support insanity
#if defined(MPT_WITH_MPG123)

#include <stdlib.h>
#include <sys/types.h>

#include <mpg123.h>

// check for utterly weird mpg123 largefile support

#if !defined(MPG123_API_VERSION)
#error "libmpg123 API version unknown. Assuming too old."
#else

#if (MPG123_API_VERSION < 11)
#error "libmpg123 API version too old."

#elif (MPG123_API_VERSION < 23)
// OK

#elif (MPG123_API_VERSION < 25) // < 1.12.0
#if MPT_COMPILER_MSVC
#pragma message("libmpg123 API version with split largefile support detected. This has not been tested at all.")
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG || MPT_COMPILER_MSVCCLANGC2
#warning "libmpg123 API version with split largefile support detected. This has not been tested at all."
#else
// There is no portable way to display a warning.
// Try to provoke a warning with an unused function.
static void OpenMPT_libmpg123_API_version_with_split_largefile_support_detected__This_has_not_been_tested_at_all_() { }
#endif

#elif (MPG123_API_VERSION == 25) // == 1.12.x and some later
// 1.12.2 introduced dumb wrappers for suffixed _32 and _64 functions.
// We cannot detect whether it's one of the broken versions 1.12.0 or 1.12.1,
// and thus have to implement a work-around for all of them (does not hurt though).
#if defined(MPT_ARCH_BITS)
#define MPT_LIBMPG123_WORKAROUND_LARGEFILE_SUFFIX
#else // !MPT_ARCH_BITS
#error message("libmpg123 API version largefile work-around requires MPT_ARCH_BITS to be defined. Please edit common/CompilerDetect.h and add support for your compiler.")
#endif // MPT_ARCH_BITS

#else // modern
// OK

#endif // MPG123_API_VERSION
#endif // MPG123_API_VERSION

#if defined(MPT_LIBMPG123_WORKAROUND_LARGEFILE_SUFFIX)

#if !MPT_COMPILER_MSVC
#if defined(_FILE_OFFSET_BITS) && !defined(MPG123_LARGESUFFIX)
#if defined(MPT_ARCH_BITS)
#if MPT_ARCH_BITS_64

// We are always compiling with _FILE_OFFSET_BITS==64, even on 64 bit platforms.
// libmpg123 does not provide the suffixed _64 versions in this case.
// Thus, on 64 bit, #undef all the wrapper macros.

extern "C" {

#ifdef mpg123_open
#undef mpg123_open
#endif
EXPORT int mpg123_open(mpg123_handle *mh, const char *path);

#ifdef mpg123_open_fd
#undef mpg123_open_fd
#endif
EXPORT int mpg123_open_fd(mpg123_handle *mh, int fd);

#ifdef mpg123_open_handle
#undef mpg123_open_handle
#endif
EXPORT int mpg123_open_handle(mpg123_handle *mh, void *iohandle);

#ifdef mpg123_framebyframe_decode
#undef mpg123_framebyframe_decode
#endif
EXPORT int mpg123_framebyframe_decode(mpg123_handle *mh, off_t *num, unsigned char **audio, size_t *bytes);

#ifdef mpg123_decode_frame
#undef mpg123_decode_frame
#endif
EXPORT int mpg123_decode_frame(mpg123_handle *mh, off_t *num, unsigned char **audio, size_t *bytes);

#ifdef mpg123_tell
#undef mpg123_tell
#endif
EXPORT off_t mpg123_tell(mpg123_handle *mh);

#ifdef mpg123_tellframe
#undef mpg123_tellframe
#endif
EXPORT off_t mpg123_tellframe(mpg123_handle *mh);

#ifdef mpg123_tell_stream
#undef mpg123_tell_stream
#endif
EXPORT off_t mpg123_tell_stream(mpg123_handle *mh);

#ifdef mpg123_seek
#undef mpg123_seek
#endif
EXPORT off_t mpg123_seek(mpg123_handle *mh, off_t sampleoff, int whence);

#ifdef mpg123_feedseek
#undef mpg123_feedseek
#endif
EXPORT off_t mpg123_feedseek(mpg123_handle *mh, off_t sampleoff, int whence, off_t *input_offset);

#ifdef mpg123_seek_frame
#undef mpg123_seek_frame
#endif
EXPORT off_t mpg123_seek_frame(mpg123_handle *mh, off_t frameoff, int whence);

#ifdef mpg123_timeframe
#undef mpg123_timeframe
#endif
EXPORT off_t mpg123_timeframe(mpg123_handle *mh, double sec);

#ifdef mpg123_index
#undef mpg123_index
#endif
EXPORT int mpg123_index(mpg123_handle *mh, off_t **offsets, off_t *step, size_t *fill);

#ifdef mpg123_set_index
#undef mpg123_set_index
#endif
EXPORT int mpg123_set_index(mpg123_handle *mh, off_t *offsets, off_t step, size_t fill);

#ifdef mpg123_position
#undef mpg123_position
#endif
EXPORT int mpg123_position( mpg123_handle *mh, off_t frame_offset, off_t buffered_bytes, off_t *current_frame, off_t *frames_left, double *current_seconds, double *seconds_left);

#ifdef mpg123_length
#undef mpg123_length
#endif
EXPORT off_t mpg123_length(mpg123_handle *mh);

#ifdef mpg123_set_filesize
#undef mpg123_set_filesize
#endif
EXPORT int mpg123_set_filesize(mpg123_handle *mh, off_t size);

#ifdef mpg123_replace_reader
#undef mpg123_replace_reader
#endif
EXPORT int mpg123_replace_reader(mpg123_handle *mh, ssize_t (*r_read) (int, void *, size_t), off_t (*r_lseek)(int, off_t, int));

#ifdef mpg123_replace_reader_handle
#undef mpg123_replace_reader_handle
#endif
EXPORT int mpg123_replace_reader_handle(mpg123_handle *mh, ssize_t (*r_read) (void *, void *, size_t), off_t (*r_lseek)(void *, off_t, int), void (*cleanup)(void*));

} // extern "C"

#endif
#endif
#endif
#endif

#endif // MPT_LIBMPG123_WORKAROUND_LARGEFILE_SUFFIX

#endif // MPT_WITH_MPG123


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_WITH_MEDIAFOUNDATION) || defined(MPT_WITH_OPUSFILE) || defined(MPT_WITH_VORBISFILE)

static mpt::ustring GetSampleNameFromTags(const FileTags &tags)
//-------------------------------------------------------------
{
	mpt::ustring result;
	if(tags.artist.empty())
	{
		result = tags.title;
	} else
	{
		result = mpt::String::Print(MPT_USTRING("%1 (by %2)"), tags.title, tags.artist);
	}
	return result;
}

#endif // MPT_WITH_MEDIAFOUNDATION || MPT_WITH_OPUSFILE || MPT_WITH_VORBISFILE


bool CSoundFile::ReadSampleFromFile(SAMPLEINDEX nSample, FileReader &file, bool mayNormalize, bool includeInstrumentFormats)
//--------------------------------------------------------------------------------------------------------------------------
{
	if(!nSample || nSample >= MAX_SAMPLES) return false;
	if(!ReadWAVSample(nSample, file, mayNormalize)
		&& !(includeInstrumentFormats && ReadXISample(nSample, file))
		&& !(includeInstrumentFormats && ReadITISample(nSample, file))
		&& !ReadAIFFSample(nSample, file, mayNormalize)
		&& !ReadITSSample(nSample, file)
		&& !(includeInstrumentFormats && ReadPATSample(nSample, file))
		&& !ReadIFFSample(nSample, file)
		&& !ReadS3ISample(nSample, file)
		&& !ReadAUSample(nSample, file, mayNormalize)
		&& !ReadFLACSample(nSample, file)
		&& !ReadOpusSample(nSample, file)
		&& !ReadVorbisSample(nSample, file)
		&& !ReadMP3Sample(nSample, file)
		&& !ReadMediaFoundationSample(nSample, file)
		)
	{
		return false;
	}

	if(nSample > GetNumSamples())
	{
		m_nSamples = nSample;
	}
	return true;
}


bool CSoundFile::ReadInstrumentFromFile(INSTRUMENTINDEX nInstr, FileReader &file, bool mayNormalize)
//--------------------------------------------------------------------------------------------------
{
	if ((!nInstr) || (nInstr >= MAX_INSTRUMENTS)) return false;
	if(!ReadITIInstrument(nInstr, file)
		&& !ReadXIInstrument(nInstr, file)
		&& !ReadPATInstrument(nInstr, file)
		&& !ReadSFZInstrument(nInstr, file)
		// Generic read
		&& !ReadSampleAsInstrument(nInstr, file, mayNormalize))
	{
		bool ok = false;
#ifdef MODPLUG_TRACKER
		CDLSBank bank;
		if(bank.Open(file))
		{
			ok = bank.ExtractInstrument(*this, nInstr, 0, 0);
		}
#endif // MODPLUG_TRACKER
		if(!ok) return false;
	}

	if(nInstr > GetNumInstruments()) m_nInstruments = nInstr;
	return true;
}


bool CSoundFile::ReadSampleAsInstrument(INSTRUMENTINDEX nInstr, FileReader &file, bool mayNormalize)
//--------------------------------------------------------------------------------------------------
{
	// Scanning free sample
	SAMPLEINDEX nSample = GetNextFreeSample(nInstr); // may also return samples which are only referenced by the current instrument
	if(nSample == SAMPLEINDEX_INVALID)
	{
		return false;
	}

	// Loading Instrument
	ModInstrument *pIns = new (std::nothrow) ModInstrument(nSample);
	if(pIns == nullptr)
	{
		return false;
	}
	if(!ReadSampleFromFile(nSample, file, mayNormalize, false))
	{
		delete pIns;
		return false;
	}

	// Remove all samples which are only referenced by the old instrument, except for the one we just loaded our new sample into.
	RemoveInstrumentSamples(nInstr, nSample);

	// Replace the instrument
	DestroyInstrument(nInstr, doNoDeleteAssociatedSamples);
	Instruments[nInstr] = pIns;

#if defined(MPT_ENABLE_FILEIO) && defined(MPT_EXTERNAL_SAMPLES)
	SetSamplePath(nSample, file.GetFileName());
#endif

	return true;
}


bool CSoundFile::DestroyInstrument(INSTRUMENTINDEX nInstr, deleteInstrumentSamples removeSamples)
//-----------------------------------------------------------------------------------------------
{
	if(nInstr == 0 || nInstr >= MAX_INSTRUMENTS || !Instruments[nInstr]) return true;

	if(removeSamples == deleteAssociatedSamples)
	{
		RemoveInstrumentSamples(nInstr);
	}

	CriticalSection cs;

	ModInstrument *pIns = Instruments[nInstr];
	Instruments[nInstr] = nullptr;
	for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
	{
		if (m_PlayState.Chn[i].pModInstrument == pIns)
		{
			m_PlayState.Chn[i].pModInstrument = nullptr;
		}
	}
	delete pIns;
	return true;
}


// Remove all unused samples from the given nInstr and keep keepSample if provided
bool CSoundFile::RemoveInstrumentSamples(INSTRUMENTINDEX nInstr, SAMPLEINDEX keepSample)
//--------------------------------------------------------------------------------------
{
	if(Instruments[nInstr] == nullptr)
	{
		return false;
	}

	std::vector<bool> keepSamples(GetNumSamples() + 1, true);

	// Check which samples are used by the instrument we are going to nuke.
	std::set<SAMPLEINDEX> referencedSamples = Instruments[nInstr]->GetSamples();
	for(auto sample = referencedSamples.cbegin(); sample != referencedSamples.cend(); sample++)
	{
		if((*sample) <= GetNumSamples())
		{
			keepSamples[*sample] = false;
		}
	}

	// If we want to keep a specific sample, do so.
	if(keepSample != SAMPLEINDEX_INVALID)
	{
		if(keepSample <= GetNumSamples())
		{
			keepSamples[keepSample] = true;
		}
	}

	// Check if any of those samples are referenced by other instruments as well, in which case we want to keep them of course.
	for(INSTRUMENTINDEX nIns = 1; nIns <= GetNumInstruments(); nIns++) if (Instruments[nIns] != nullptr && nIns != nInstr)
	{
		Instruments[nIns]->GetSamples(keepSamples);
	}

	// Now nuke the selected samples.
	RemoveSelectedSamples(keepSamples);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// I/O From another song
//

bool CSoundFile::ReadInstrumentFromSong(INSTRUMENTINDEX targetInstr, const CSoundFile &srcSong, INSTRUMENTINDEX sourceInstr)
//--------------------------------------------------------------------------------------------------------------------------
{
	if ((!sourceInstr) || (sourceInstr > srcSong.GetNumInstruments())
		|| (targetInstr >= MAX_INSTRUMENTS) || (!srcSong.Instruments[sourceInstr]))
	{
		return false;
	}
	if (m_nInstruments < targetInstr) m_nInstruments = targetInstr;

	ModInstrument *pIns = new (std::nothrow) ModInstrument();
	if(pIns == nullptr)
	{
		return false;
	}

	DestroyInstrument(targetInstr, deleteAssociatedSamples);

	Instruments[targetInstr] = pIns;
	*pIns = *srcSong.Instruments[sourceInstr];

	std::vector<SAMPLEINDEX> sourceSample;	// Sample index in source song
	std::vector<SAMPLEINDEX> targetSample;	// Sample index in target song
	SAMPLEINDEX targetIndex = 0;		// Next index for inserting sample

	for(size_t i = 0; i < CountOf(pIns->Keyboard); i++)
	{
		const SAMPLEINDEX sourceIndex = pIns->Keyboard[i];
		if(sourceIndex > 0 && sourceIndex <= srcSong.GetNumSamples())
		{
			const auto entry = std::find(sourceSample.cbegin(), sourceSample.cend(), sourceIndex);
			if(entry == sourceSample.end())
			{
				// Didn't consider this sample yet, so add it to our map.
				targetIndex = GetNextFreeSample(targetInstr, targetIndex + 1);
				if(targetIndex <= GetModSpecifications().samplesMax)
				{
					sourceSample.push_back(sourceIndex);
					targetSample.push_back(targetIndex);
					pIns->Keyboard[i] = targetIndex;
				} else
				{
					pIns->Keyboard[i] = 0;
				}
			} else
			{
				// Sample reference has already been created, so only need to update the sample map.
				pIns->Keyboard[i] = *(entry - sourceSample.begin() + targetSample.begin());
			}
		} else
		{
			// Invalid or no source sample
			pIns->Keyboard[i] = 0;
		}
	}

	pIns->Convert(srcSong.GetType(), GetType());

	// Copy all referenced samples over
	for(size_t i = 0; i < targetSample.size(); i++)
	{
		ReadSampleFromSong(targetSample[i], srcSong, sourceSample[i]);
	}

	return true;
}


bool CSoundFile::ReadSampleFromSong(SAMPLEINDEX targetSample, const CSoundFile &srcSong, SAMPLEINDEX sourceSample)
//----------------------------------------------------------------------------------------------------------------
{
	if(!sourceSample
		|| sourceSample > srcSong.GetNumSamples()
		|| (targetSample >= GetModSpecifications().samplesMax && targetSample > GetNumSamples()))
	{
		return false;
	}

	DestroySampleThreadsafe(targetSample);

	const ModSample &sourceSmp = srcSong.GetSample(sourceSample);

	if(GetNumSamples() < targetSample) m_nSamples = targetSample;
	Samples[targetSample] = sourceSmp;
	Samples[targetSample].Convert(srcSong.GetType(), GetType());
	strcpy(m_szNames[targetSample], srcSong.m_szNames[sourceSample]);

	if(sourceSmp.pSample)
	{
		Samples[targetSample].pSample = nullptr;	// Don't want to delete the original sample!
		if(Samples[targetSample].AllocateSample())
		{
			SmpLength nSize = sourceSmp.GetSampleSizeInBytes();
			memcpy(Samples[targetSample].pSample, sourceSmp.pSample, nSize);
			Samples[targetSample].PrecomputeLoops(*this, false);
		}
		// Remember on-disk path (for MPTM files), but don't implicitely enable on-disk storage
		// (we really don't want this for e.g. duplicating samples or splitting stereo samples)
#ifdef MPT_EXTERNAL_SAMPLES
		SetSamplePath(targetSample, srcSong.GetSamplePath(sourceSample));
#endif
		Samples[targetSample].uFlags.reset(SMP_KEEPONDISK);
	}

	return true;
}


////////////////////////////////////////////////////////////////////////
// IMA ADPCM Support for WAV files


static bool IMAADPCMUnpack16(int16 *target, SmpLength sampleLen, FileReader file, uint16 blockAlign, uint32 numChannels)
//----------------------------------------------------------------------------------------------------------------------
{
	static const int32 IMAIndexTab[8] =  { -1, -1, -1, -1, 2, 4, 6, 8 };
	static const int32 IMAUnpackTable[90] =
	{
		7,     8,     9,     10,    11,    12,    13,    14,
		16,    17,    19,    21,    23,    25,    28,    31,
		34,    37,    41,    45,    50,    55,    60,    66,
		73,    80,    88,    97,    107,   118,   130,   143,
		157,   173,   190,   209,   230,   253,   279,   307,
		337,   371,   408,   449,   494,   544,   598,   658,
		724,   796,   876,   963,   1060,  1166,  1282,  1411,
		1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
		3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
		7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
		15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
		32767, 0
	};

	if(target == nullptr || blockAlign < 4u * numChannels)
		return false;

	SmpLength samplePos = 0;
	sampleLen *= numChannels;
	while(file.CanRead(4u * numChannels) && samplePos < sampleLen)
	{
		FileReader block = file.ReadChunk(blockAlign);
		FileReader::PinnedRawDataView blockView = block.GetPinnedRawDataView();
		const mpt::byte *data = blockView.data();
		const uint32 blockSize = static_cast<uint32>(blockView.size());

		for(uint32 chn = 0; chn < numChannels; chn++)
		{
			// Block header
			int32 value = block.ReadInt16LE();
			int32 nIndex = block.ReadUint8();
			Limit(nIndex, 0, 89);
			block.Skip(1);

			SmpLength smpPos = samplePos + chn;
			uint32 dataPos = (numChannels + chn) * 4;
			// Block data
			while(smpPos <= (sampleLen - 8) && dataPos <= (blockSize - 4))
			{
				for(uint32 i = 0; i < 8; i++)
				{
					uint8 delta = data[dataPos];
					if(i & 1)
					{
						delta >>= 4;
						dataPos++;
					} else
					{
						delta &= 0x0F;
					}
					int32 v = IMAUnpackTable[nIndex] >> 3;
					if (delta & 1) v += IMAUnpackTable[nIndex] >> 2;
					if (delta & 2) v += IMAUnpackTable[nIndex] >> 1;
					if (delta & 4) v += IMAUnpackTable[nIndex];
					if (delta & 8) value -= v; else value += v;
					nIndex += IMAIndexTab[delta & 7];
					Limit(nIndex, 0, 88);
					Limit(value, -32768, 32767);
					target[smpPos] = static_cast<int16>(value);
					smpPos += numChannels;
				}
				dataPos += (numChannels - 1) * 4u;
			}
		}
		samplePos += ((blockSize - (numChannels * 4u)) * 2u);
	}

	return true;
}


////////////////////////////////////////////////////////////////////////////////
// WAV Open

bool CSoundFile::ReadWAVSample(SAMPLEINDEX nSample, FileReader &file, bool mayNormalize, FileReader *wsmpChunk)
//-------------------------------------------------------------------------------------------------------------
{
	WAVReader wavFile(file);

	if(!wavFile.IsValid()
		|| wavFile.GetNumChannels() == 0
		|| wavFile.GetNumChannels() > 2
		|| (wavFile.GetBitsPerSample() == 0 && wavFile.GetSampleFormat() != WAVFormatChunk::fmtMP3)
		|| (wavFile.GetBitsPerSample() > 64)
		|| (wavFile.GetSampleFormat() != WAVFormatChunk::fmtPCM && wavFile.GetSampleFormat() != WAVFormatChunk::fmtFloat && wavFile.GetSampleFormat() != WAVFormatChunk::fmtIMA_ADPCM && wavFile.GetSampleFormat() != WAVFormatChunk::fmtMP3 && wavFile.GetSampleFormat() != WAVFormatChunk::fmtALaw && wavFile.GetSampleFormat() != WAVFormatChunk::fmtULaw))
	{
		return false;
	}

	DestroySampleThreadsafe(nSample);
	strcpy(m_szNames[nSample], "");
	ModSample &sample = Samples[nSample];
	sample.Initialize();
	sample.nLength = wavFile.GetSampleLength();
	sample.nC5Speed = wavFile.GetSampleRate();
	wavFile.ApplySampleSettings(sample, m_szNames[nSample]);

	FileReader sampleChunk = wavFile.GetSampleData();

	SampleIO sampleIO(
		SampleIO::_8bit,
		(wavFile.GetNumChannels() > 1) ? SampleIO::stereoInterleaved : SampleIO::mono,
		SampleIO::littleEndian,
		SampleIO::signedPCM);

	if(wavFile.GetSampleFormat() == WAVFormatChunk::fmtIMA_ADPCM && wavFile.GetNumChannels() <= 2)
	{
		// IMA ADPCM 4:1
		LimitMax(sample.nLength, MAX_SAMPLE_LENGTH);
		sample.uFlags.set(CHN_16BIT);
		sample.uFlags.set(CHN_STEREO, wavFile.GetNumChannels() == 2);
		if(!sample.AllocateSample())
		{
			return false;
		}
		IMAADPCMUnpack16(sample.pSample16, sample.nLength, sampleChunk, wavFile.GetBlockAlign(), wavFile.GetNumChannels());
		sample.PrecomputeLoops(*this, false);
	} else if(wavFile.GetSampleFormat() == WAVFormatChunk::fmtMP3)
	{
		// MP3 in WAV
		return ReadMP3Sample(nSample, sampleChunk, true) || ReadMediaFoundationSample(nSample, sampleChunk, true);
	} else if(!wavFile.IsExtensibleFormat() && wavFile.MayBeCoolEdit16_8() && wavFile.GetSampleFormat() == WAVFormatChunk::fmtPCM && wavFile.GetBitsPerSample() == 32 && wavFile.GetBlockAlign() == wavFile.GetNumChannels() * 4)
	{
		// Syntrillium Cool Edit hack to store IEEE 32bit floating point
		// Format is described as 32bit integer PCM contained in 32bit blocks and an WAVEFORMATEX extension size of 2 which contains a single 16 bit little endian value of 1.
		//  (This is parsed in WAVTools.cpp and returned via MayBeCoolEdit16_8()).
		// The data actually stored in this case is little endian 32bit floating point PCM with 2**15 full scale.
		// Cool Edit calls this format "16.8 float".
		sampleIO |= SampleIO::_32bit;
		sampleIO |= SampleIO::floatPCM15;
		sampleIO.ReadSample(sample, sampleChunk);
	} else if(!wavFile.IsExtensibleFormat() && wavFile.GetSampleFormat() == WAVFormatChunk::fmtPCM && wavFile.GetBitsPerSample() == 24 && wavFile.GetBlockAlign() == wavFile.GetNumChannels() * 4)
	{
		// Syntrillium Cool Edit hack to store IEEE 32bit floating point
		// Format is described as 24bit integer PCM contained in 32bit blocks.
		// The data actually stored in this case is little endian 32bit floating point PCM with 2**23 full scale.
		// Cool Edit calls this format "24.0 float".
		sampleIO |= SampleIO::_32bit;
		sampleIO |= SampleIO::floatPCM23;
		sampleIO.ReadSample(sample, sampleChunk);
	} else if(wavFile.GetSampleFormat() == WAVFormatChunk::fmtALaw || wavFile.GetSampleFormat() == WAVFormatChunk::fmtULaw)
	{
		// a-law / u-law
		sampleIO |= SampleIO::_16bit;
		sampleIO |= wavFile.GetSampleFormat() == WAVFormatChunk::fmtALaw ? SampleIO::aLaw : SampleIO::uLaw;
		sampleIO.ReadSample(sample, sampleChunk);
	} else
	{
		// PCM / Float
		SampleIO::Bitdepth bitDepth;
		switch((wavFile.GetBitsPerSample() - 1) / 8u)
		{
		default:
		case 0: bitDepth = SampleIO::_8bit; break;
		case 1: bitDepth = SampleIO::_16bit; break;
		case 2: bitDepth = SampleIO::_24bit; break;
		case 3: bitDepth = SampleIO::_32bit; break;
		case 7: bitDepth = SampleIO::_64bit; break;
		}

		sampleIO |= bitDepth;
		if(wavFile.GetBitsPerSample() <= 8)
			sampleIO |= SampleIO::unsignedPCM;

		if(wavFile.GetSampleFormat() == WAVFormatChunk::fmtFloat)
			sampleIO |= SampleIO::floatPCM;

		if(mayNormalize)
			sampleIO.MayNormalize();

		sampleIO.ReadSample(sample, sampleChunk);
	}

	if(wsmpChunk != nullptr)
	{
		// DLS WSMP chunk
		*wsmpChunk = wavFile.GetWsmpChunk();
	}

	sample.Convert(MOD_TYPE_IT, GetType());
	sample.PrecomputeLoops(*this, false);

	return true;
}


///////////////////////////////////////////////////////////////
// Save WAV


#ifndef MODPLUG_NO_FILESAVE
bool CSoundFile::SaveWAVSample(SAMPLEINDEX nSample, const mpt::PathString &filename) const
//----------------------------------------------------------------------------------------
{
	mpt::ofstream f(filename, std::ios::binary);
	if(!f)
	{
		return false;
	}

	WAVWriter file(&f);

	if(!file.IsValid())
	{
		return false;
	}

	const ModSample &sample = Samples[nSample];
	file.WriteFormat(sample.GetSampleRate(GetType()), sample.GetElementarySampleSize() * 8, sample.GetNumChannels(), WAVFormatChunk::fmtPCM);

	// Write sample data
	file.StartChunk(RIFFChunk::iddata);
	file.Skip(SampleIO(
		sample.uFlags[CHN_16BIT] ? SampleIO::_16bit : SampleIO::_8bit,
		sample.uFlags[CHN_STEREO] ? SampleIO::stereoInterleaved : SampleIO::mono,
		SampleIO::littleEndian,
		sample.uFlags[CHN_16BIT] ? SampleIO::signedPCM : SampleIO::unsignedPCM)
		.WriteSample(f, sample));

	file.WriteLoopInformation(sample);
	file.WriteExtraInformation(sample, GetType());
	if(sample.HasCustomCuePoints())
	{
		file.WriteCueInformation(sample);
	}

	FileTags tags;
	tags.title = mpt::ToUnicode(GetCharsetLocaleOrModule(), m_szNames[nSample]);
	tags.encoder = mpt::ToUnicode(mpt::CharsetUTF8, MptVersion::GetOpenMPTVersionStr());
	file.WriteMetatags(tags);

	return true;
}

#endif // MODPLUG_NO_FILESAVE


#ifndef MODPLUG_NO_FILESAVE

///////////////////////////////////////////////////////////////
// Save RAW

bool CSoundFile::SaveRAWSample(SAMPLEINDEX nSample, const mpt::PathString &filename) const
//----------------------------------------------------------------------------------------
{
	mpt::ofstream f(filename, std::ios::binary);
	if(!f)
	{
		return false;
	}

	const ModSample &sample = Samples[nSample];
	SampleIO(
		sample.uFlags[CHN_16BIT] ? SampleIO::_16bit : SampleIO::_8bit,
		sample.uFlags[CHN_STEREO] ? SampleIO::stereoInterleaved : SampleIO::mono,
		SampleIO::littleEndian,
		SampleIO::signedPCM)
		.WriteSample(f, sample);

	return true;
}

#endif // MODPLUG_NO_FILESAVE

/////////////////////////////////////////////////////////////
// GUS Patches

struct GF1PatchFileHeader
{
	char     magic[8];		// "GF1PATCH"
	char     version[4];	// "100", or "110"
	char     id[10];		// "ID#000002"
	char     copyright[60];	// Copyright
	uint8le  numInstr;		// Number of instruments in patch
	uint8le  voices;		// Number of voices, usually 14
	uint8le  channels;		// Number of wav channels that can be played concurently to the patch
	uint16le numSamples;	// Total number of waveforms for all the .PAT
	uint16le volume;		// Master volume
	uint32le dataSize;
	char     reserved2[36];
};

MPT_BINARY_STRUCT(GF1PatchFileHeader, 129)


struct GF1Instrument
{
	uint16le id;			// Instrument id: 0-65535
	char     name[16];		// Name of instrument. Gravis doesn't seem to use it
	uint32le size;			// Number of bytes for the instrument with header. (To skip to next instrument)
	uint8    layers;		// Number of layers in instrument: 1-4
	char     reserved[40];
};

MPT_BINARY_STRUCT(GF1Instrument, 63)


struct GF1SampleHeader
{
	char     name[7];		// null terminated string. name of the wave.
	uint8le  fractions;		// Start loop point fraction in 4 bits + End loop point fraction in the 4 other bits.
	uint32le length;		// total size of wavesample. limited to 65535 now by the drivers, not the card.
	uint32le loopstart;		// start loop position in the wavesample
	uint32le loopend;		// end loop position in the wavesample
	uint16le freq;			// Rate at which the wavesample has been sampled
	uint32le low_freq, high_freq, root_freq;	// check note.h for the correspondance.
	int16le  finetune;		// fine tune. -512 to +512, EXCLUDING 0 cause it is a multiplier. 512 is one octave off, and 1 is a neutral value
	uint8le  balance;		// Balance: 0-15. 0=full left, 15 = full right
	uint8le  env_rate[6];	// attack rates
	uint8le  env_volume[6];	// attack volumes
	uint8le  tremolo_sweep, tremolo_rate, tremolo_depth;
	uint8le  vibrato_sweep, vibrato_rate, vibrato_depth;
	uint8le  flags;
	int16le  scale_frequency;	// Note
	uint16le scale_factor;		// 0...2048 (1024 is normal) or 0...2
	char     reserved[36];
};

MPT_BINARY_STRUCT(GF1SampleHeader, 96)

// -- GF1 Envelopes --
//
//	It can be represented like this (the envelope is totally bogus, it is
//	just to show the concept):
//
//	|
//	|           /----`               | |
//	|   /------/      `\         | | | | |
//	|  /                 \       | | | | |
//	| /                    \     | | | | |
//	|/                       \   | | | | |
//	---------------------------- | | | | | |
//	<---> attack rate 0          0 1 2 3 4 5 amplitudes
//	     <----> attack rate 1
//		     <> attack rate 2
//			 <--> attack rate 3
//			     <> attack rate 4
//				 <-----> attack rate 5
//
// -- GF1 Flags --
//
// bit 0: 8/16 bit
// bit 1: Signed/Unsigned
// bit 2: off/on looping
// bit 3: off/on bidirectionnal looping
// bit 4: off/on backward looping
// bit 5: off/on sustaining (3rd point in env.)
// bit 6: off/on envelopes
// bit 7: off/on clamped release (6th point, env)


struct GF1Layer
{
	uint8le  previous;		// If !=0 the wavesample to use is from the previous layer. The waveheader is still needed
	uint8le  id;			// Layer id: 0-3
	uint32le size;			// data size in bytes in the layer, without the header. to skip to next layer for example:
	uint8le  samples;		// number of wavesamples
	char     reserved[40];
};

MPT_BINARY_STRUCT(GF1Layer, 47)


static int32 PatchFreqToNote(uint32 nFreq)
//----------------------------------------
{
	const float inv_log_2 = 1.44269504089f; // 1.0f/std::log(2.0f)
	const float base = 1.0f / 2044.0f;
	return Util::Round<int32>(std::log(nFreq * base) * (12.0f * inv_log_2));
}


static void PatchToSample(CSoundFile *that, SAMPLEINDEX nSample, GF1SampleHeader &sampleHeader, FileReader &file)
//---------------------------------------------------------------------------------------------------------------
{
	ModSample &sample = that->GetSample(nSample);

	file.ReadStruct(sampleHeader);

	sample.Initialize();
	if(sampleHeader.flags & 4) sample.uFlags.set(CHN_LOOP);
	if(sampleHeader.flags & 8) sample.uFlags.set(CHN_PINGPONGLOOP);
	sample.nLength = sampleHeader.length;
	sample.nLoopStart = sampleHeader.loopstart;
	sample.nLoopEnd = sampleHeader.loopend;
	sample.nC5Speed = sampleHeader.freq;
	sample.nPan = (sampleHeader.balance * 256 + 8) / 15;
	if(sample.nPan > 256) sample.nPan = 128;
	else sample.uFlags.set(CHN_PANNING);
	sample.nVibType = VIB_SINE;
	sample.nVibSweep = sampleHeader.vibrato_sweep;
	sample.nVibDepth = sampleHeader.vibrato_depth;
	sample.nVibRate = sampleHeader.vibrato_rate / 4;
	if(sampleHeader.scale_factor)
	{
		sample.FrequencyToTranspose();
		sample.RelativeTone += static_cast<int8>(84 - PatchFreqToNote(sampleHeader.root_freq));
		sample.RelativeTone = static_cast<uint8>(sample.RelativeTone - (sampleHeader.scale_frequency - 60));
		sample.TransposeToFrequency();
	}

	SampleIO sampleIO(
		SampleIO::_8bit,
		SampleIO::mono,
		SampleIO::littleEndian,
		(sampleHeader.flags & 2) ? SampleIO::unsignedPCM : SampleIO::signedPCM);

	if(sampleHeader.flags & 1)
	{
		sampleIO |= SampleIO::_16bit;
		sample.nLength /= 2;
		sample.nLoopStart /= 2;
		sample.nLoopEnd /= 2;
	}
	sampleIO.ReadSample(sample, file);
	sample.Convert(MOD_TYPE_IT, that->GetType());
	sample.PrecomputeLoops(*that, false);

	mpt::String::Read<mpt::String::maybeNullTerminated>(that->m_szNames[nSample], sampleHeader.name);
}


bool CSoundFile::ReadPATSample(SAMPLEINDEX nSample, FileReader &file)
//-------------------------------------------------------------------
{
	file.Rewind();
	GF1PatchFileHeader fileHeader;
	GF1Instrument instrHeader;	// We only support one instrument
	GF1Layer layerHeader;
	if(!file.ReadStruct(fileHeader)
		|| memcmp(fileHeader.magic, "GF1PATCH", 8)
		|| (memcmp(fileHeader.version, "110\0", 4) && memcmp(fileHeader.version, "100\0", 4))
		|| memcmp(fileHeader.id, "ID#000002\0", 10)
		|| !fileHeader.numInstr || !fileHeader.numSamples
		|| !file.ReadStruct(instrHeader)
		//|| !instrHeader.layers	// DOO.PAT has 0 layers
		|| !file.ReadStruct(layerHeader)
		|| !layerHeader.samples)
	{
		return false;
	}

	DestroySampleThreadsafe(nSample);
	GF1SampleHeader sampleHeader;
	PatchToSample(this, nSample, sampleHeader, file);

	if(instrHeader.name[0] > ' ')
	{
		mpt::String::Read<mpt::String::maybeNullTerminated>(m_szNames[nSample], instrHeader.name);
	}
	return true;
}


// PAT Instrument
bool CSoundFile::ReadPATInstrument(INSTRUMENTINDEX nInstr, FileReader &file)
//--------------------------------------------------------------------------
{
	file.Rewind();
	GF1PatchFileHeader fileHeader;
	GF1Instrument instrHeader;	// We only support one instrument
	GF1Layer layerHeader;
	if(!file.ReadStruct(fileHeader)
		|| memcmp(fileHeader.magic, "GF1PATCH", 8)
		|| (memcmp(fileHeader.version, "110\0", 4) && memcmp(fileHeader.version, "100\0", 4))
		|| memcmp(fileHeader.id, "ID#000002\0", 10)
		|| !fileHeader.numInstr || !fileHeader.numSamples
		|| !file.ReadStruct(instrHeader)
		//|| !instrHeader.layers	// DOO.PAT has 0 layers
		|| !file.ReadStruct(layerHeader)
		|| !layerHeader.samples)
	{
		return false;
	}

	ModInstrument *pIns = new (std::nothrow) ModInstrument();
	if(pIns == nullptr)
	{
		return false;
	}

	DestroyInstrument(nInstr, deleteAssociatedSamples);
	if (nInstr > m_nInstruments) m_nInstruments = nInstr;
	Instruments[nInstr] = pIns;

	mpt::String::Read<mpt::String::maybeNullTerminated>(pIns->name, instrHeader.name);
	pIns->nFadeOut = 2048;
	if(GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))
	{
		pIns->nNNA = NNA_NOTEOFF;
		pIns->nDNA = DNA_NOTEFADE;
	}

	SAMPLEINDEX nextSample = 0;
	int32 nMinSmpNote = 0xFF;
	SAMPLEINDEX nMinSmp = 0;
	for(uint8 smp = 0; smp < layerHeader.samples; smp++)
	{
		// Find a free sample
		nextSample = GetNextFreeSample(nInstr, nextSample + 1);
		if(nextSample == SAMPLEINDEX_INVALID) break;
		if(m_nSamples < nextSample) m_nSamples = nextSample;
		if(!nMinSmp) nMinSmp = nextSample;
		// Load it
		GF1SampleHeader sampleHeader;
		PatchToSample(this, nextSample, sampleHeader, file);
		int32 nMinNote = (sampleHeader.low_freq > 100) ? PatchFreqToNote(sampleHeader.low_freq) : 0;
		int32 nMaxNote = (sampleHeader.high_freq > 100) ? PatchFreqToNote(sampleHeader.high_freq) : NOTE_MAX;
		int32 nBaseNote = (sampleHeader.root_freq > 100) ? PatchFreqToNote(sampleHeader.root_freq) : -1;
		if(!sampleHeader.scale_factor && layerHeader.samples == 1) { nMinNote = 0; nMaxNote = NOTE_MAX; }
		// Fill Note Map
		for(int32 k = 0; k < NOTE_MAX; k++)
		{
			if(k == nBaseNote || (!pIns->Keyboard[k] && k >= nMinNote && k <= nMaxNote))
			{
				if(!sampleHeader.scale_factor)
					pIns->NoteMap[k] = NOTE_MIDDLEC;

				pIns->Keyboard[k] = nextSample;
				if(k < nMinSmpNote)
				{
					nMinSmpNote = k;
					nMinSmp = nextSample;
				}
			}
		}
	}
	if(nMinSmp)
	{
		// Fill note map and missing samples
		for(uint8 k = 0; k < NOTE_MAX; k++)
		{
			if(!pIns->NoteMap[k]) pIns->NoteMap[k] = k + 1;
			if(!pIns->Keyboard[k])
			{
				pIns->Keyboard[k] = nMinSmp;
			} else
			{
				nMinSmp = pIns->Keyboard[k];
			}
		}
	}

	pIns->Sanitize(MOD_TYPE_IT);
	pIns->Convert(MOD_TYPE_IT, GetType());
	return true;
}


/////////////////////////////////////////////////////////////
// S3I Samples


bool CSoundFile::ReadS3ISample(SAMPLEINDEX nSample, FileReader &file)
//-------------------------------------------------------------------
{
	file.Rewind();

	S3MSampleHeader sampleHeader;
	if(!file.ReadStruct(sampleHeader)
		|| sampleHeader.sampleType != S3MSampleHeader::typePCM
		|| memcmp(sampleHeader.magic, "SCRS", 4)
		|| !file.Seek((sampleHeader.dataPointer[1] << 4) | (sampleHeader.dataPointer[2] << 12) | (sampleHeader.dataPointer[0] << 20)))
	{
		return false;
	}

	DestroySampleThreadsafe(nSample);

	ModSample &sample = Samples[nSample];
	sampleHeader.ConvertToMPT(sample);
	mpt::String::Read<mpt::String::nullTerminated>(m_szNames[nSample], sampleHeader.name);
	sampleHeader.GetSampleFormat(false).ReadSample(sample, file);
	sample.Convert(MOD_TYPE_S3M, GetType());
	sample.PrecomputeLoops(*this, false);
	return true;
}


/////////////////////////////////////////////////////////////
// XI Instruments


bool CSoundFile::ReadXIInstrument(INSTRUMENTINDEX nInstr, FileReader &file)
//-------------------------------------------------------------------------
{
	file.Rewind();

	XIInstrumentHeader fileHeader;
	if(!file.ReadStruct(fileHeader)
		|| memcmp(fileHeader.signature, "Extended Instrument: ", 21)
		|| fileHeader.version != XIInstrumentHeader::fileVersion
		|| fileHeader.eof != 0x1A)
	{
		return false;
	}

	ModInstrument *pIns = new (std::nothrow) ModInstrument();
	if(pIns == nullptr)
	{
		return false;
	}

	DestroyInstrument(nInstr, deleteAssociatedSamples);
	if(nInstr > m_nInstruments)
	{
		m_nInstruments = nInstr;
	}
	Instruments[nInstr] = pIns;

	fileHeader.ConvertToMPT(*pIns);

	// Translate sample map and find available sample slots
	std::vector<SAMPLEINDEX> sampleMap(fileHeader.numSamples);
	SAMPLEINDEX maxSmp = 0;

	for(size_t i = 0 + 12; i < 96 + 12; i++)
	{
		if(pIns->Keyboard[i] >= fileHeader.numSamples)
		{
			continue;
		}

		if(sampleMap[pIns->Keyboard[i]] == 0)
		{
			// Find slot for this sample
			maxSmp = GetNextFreeSample(nInstr, maxSmp + 1);
			if(maxSmp != SAMPLEINDEX_INVALID)
			{
				sampleMap[pIns->Keyboard[i]] = maxSmp;
			}
		}
		pIns->Keyboard[i] = sampleMap[pIns->Keyboard[i]];
	}

	if(m_nSamples < maxSmp)
	{
		m_nSamples = maxSmp;
	}

	std::vector<SampleIO> sampleFlags(fileHeader.numSamples);

	// Read sample headers
	for(SAMPLEINDEX i = 0; i < fileHeader.numSamples; i++)
	{
		XMSample sampleHeader;
		if(!file.ReadStruct(sampleHeader)
			|| !sampleMap[i])
		{
			continue;
		}

		ModSample &mptSample = Samples[sampleMap[i]];
		sampleHeader.ConvertToMPT(mptSample);
		fileHeader.instrument.ApplyAutoVibratoToMPT(mptSample);
		mptSample.Convert(MOD_TYPE_XM, GetType());
		if(GetType() != MOD_TYPE_XM && fileHeader.numSamples == 1)
		{
			// No need to pan that single sample, thank you...
			mptSample.uFlags &= ~CHN_PANNING;
		}

		mpt::String::Read<mpt::String::spacePadded>(mptSample.filename, sampleHeader.name);
		mpt::String::Read<mpt::String::spacePadded>(m_szNames[sampleMap[i]], sampleHeader.name);

		sampleFlags[i] = sampleHeader.GetSampleFormat();
	}

	// Read sample data
	for(SAMPLEINDEX i = 0; i < fileHeader.numSamples; i++)
	{
		if(sampleMap[i])
		{
			sampleFlags[i].ReadSample(Samples[sampleMap[i]], file);
			Samples[sampleMap[i]].PrecomputeLoops(*this, false);
		}
	}

	pIns->Convert(MOD_TYPE_XM, GetType());

	// Read MPT crap
	ReadExtendedInstrumentProperties(pIns, file);
	pIns->Sanitize(GetType());
	return true;
}


#ifndef MODPLUG_NO_FILESAVE

bool CSoundFile::SaveXIInstrument(INSTRUMENTINDEX nInstr, const mpt::PathString &filename) const
//----------------------------------------------------------------------------------------------
{
	ModInstrument *pIns = Instruments[nInstr];
	if(pIns == nullptr || filename.empty())
	{
		return false;
	}

	FILE *f;
	if((f = mpt_fopen(filename, "wb")) == nullptr)
	{
		return false;
	}

	// Create file header
	XIInstrumentHeader header;
	header.ConvertToXM(*pIns, false);

	const std::vector<SAMPLEINDEX> samples = header.instrument.GetSampleList(*pIns, false);
	if(samples.size() > 0 && samples[0] <= GetNumSamples())
	{
		// Copy over auto-vibrato settings of first sample
		header.instrument.ApplyAutoVibratoToXM(Samples[samples[0]], GetType());
	}

	fwrite(&header, 1, sizeof(XIInstrumentHeader), f);

	std::vector<SampleIO> sampleFlags(samples.size());

	// XI Sample Headers
	for(SAMPLEINDEX i = 0; i < samples.size(); i++)
	{
		XMSample xmSample;
		if(samples[i] <= GetNumSamples())
		{
			xmSample.ConvertToXM(Samples[samples[i]], GetType(), false);
		} else
		{
			MemsetZero(xmSample);
		}
		sampleFlags[i] = xmSample.GetSampleFormat();

		mpt::String::Write<mpt::String::spacePadded>(xmSample.name, m_szNames[samples[i]]);

		fwrite(&xmSample, 1, sizeof(xmSample), f);
	}

	// XI Sample Data
	for(SAMPLEINDEX i = 0; i < samples.size(); i++)
	{
		if(samples[i] <= GetNumSamples())
		{
			sampleFlags[i].WriteSample(f, Samples[samples[i]]);
		}
	}

	// Write 'MPTX' extension tag
	char code[4];
	memcpy(code, "XTPM", 4);
	fwrite(code, 1, 4, f);
	WriteInstrumentHeaderStructOrField(pIns, f);	// Write full extended header.

	fclose(f);
	return true;
}

#endif // MODPLUG_NO_FILESAVE


// Read first sample from XI file into a sample slot
bool CSoundFile::ReadXISample(SAMPLEINDEX nSample, FileReader &file)
//------------------------------------------------------------------
{
	file.Rewind();

	XIInstrumentHeader fileHeader;
	if(!file.ReadStruct(fileHeader)
		|| !file.CanRead(sizeof(XMSample))
		|| memcmp(fileHeader.signature, "Extended Instrument: ", 21)
		|| fileHeader.version != XIInstrumentHeader::fileVersion
		|| fileHeader.eof != 0x1A
		|| fileHeader.numSamples == 0)
	{
		return false;
	}

	if(m_nSamples < nSample)
	{
		m_nSamples = nSample;
	}

	uint16 numSamples = fileHeader.numSamples;
	FileReader::off_t samplePos = sizeof(XIInstrumentHeader) + numSamples * sizeof(XMSample);
	// Preferrably read the middle-C sample
	auto sample = fileHeader.instrument.sampleMap[48];
	if(sample >= fileHeader.numSamples)
		sample = 0;
	XMSample sampleHeader;
	while(sample--)
	{
		file.ReadStruct(sampleHeader);
		samplePos += sampleHeader.length;
	}
	file.ReadStruct(sampleHeader);
	// Gotta skip 'em all!
	file.Seek(samplePos);

	DestroySampleThreadsafe(nSample);

	ModSample &mptSample = Samples[nSample];
	sampleHeader.ConvertToMPT(mptSample);
	if(GetType() != MOD_TYPE_XM)
	{
		// No need to pan that single sample, thank you...
		mptSample.uFlags.reset(CHN_PANNING);
	}
	fileHeader.instrument.ApplyAutoVibratoToMPT(mptSample);
	mptSample.Convert(MOD_TYPE_XM, GetType());

	mpt::String::Read<mpt::String::spacePadded>(mptSample.filename, sampleHeader.name);
	mpt::String::Read<mpt::String::spacePadded>(m_szNames[nSample], sampleHeader.name);

	// Read sample data
	sampleHeader.GetSampleFormat().ReadSample(Samples[nSample], file);
	Samples[nSample].PrecomputeLoops(*this, false);

	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////
// SFZ Instrument

struct SFZControl
{
	std::string defaultPath;
	int8 octaveOffset, noteOffset;

	SFZControl()
		: octaveOffset(0), noteOffset(0)
	{ }

	void Parse(const std::string &key, const std::string &value)
	{
		if(key == "default_path")
			defaultPath = value;
		else if(key == "octave_offset")
			octaveOffset = ConvertStrTo<int8>(value);
		else if(key == "note_offset")
			noteOffset = ConvertStrTo<int8>(value);
	}
};

struct SFZEnvelope
{
	float startLevel, delay, attack, hold, decay, sustainLevel, release, depth;
	SFZEnvelope()
		: startLevel(0), delay(0), attack(0), hold(0), decay(0), sustainLevel(100), release(0), depth(0)
	{ }

	void Parse(std::string key, const std::string &value)
	{
		key.erase(0, key.find('_') + 1);
		float v = ConvertStrTo<float>(value);
		if(key == "depth")
			Limit(v, -12000.0f, 12000.0f);
		else if(key == "start" || key == "sustain")
			Limit(v, -100.0f, 100.0f);
		else
			Limit(v, 0.0f, 100.0f);

		if(key == "start")
			startLevel = v;
		else if(key == "delay")
			delay = v;
		else if(key == "attack")
			attack = v;
		else if(key == "hold")
			hold = v;
		else if(key == "decay")
			decay = v;
		else if(key == "sustain")
			sustainLevel = v;
		else if(key == "release")
			release = v;
		else if(key == "depth")
			depth = v;
	}

	static EnvelopeNode::tick_t ToTicks(float duration, float tickDuration)
	{
		return std::max(EnvelopeNode::tick_t(1), Util::Round<EnvelopeNode::tick_t>(duration / tickDuration));
	}

	EnvelopeNode::value_t ToValue(float value, EnvelopeType envType) const
	{
		value *= (ENVELOPE_MAX / 100.0f);
		if(envType == ENV_PITCH)
		{
			value *= depth / 3200.0f;
			value += ENVELOPE_MID;
		}
		Limit<float, float>(value, ENVELOPE_MIN, ENVELOPE_MAX);
		return Util::Round<EnvelopeNode::value_t>(value);
	}

	void ConvertToMPT(ModInstrument *ins, const CSoundFile &sndFile, EnvelopeType envType) const
	{
		auto &env = ins->GetEnvelope(envType);
		float tickDuration = sndFile.m_PlayState.m_nSamplesPerTick / static_cast<float>(sndFile.GetSampleRate());
		if(tickDuration <= 0)
			return;
		env.clear();
		if(envType != ENV_VOLUME && attack == 0 && delay == 0 && hold == 0 && decay == 0 && sustainLevel == 100 && release == 0 && depth == 0)
		{
			env.dwFlags.reset(ENV_SUSTAIN | ENV_ENABLED);
			return;
		}
		if(attack > 0 || delay > 0)
		{
			env.push_back(EnvelopeNode(0, ToValue(startLevel, envType)));
			if(delay > 0)
				env.push_back(EnvelopeNode(ToTicks(delay, tickDuration), env.back().value));
			env.push_back(EnvelopeNode(env.back().tick + ToTicks(attack, tickDuration), ToValue(100, envType)));
		}
		if(hold > 0)
		{
			if(env.empty())
				env.push_back(EnvelopeNode(0, ToValue(100, envType)));
			env.push_back(EnvelopeNode(env.back().tick + ToTicks(hold, tickDuration), env.back().value));
		}
		if(env.empty())
			env.push_back(EnvelopeNode(0, ToValue(100, envType)));
		auto sustain = ToValue(sustainLevel, envType);
		if(env.back().value != sustain)
			env.push_back(EnvelopeNode(env.back().tick + ToTicks(decay, tickDuration), sustain));
		env.nSustainStart = env.nSustainEnd = static_cast<uint8>(env.size() - 1);
		if(sustainLevel != 0)
		{
			env.push_back(EnvelopeNode(env.back().tick + ToTicks(release, tickDuration), ToValue(0, envType)));
			env.dwFlags.set(ENV_SUSTAIN);
		}
		env.dwFlags.set(ENV_ENABLED);
	}
};

struct SFZRegion
{
	std::string filename;
	SFZEnvelope ampEnv, pitchEnv, filterEnv;
	SmpLength loopStart, loopEnd, end, offset;
	FlagSet<ChannelFlags> loopMode, loopType;
	int32 cutoff;			// in Hz
	int32 filterRandom;		// 0...9600 cents
	int16 volume;			// -144dB...+6dB
	int16 pitchBend;		// -9600...9600 cents
	float pitchLfoFade;		// 0...100 seconds
	int16 pitchLfoDepth;	// -1200...12000
	uint8 pitchLfoFreq;		// 0...20 Hz
	int8 panning;			// -100...+100
	int8 transpose;
	int8 finetune;
	uint8 keyLo, keyHi, keyRoot;
	uint8 resonance;		// 0...40dB
	uint8 filterType;
	uint8 polyphony;
	bool useSampleKeyRoot : 1;
	bool invertPhase : 1;

	SFZRegion()
		: loopStart(0), loopEnd(0), end(MAX_SAMPLE_LENGTH), offset(0)
		, loopMode(ChannelFlags(0))
		, loopType(ChannelFlags(0))
		, cutoff(0)
		, filterRandom(0)
		, volume(0)
		, pitchBend(200)
		, pitchLfoFade(0)
		, pitchLfoDepth(0)
		, pitchLfoFreq(0)
		, panning(-128)
		, transpose(0)
		, finetune(0)
		, keyLo(0), keyHi(127), keyRoot(60)
		, resonance(0)
		, filterType(FLTMODE_UNCHANGED)
		, polyphony(255)
		, useSampleKeyRoot(false)
		, invertPhase(false)
	{ }

	template<typename T, typename Tc>
	static void Read(const std::string &valueStr, T &value, Tc valueMin = std::numeric_limits<T>::min(), Tc valueMax = std::numeric_limits<T>::max())
	{
		value = Clamp(ConvertStrTo<T>(valueStr), static_cast<T>(valueMin), static_cast<T>(valueMax));
	}

	static uint8 ReadKey(const std::string &value, const SFZControl &control)
	{
		if(value.empty())
			return 0;

		int key = 0;
		if(value[0] >= '0' && value[0] <= '9')
		{
			// MIDI key
			key = ConvertStrTo<uint8>(value);
		} else if(value.length() < 2)
		{
			return 0;
		} else
		{
			// Scientific pitch
			static const int8 keys[] = { 9, 11, 0, 2, 4, 5, 7 };
			STATIC_ASSERT(CountOf(keys) == 'g' - 'a' + 1);
			auto keyC = value[0];
			if(keyC >= 'A' && keyC <= 'G')
				key = keys[keyC - 'A'];
			if(keyC >= 'a' && keyC <= 'g')
				key = keys[keyC - 'a'];
			else
				return 0;

			uint8 octaveOffset = 1;
			if(value[1] == '#')
			{
				key++;
				octaveOffset = 2;
			} else if(value[1] == 'b' || value[1] == 'B')
			{
				key--;
				octaveOffset = 2;
			}
			if(octaveOffset >= value.length())
				return 0;

			int8 octave = ConvertStrTo<int8>(value.data() + octaveOffset);
			key += (octave + 1) * 12;
		}
		key += control.octaveOffset * 12 + control.noteOffset;
		return static_cast<uint8>(Clamp(key, 0, 127));
}

	void Parse(const std::string &key, const std::string &value, const SFZControl &control)
	{
		if(key == "sample")
			filename = control.defaultPath + value;
		else if(key == "lokey")
			keyLo = ReadKey(value, control);
		else if(key == "hikey")
			keyHi = ReadKey(value, control);
		else if(key == "pitch_keycenter")
		{
			keyRoot = ReadKey(value, control);
			useSampleKeyRoot = (value == "sample");
		}
		else if(key == "key")
		{
			keyLo = keyHi = keyRoot = ReadKey(value, control);
			useSampleKeyRoot = false;
		}
		else if(key == "bend_up" || key == "bendup")
			Read(value, pitchBend, -9600, 9600);
		else if(key == "pitchlfo_fade")
			Read(value, pitchLfoFade, 0.0f, 100.0f);
		else if(key == "pitchlfo_depth")
			Read(value, pitchLfoDepth, -12000, 12000);
		else if(key == "pitchlfo_freq")
			Read(value, pitchLfoFreq, 0, 20);
		else if(key == "volume")
			Read(value, volume, -144, 6);
		else if(key == "pan")
			Read(value, panning, -100, 100);
		else if(key == "transpose")
			Read(value, transpose, -127, 127);
		else if(key == "tune")
			Read(value, finetune, -100, 100);
		else if(key == "end")
			Read(value, end, SmpLength(0), MAX_SAMPLE_LENGTH);
		else if(key == "offset")
			Read(value, offset, SmpLength(0), MAX_SAMPLE_LENGTH);
		else if(key == "loop_start" || key == "loopstart")
			Read(value, loopStart, SmpLength(0), MAX_SAMPLE_LENGTH);
		else if(key == "loop_end" || key == "loopend")
			Read(value, loopEnd, SmpLength(0), MAX_SAMPLE_LENGTH);
		else if(key == "loop_mode" || key == "loopmode")
		{
			if(value == "loop_continuous" || value == "one_shot")
				loopMode = CHN_LOOP;
			else if(value == "loop_sustain")
				loopMode = CHN_SUSTAINLOOP;
			else if(value == "no_loop")
				loopMode = ChannelFlags(0);
		}
		else if(key == "loop_type" || key == "looptype")
		{
			if(value == "forward")
				loopType = ChannelFlags(0);
			else if(value == "backward")
				loopType = CHN_REVERSE;
			else if(value == "alternate")
				loopType = CHN_PINGPONGLOOP | CHN_PINGPONGSUSTAIN;
		}
		else if(key == "cutoff")
			Read(value, cutoff, 0, 96000);
		else if(key == "fil_random")
			Read(value, filterRandom, 0, 9600);
		else if(key == "resonance")
			Read(value, resonance, 0u, 40u);
		else if(key == "polyphony")
			Read(value, polyphony, 0u, 255u);
		else if(key == "phase")
			invertPhase = (value == "invert");
		else if(key == "fil_type" || key == "filtype")
		{
			if(value == "lpf_1p" || value == "lpf_2p" || value == "lpf_4p" || value == "lpf_6p")
				filterType = FLTMODE_LOWPASS;
			else if(value == "hpf_1p" || value == "hpf_2p" || value == "hpf_4p" || value == "hpf_6p")
				filterType = FLTMODE_HIGHPASS;
			// Alternatives: bpf_2p, brf_2p
		}
		else if(key.substr(0, 6) == "ampeg_")
			ampEnv.Parse(key, value);
		else if(key.substr(0, 6) == "fileg_")
			filterEnv.Parse(key, value);
		else if(key.substr(0, 8) == "pitcheg_")
			pitchEnv.Parse(key, value);
	}
};

bool CSoundFile::ReadSFZInstrument(INSTRUMENTINDEX nInstr, FileReader &file)
//--------------------------------------------------------------------------
{
#ifdef MPT_EXTERNAL_SAMPLES
	file.Rewind();

	enum { kNone, kGlobal, kMaster, kGroup, kRegion, kControl, kUnknown } section = kNone;
	SFZControl control;
	SFZRegion group, master, globals;
	std::vector<SFZRegion> regions;
	std::map<std::string, std::string> macros;

	std::string s;
	while(file.ReadLine(s, 1024))
	{
		// First, terminate line at the start of a comment block
		auto commentPos = s.find("//");
		if(commentPos != std::string::npos)
		{
			s.resize(commentPos);
		}

		// Now, read the tokens.
		// This format is so funky that no general tokenizer approach seems to work here...
		// Consider this jolly good example found at https://stackoverflow.com/questions/5923895/tokenizing-a-custom-text-file-format-file-using-c-sharp
		// <region>sample=piano C3.wav key=48 ampeg_release=0.7 // a comment here
		// <region>key = 49 sample = piano Db3.wav
		// <region>
		// group=1
		// key = 48
		//     sample = piano D3.ogg
		// The original sfz specification claims that spaces around = are not allowed, but a quick look into the real world tells us otherwise.

		while(!s.empty())
		{
			s.erase(0, s.find_first_not_of(" \t"));
			
			// Replace macros
			for(auto m = macros.cbegin(); m != macros.cend(); m++)
			{
				const auto &oldStr = m->first;
				const auto &newStr = m->second;
				std::string::size_type pos = 0;
				while((pos = s.find(oldStr, pos)) != std::string::npos)
				{
					s.replace(pos, oldStr.length(), newStr);
					pos += newStr.length();
				}
			}

			if(s.empty())
			{
				break;
			}
			std::string::size_type charsRead = 0;

			if(s.substr(0, 1) == "<" && (charsRead = s.find('>')) != std::string::npos)
			{
				// Section header
				std::string sec = s.substr(1, charsRead - 1);
				section = kUnknown;
				if(sec == "global")
				{
					section = kGlobal;
					// Reset global parameters
					globals = SFZRegion();
				} else if(sec == "master")
				{
					section = kMaster;
					// Reset master parameters
					master = globals;
				} else if(sec == "group")
				{
					section = kGroup;
					// Reset group parameters
					group = master;
				} else if(sec == "region")
				{
					section = kRegion;
					regions.push_back(group);
				} else if(sec == "control")
				{
					section = kControl;
				}
				charsRead++;
			} else if(s.substr(0, 8) == "#define " || s.substr(0, 8) == "#define\t")
			{
				// Macro definition
				auto keyStart = s.find_first_not_of(" \t", 8);
				auto keyEnd = s.find_first_of(" \t", keyStart);
				auto valueStart = s.find_first_not_of(" \t", keyEnd);
				std::string key = s.substr(keyStart, keyEnd - keyStart);
				if(valueStart != std::string::npos && key.length() > 1 && key[0] == '$')
				{
					charsRead = s.find_first_of(" \t", valueStart);
					macros[key] = s.substr(valueStart, charsRead - valueStart);
				}
			} else if(section == kNone)
			{
				// Garbage before any section, probably not an sfz file
				return false;
			} else if(s.find('=') != std::string::npos)
			{
				// Read key=value pair
				auto keyEnd = s.find_first_of(" =");
				auto valueStart = s.find_first_not_of(" =", keyEnd);
				std::string key = mpt::ToLowerCaseAscii(s.substr(0, keyEnd));
				if(key == "sample" || key == "default_path")
				{
					// Sample name may contain spaces...
					charsRead = s.find_first_of("=\t<", valueStart);
					if(charsRead != std::string::npos && s[charsRead] == '=')
					{
						while(charsRead > valueStart && s[charsRead] != ' ')
						{
							charsRead--;
						}
					}
				} else
				{
					charsRead = s.find_first_of(" \t<", valueStart);
				}
				std::string value = s.substr(valueStart, charsRead - valueStart);

				switch(section)
				{
				case kGlobal:
					globals.Parse(key, value, control);
					MPT_FALLTHROUGH;
				case kMaster:
					master.Parse(key, value, control);
					MPT_FALLTHROUGH;
				case kGroup:
					group.Parse(key, value, control);
					break;
				case kRegion:
					regions.back().Parse(key, value, control);
					break;
				case kControl:
					control.Parse(key, value);
					break;
				}
			} else
			{
				// Garbage, probably not an sfz file
				MPT_ASSERT(false);
				return false;
			}

			// Remove the token(s) we just read
			s.erase(0, charsRead);
		}
	}

	if(regions.empty())
	{
		return false;
	}


	ModInstrument *pIns = new (std::nothrow) ModInstrument();
	if(pIns == nullptr)
	{
		return false;
	}

	DestroyInstrument(nInstr, deleteAssociatedSamples);
	if(nInstr > m_nInstruments) m_nInstruments = nInstr;
	Instruments[nInstr] = pIns;

	SAMPLEINDEX prevSmp = 0;
	for(auto region = regions.begin(); region != regions.end(); region++)
	{
		uint8 keyLo = region->keyLo, keyHi = region->keyHi;
		if(keyLo > keyHi)
			continue;
		Clamp<uint8, uint8>(keyLo, 0, NOTE_MAX - NOTE_MIN);
		Clamp<uint8, uint8>(keyHi, 0, NOTE_MAX - NOTE_MIN);
		SAMPLEINDEX smp = GetNextFreeSample(nInstr, prevSmp + 1);
		if(smp == SAMPLEINDEX_INVALID)
			break;
		prevSmp = smp;

		ModSample &sample = Samples[smp];
		mpt::PathString filename = mpt::PathString::FromUTF8(region->filename);
		if(!filename.empty())
		{
			if(region->filename.find(':') == std::string::npos)
			{
				filename = file.GetFileName().GetPath() + filename;
			}
			SetSamplePath(smp, filename);
			InputFile f(filename);
			FileReader file = GetFileReader(f);
			if(!ReadSampleFromFile(smp, file, false))
			{
				AddToLog(LogWarning, MPT_USTRING("Unable to load sample: ") + filename.ToUnicode());
				prevSmp--;
				continue;
			}
			if(!m_szNames[smp][0])
			{
				mpt::String::Copy(m_szNames[smp], filename.GetFullFileName().ToLocale());
			}
		}
		sample.uFlags.set(SMP_KEEPONDISK, sample.pSample != nullptr);

		if(region->useSampleKeyRoot)
		{
			if(sample.rootNote != NOTE_NONE)
				region->keyRoot = sample.rootNote - NOTE_MIN;
			else
				region->keyRoot = 60;
		}

		int8 transp = NOTE_MIN + region->transpose + (60 - region->keyRoot);
		for(uint8 i = keyLo; i <= keyHi; i++)
		{
			pIns->Keyboard[i] = smp;
			pIns->NoteMap[i] = i + transp;
		}

		pIns->nFilterMode = region->filterType;
		if(region->cutoff != 0)
			pIns->SetCutoff(FrequencyToCutOff(region->cutoff), true);
		if(region->resonance != 0)
			pIns->SetResonance(mpt::saturate_cast<uint8>(Util::muldivr(region->resonance, 128, 24)), true);
		pIns->nCutSwing = mpt::saturate_cast<uint8>(Util::muldivr(region->filterRandom, m_SongFlags[SONG_EXFILTERRANGE] ? 20 : 24, 1200));
		pIns->midiPWD = static_cast<int8>(region->pitchBend / 100);

		pIns->nNNA = NNA_NOTEOFF;
		if(region->polyphony == 1)
		{
			pIns->nDNA = NNA_NOTECUT;
			pIns->nDCT = DCT_SAMPLE;
		}
		region->ampEnv.ConvertToMPT(pIns, *this, ENV_VOLUME);
		region->pitchEnv.ConvertToMPT(pIns, *this, ENV_PITCH);
		//region->filterEnv.ConvertToMPT(pIns, *this, ENV_PITCH);

		sample.rootNote = region->keyRoot + NOTE_MIN;
		sample.nGlobalVol = Util::Round<decltype(sample.nGlobalVol)>(64 * std::pow(10.0, region->volume / 20.0));
		if(region->panning != -128)
		{
			sample.nPan = static_cast<decltype(sample.nPan)>(Util::muldivr_unsigned(region->panning + 100, 256, 200));
			sample.uFlags.set(CHN_PANNING);
		}
		sample.Transpose(region->finetune / 1200.0);

		if(region->pitchLfoDepth && region->pitchLfoFreq)
		{
			sample.nVibSweep = 255;
			if(region->pitchLfoFade > 0)
				sample.nVibSweep = Util::Round<uint8>(255 / region->pitchLfoFade);
			sample.nVibDepth = static_cast<uint8>(Util::muldivr(region->pitchLfoDepth, 32, 100));
			sample.nVibRate = region->pitchLfoFreq * 4;
		}
		
		sample.uFlags.set(region->loopMode);
		sample.uFlags.set(region->loopType);
		if(region->loopEnd > region->loopStart)
		{
			// Loop may also be defined in file, in which case loopStart and loopEnd are unset.
			if(region->loopMode == CHN_SUSTAINLOOP)
			{
				sample.nSustainStart = region->loopStart;
				sample.nSustainEnd = region->loopEnd + 1;
			} else if(region->loopMode == CHN_LOOP)
			{
				sample.nLoopStart = region->loopStart;
				sample.nLoopEnd = region->loopEnd + 1;
			}
		} else if(sample.nLoopEnd <= sample.nLoopStart && region->loopMode)
		{
			sample.nLoopEnd = sample.nLength;
		}
		if(region->offset && region->offset < sample.nLength)
		{
			auto offset = region->offset * sample.GetBytesPerSample();
			memmove(sample.pSample8, sample.pSample8 + offset, sample.nLength * sample.GetBytesPerSample() - offset);
			if(region->end > region->offset)
				region->end -= region->offset;
			sample.nLoopStart -= region->offset;
			sample.nLoopEnd -= region->offset;
			sample.uFlags.set(SMP_MODIFIED);
		}
		LimitMax(sample.nLength, region->end);

		if(region->invertPhase)
		{
			ctrlSmp::InvertSample(sample, 0, sample.nLength, *this);
			sample.uFlags.set(SMP_MODIFIED);
		}

		sample.PrecomputeLoops(*this, false);
		sample.Convert(MOD_TYPE_MPT, GetType());
	}

	pIns->Sanitize(MOD_TYPE_MPT);
	pIns->Convert(MOD_TYPE_MPT, GetType());
	return true;
#else
	MPT_UNREFERENCED_PARAMETER(nInstr);
	MPT_UNREFERENCED_PARAMETER(file);
	return false;
#endif // MPT_EXTERNAL_SAMPLES
}


/////////////////////////////////////////////////////////////////////////////////////////
// AIFF File I/O

// AIFF header
struct AIFFHeader
{
	char     magic[4];	// FORM
	uint32be length;	// Size of the file, not including magic and length
	char     type[4];	// AIFF or AIFC
};

MPT_BINARY_STRUCT(AIFFHeader, 12)


// General IFF Chunk header
struct AIFFChunk
{
	// 32-Bit chunk identifiers
	enum ChunkIdentifiers
	{
		idCOMM	= MAGIC4BE('C','O','M','M'),
		idSSND	= MAGIC4BE('S','S','N','D'),
		idINST	= MAGIC4BE('I','N','S','T'),
		idMARK	= MAGIC4BE('M','A','R','K'),
		idNAME	= MAGIC4BE('N','A','M','E'),
	};

	typedef ChunkIdentifiers id_type;

	uint32be id;		// See ChunkIdentifiers
	uint32be length;	// Chunk size without header

	size_t GetLength() const
	{
		return length;
	}

	id_type GetID() const
	{
		return static_cast<id_type>(id.get());
	}
};

MPT_BINARY_STRUCT(AIFFChunk, 8)


// "Common" chunk (in AIFC, a compression ID and compression name follows this header, but apart from that it's identical)
struct AIFFCommonChunk
{
	uint16be numChannels;
	uint32be numSampleFrames;
	uint16be sampleSize;
	uint8be  sampleRate[10];		// Sample rate in 80-Bit floating point

	// Convert sample rate to integer
	uint32 GetSampleRate() const
	{
		uint32 mantissa = (sampleRate[2] << 24) | (sampleRate[3] << 16) | (sampleRate[4] << 8) | (sampleRate[5] << 0);
		uint32 last = 0;
		uint8 exp = 30 - sampleRate[1];

		while(exp--)
		{
			last = mantissa;
			mantissa >>= 1;
		}
		if(last & 1) mantissa++;
		return mantissa;
	}
};

MPT_BINARY_STRUCT(AIFFCommonChunk, 18)


// Sound chunk
struct AIFFSoundChunk
{
	uint32be offset;
	uint32be blockSize;
};

MPT_BINARY_STRUCT(AIFFSoundChunk, 8)


// Marker
struct AIFFMarker
{
	uint16be id;
	uint32be position;		// Position in sample
	uint8be  nameLength;	// Not counting eventually existing padding byte in name string
};

MPT_BINARY_STRUCT(AIFFMarker, 7)


// Instrument loop
struct AIFFInstrumentLoop
{
	enum PlayModes
	{
		noLoop		= 0,
		loopNormal	= 1,
		loopBidi	= 2,
	};

	uint16be playMode;
	uint16be beginLoop;	// Marker index
	uint16be endLoop;		// Marker index
};

MPT_BINARY_STRUCT(AIFFInstrumentLoop, 6)


struct AIFFInstrumentChunk
{
	uint8be  baseNote;
	uint8be  detune;
	uint8be  lowNote;
	uint8be  highNote;
	uint8be  lowVelocity;
	uint8be  highVelocity;
	uint16be gain;
	AIFFInstrumentLoop sustainLoop;
	AIFFInstrumentLoop releaseLoop;
};

MPT_BINARY_STRUCT(AIFFInstrumentChunk, 20)


bool CSoundFile::ReadAIFFSample(SAMPLEINDEX nSample, FileReader &file, bool mayNormalize)
//---------------------------------------------------------------------------------------
{
	file.Rewind();
	ChunkReader chunkFile(file);

	// Verify header
	AIFFHeader fileHeader;
	if(!chunkFile.ReadStruct(fileHeader)
		|| memcmp(fileHeader.magic, "FORM", 4)
		|| (memcmp(fileHeader.type, "AIFF", 4) && memcmp(fileHeader.type, "AIFC", 4)))
	{
		return false;
	}

	ChunkReader::ChunkList<AIFFChunk> chunks = chunkFile.ReadChunks<AIFFChunk>(2);

	// Read COMM chunk
	FileReader commChunk(chunks.GetChunk(AIFFChunk::idCOMM));
	AIFFCommonChunk sampleInfo;
	if(!commChunk.ReadStruct(sampleInfo))
	{
		return false;
	}

	// Is this a proper sample?
	if(sampleInfo.numSampleFrames == 0
		|| sampleInfo.numChannels < 1 || sampleInfo.numChannels > 2
		|| sampleInfo.sampleSize < 1 || sampleInfo.sampleSize > 64)
	{
		return false;
	}

	// Read compression type in AIFF-C files.
	uint8 compression[4] = { 'N', 'O', 'N', 'E' };
	SampleIO::Endianness endian = SampleIO::bigEndian;
	if(!memcmp(fileHeader.type, "AIFC", 4))
	{
		if(!commChunk.ReadArray(compression))
		{
			return false;
		}
		if(!memcmp(compression, "twos", 4))
		{
			endian = SampleIO::littleEndian;
		}
	}

	// Read SSND chunk
	FileReader soundChunk(chunks.GetChunk(AIFFChunk::idSSND));
	AIFFSoundChunk sampleHeader;
	if(!soundChunk.ReadStruct(sampleHeader)
		|| !soundChunk.CanRead(sampleHeader.offset))
	{
		return false;
	}

	SampleIO::Bitdepth bitDepth;
	switch((sampleInfo.sampleSize - 1) / 8)
	{
	default:
	case 0: bitDepth = SampleIO::_8bit; break;
	case 1: bitDepth = SampleIO::_16bit; break;
	case 2: bitDepth = SampleIO::_24bit; break;
	case 3: bitDepth = SampleIO::_32bit; break;
	case 7: bitDepth = SampleIO::_64bit; break;
	}

	SampleIO sampleIO(bitDepth,
		(sampleInfo.numChannels == 2) ? SampleIO::stereoInterleaved : SampleIO::mono,
		endian,
		SampleIO::signedPCM);

	if(!memcmp(compression, "fl32", 4) || !memcmp(compression, "FL32", 4) || !memcmp(compression, "fl64", 4))
	{
		sampleIO |= SampleIO::floatPCM;
	} else if(!memcmp(compression, "alaw", 4) || !memcmp(compression, "ALAW", 4))
	{
		sampleIO |= SampleIO::aLaw;
		sampleIO |= SampleIO::_16bit;
	} else if(!memcmp(compression, "ulaw", 4) || !memcmp(compression, "ULAW", 4))
	{
		sampleIO |= SampleIO::uLaw;
		sampleIO |= SampleIO::_16bit;
	}

	if(mayNormalize)
	{
		sampleIO.MayNormalize();
	}

	soundChunk.Skip(sampleHeader.offset);

	ModSample &mptSample = Samples[nSample];
	DestroySampleThreadsafe(nSample);
	mptSample.Initialize();
	mptSample.nLength = sampleInfo.numSampleFrames;
	mptSample.nC5Speed = sampleInfo.GetSampleRate();

	sampleIO.ReadSample(mptSample, soundChunk);

	// Read MARK and INST chunk to extract sample loops
	FileReader markerChunk(chunks.GetChunk(AIFFChunk::idMARK));
	AIFFInstrumentChunk instrHeader;
	if(markerChunk.IsValid() && chunks.GetChunk(AIFFChunk::idINST).ReadStruct(instrHeader))
	{
		uint16 numMarkers = markerChunk.ReadUint16BE();

		std::vector<AIFFMarker> markers;
		markers.reserve(numMarkers);
		for(size_t i = 0; i < numMarkers; i++)
		{
			AIFFMarker marker;
			if(!markerChunk.ReadStruct(marker))
			{
				break;
			}
			markers.push_back(marker);
			markerChunk.Skip(marker.nameLength + ((marker.nameLength % 2u) == 0 ? 1 : 0));
		}

		if(instrHeader.sustainLoop.playMode != AIFFInstrumentLoop::noLoop)
		{
			mptSample.uFlags.set(CHN_SUSTAINLOOP);
			mptSample.uFlags.set(CHN_PINGPONGSUSTAIN, instrHeader.sustainLoop.playMode == AIFFInstrumentLoop::loopBidi);
		}

		if(instrHeader.releaseLoop.playMode != AIFFInstrumentLoop::noLoop)
		{
			mptSample.uFlags.set(CHN_LOOP);
			mptSample.uFlags.set(CHN_PINGPONGLOOP, instrHeader.releaseLoop.playMode == AIFFInstrumentLoop::loopBidi);
		}

		// Read markers
		for(auto iter = markers.begin(); iter != markers.end(); iter++)
		{
			if(iter->id == instrHeader.sustainLoop.beginLoop)
			{
				mptSample.nSustainStart = iter->position;
			}
			if(iter->id == instrHeader.sustainLoop.endLoop)
			{
				mptSample.nSustainEnd = iter->position;
			}
			if(iter->id == instrHeader.releaseLoop.beginLoop)
			{
				mptSample.nLoopStart = iter->position;
			}
			if(iter->id == instrHeader.releaseLoop.endLoop)
			{
				mptSample.nLoopEnd = iter->position;
			}
		}
		mptSample.SanitizeLoops();
	}

	// Extract sample name
	FileReader nameChunk(chunks.GetChunk(AIFFChunk::idNAME));
	if(nameChunk.IsValid())
	{
		nameChunk.ReadString<mpt::String::spacePadded>(m_szNames[nSample], nameChunk.GetLength());
	} else
	{
		strcpy(m_szNames[nSample], "");
	}

	mptSample.Convert(MOD_TYPE_IT, GetType());
	mptSample.PrecomputeLoops(*this, false);
	return true;
}


bool CSoundFile::ReadAUSample(SAMPLEINDEX nSample, FileReader &file, bool mayNormalize)
//-------------------------------------------------------------------------------------
{
	file.Rewind();

	// Verify header
	if(!file.ReadMagic(".snd"))
		return false;

	uint32 dataOffset = file.ReadUint32BE();
	uint32 dataSize = file.ReadUint32BE();
	uint32 encoding = file.ReadUint32BE();
	uint32 sampleRate = file.ReadUint32BE();
	uint32 channels = file.ReadUint32BE();

	if(channels < 1 || channels > 2)
		return false;

	SampleIO sampleIO(SampleIO::_8bit, channels == 1 ? SampleIO::mono : SampleIO::stereoInterleaved, SampleIO::bigEndian, SampleIO::signedPCM);
	switch(encoding)
	{
	case 1: sampleIO |= SampleIO::_16bit;			// u-law
		sampleIO |= SampleIO::uLaw; break;
	case 2: break;									// 8-bit linear PCM
	case 3: sampleIO |= SampleIO::_16bit; break;	// 16-bit linear PCM
	case 4: sampleIO |= SampleIO::_24bit; break;	// 24-bit linear PCM
	case 5: sampleIO |= SampleIO::_32bit; break;	// 32-bit linear PCM
	case 6: sampleIO |= SampleIO::_32bit;			// 32-bit IEEE floating point
		sampleIO |= SampleIO::floatPCM;
		break;
	case 7: sampleIO |= SampleIO::_64bit;			// 64-bit IEEE floating point
		sampleIO |= SampleIO::floatPCM;
		break;
	case 27: sampleIO |= SampleIO::_16bit;			// a-law
		sampleIO |= SampleIO::aLaw; break;
	default: return false;
	}

	if(!file.Seek(dataOffset))
		return false;

	ModSample &mptSample = Samples[nSample];
	DestroySampleThreadsafe(nSample);
	mptSample.Initialize();
	SmpLength length = mpt::saturate_cast<SmpLength>(file.BytesLeft());
	if(dataSize != 0xFFFFFFFF)
		LimitMax(length, dataSize);
	mptSample.nLength = (length * 8u) / (sampleIO.GetEncodedBitsPerSample() * channels);
	mptSample.nC5Speed = sampleRate;
	strcpy(m_szNames[nSample], "");

	if(mayNormalize)
	{
		sampleIO.MayNormalize();
	}

	sampleIO.ReadSample(mptSample, file);

	mptSample.Convert(MOD_TYPE_IT, GetType());
	mptSample.PrecomputeLoops(*this, false);
	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////
// ITS Samples


bool CSoundFile::ReadITSSample(SAMPLEINDEX nSample, FileReader &file, bool rewind)
//--------------------------------------------------------------------------------
{
	if(rewind)
	{
		file.Rewind();
	}

	ITSample sampleHeader;
	if(!file.ReadStruct(sampleHeader)
		|| memcmp(sampleHeader.id, "IMPS", 4))
	{
		return false;
	}
	DestroySampleThreadsafe(nSample);

	ModSample &sample = Samples[nSample];
	file.Seek(sampleHeader.ConvertToMPT(sample));
	mpt::String::Read<mpt::String::spacePaddedNull>(m_szNames[nSample], sampleHeader.name);

	if(!sample.uFlags[SMP_KEEPONDISK])
	{
		sampleHeader.GetSampleFormat().ReadSample(Samples[nSample], file);
	} else
	{
		// External sample
		size_t strLen;
		file.ReadVarInt(strLen);
#ifdef MPT_EXTERNAL_SAMPLES
		std::string filenameU8;
		file.ReadString<mpt::String::maybeNullTerminated>(filenameU8, strLen);
		mpt::PathString filename = mpt::PathString::FromUTF8(filenameU8);

		if(!filename.empty())
		{
			if(!file.GetFileName().empty())
			{
				filename = filename.RelativePathToAbsolute(file.GetFileName().GetPath());
			}
			if(!LoadExternalSample(nSample, filename))
			{
				AddToLog(LogWarning, MPT_USTRING("Unable to load sample: ") + filename.ToUnicode());
			}
		} else
		{
			sample.uFlags.reset(SMP_KEEPONDISK);
		}
#else
		file.Skip(strLen);
#endif // MPT_EXTERNAL_SAMPLES
	}

	sample.Convert(MOD_TYPE_IT, GetType());
	sample.PrecomputeLoops(*this, false);
	return true;
}


bool CSoundFile::ReadITISample(SAMPLEINDEX nSample, FileReader &file)
//-------------------------------------------------------------------
{
	ITInstrument instrumentHeader;

	file.Rewind();
	if(!file.ReadStruct(instrumentHeader)
		|| memcmp(instrumentHeader.id, "IMPI", 4))
	{
		return false;
	}
	file.Rewind();
	ModInstrument dummy;
	ITInstrToMPT(file, dummy, instrumentHeader.trkvers);
	SAMPLEINDEX nsamples = instrumentHeader.nos;
	// Old SchismTracker versions set nos=0
	for(size_t i = 0; i < CountOf(dummy.Keyboard); i++)
	{
		nsamples = std::max(nsamples, dummy.Keyboard[i]);
	}
	if(!nsamples)
		return false;

	// Preferrably read the middle-C sample
	auto sample = dummy.Keyboard[NOTE_MIDDLEC - NOTE_MIN];
	if(sample > 0 && sample <= instrumentHeader.nos)
		sample--;
	else
		sample = 0;
	file.Seek(file.GetPosition() + sample * sizeof(ITSample));
	return ReadITSSample(nSample, file, false);
}


bool CSoundFile::ReadITIInstrument(INSTRUMENTINDEX nInstr, FileReader &file)
//--------------------------------------------------------------------------
{
	ITInstrument instrumentHeader;
	SAMPLEINDEX smp = 0;

	file.Rewind();
	if(!file.ReadStruct(instrumentHeader)
		|| memcmp(instrumentHeader.id, "IMPI", 4))
	{
		return false;
	}
	if(nInstr > GetNumInstruments()) m_nInstruments = nInstr;

	ModInstrument *pIns = new (std::nothrow) ModInstrument();
	if(pIns == nullptr)
	{
		return false;
	}

	DestroyInstrument(nInstr, deleteAssociatedSamples);

	Instruments[nInstr] = pIns;
	file.Rewind();
	ITInstrToMPT(file, *pIns, instrumentHeader.trkvers);
	SAMPLEINDEX nsamples = instrumentHeader.nos;
	// Old SchismTracker versions set nos=0
	for(size_t i = 0; i < CountOf(pIns->Keyboard); i++)
	{
		nsamples = std::max(nsamples, pIns->Keyboard[i]);
	}

	// In order to properly compute the position, in file, of eventual extended settings
	// such as "attack" we need to keep the "real" size of the last sample as those extra
	// setting will follow this sample in the file
	FileReader::off_t extraOffset = file.GetPosition();

	// Reading Samples
	std::vector<SAMPLEINDEX> samplemap(nsamples, 0);
	for(SAMPLEINDEX i = 0; i < nsamples; i++)
	{
		smp = GetNextFreeSample(nInstr, smp + 1);
		if(smp == SAMPLEINDEX_INVALID) break;
		samplemap[i] = smp;
		const FileReader::off_t offset = file.GetPosition();
		if(!ReadITSSample(smp, file, false))
			smp--;
		extraOffset = std::max(extraOffset, file.GetPosition());
		file.Seek(offset + sizeof(ITSample));
	}
	if(GetNumSamples() < smp) m_nSamples = smp;

	for(size_t j = 0; j < CountOf(pIns->Keyboard); j++)
	{
		if(pIns->Keyboard[j] && pIns->Keyboard[j] <= nsamples)
		{
			pIns->Keyboard[j] = samplemap[pIns->Keyboard[j] - 1];
		}
	}

	pIns->Convert(MOD_TYPE_IT, GetType());

	if(file.Seek(extraOffset))
	{
		// Read MPT crap
		ReadExtendedInstrumentProperties(pIns, file);
	}
	pIns->Sanitize(GetType());

	return true;
}


#ifndef MODPLUG_NO_FILESAVE

bool CSoundFile::SaveITIInstrument(INSTRUMENTINDEX nInstr, const mpt::PathString &filename, bool compress, bool allowExternal) const
//----------------------------------------------------------------------------------------------------------------------------------
{
	ITInstrument iti;
	ModInstrument *pIns = Instruments[nInstr];
	FILE *f;

	if((!pIns) || filename.empty()) return false;
	if((f = mpt_fopen(filename, "wb")) == nullptr) return false;

	size_t instSize = iti.ConvertToIT(*pIns, false, *this);

	// Create sample assignment table
	std::vector<SAMPLEINDEX> smptable;
	std::vector<uint8> smpmap(GetNumSamples(), 0);
	for(size_t i = 0; i < NOTE_MAX; i++)
	{
		const SAMPLEINDEX smp = pIns->Keyboard[i];
		if(smp && smp <= GetNumSamples())
		{
			if(!smpmap[smp - 1])
			{
				// We haven't considered this sample yet.
				smptable.push_back(smp);
				smpmap[smp - 1] = static_cast<uint8>(smptable.size());
			}
			iti.keyboard[i * 2 + 1] = smpmap[smp - 1];
		} else
		{
			iti.keyboard[i * 2 + 1] = 0;
		}
	}
	iti.nos = static_cast<uint8>(smptable.size());
	smpmap.clear();

	uint32 filePos = instSize;
	fwrite(&iti, 1, instSize, f);

	filePos += mpt::saturate_cast<uint32>(smptable.size() * sizeof(ITSample));

	// Writing sample headers + data
	std::vector<SampleIO> sampleFlags;
	for(auto iter = smptable.begin(); iter != smptable.end(); iter++)
	{
		ITSample itss;
		itss.ConvertToIT(Samples[*iter], GetType(), compress, compress, allowExternal);
		const bool isExternal = itss.cvt == ITSample::cvtExternalSample;

		mpt::String::Write<mpt::String::nullTerminated>(itss.name, m_szNames[*iter]);

		itss.samplepointer = filePos;
		fwrite(&itss, 1, sizeof(itss), f);

		// Write sample
		off_t curPos = ftell(f);
		fseek(f, filePos, SEEK_SET);
		if(!isExternal)
		{
			filePos += mpt::saturate_cast<uint32>(itss.GetSampleFormat(0x0214).WriteSample(f, Samples[*iter]));
		} else
		{
#ifdef MPT_EXTERNAL_SAMPLES
			const std::string filenameU8 = GetSamplePath(*iter).AbsolutePathToRelative(filename.GetPath()).ToUTF8();
			const size_t strSize = mpt::saturate_cast<uint16>(filenameU8.size());
			size_t intBytes = 0;
			if(mpt::IO::WriteVarInt(f, strSize, &intBytes))
			{
				filePos += intBytes + strSize;
				mpt::IO::WriteRaw(f, filenameU8.data(), strSize);
			}
#endif // MPT_EXTERNAL_SAMPLES
		}
		fseek(f, curPos, SEEK_SET);
	}

	fseek(f, 0, SEEK_END);
	// Write 'MPTX' extension tag
	char code[4];
	memcpy(code, "XTPM", 4);
	fwrite(&code, 1, 4, f);
	WriteInstrumentHeaderStructOrField(pIns, f);	// Write full extended header.

	fclose(f);
	return true;
}

#endif // MODPLUG_NO_FILESAVE


///////////////////////////////////////////////////////////////////////////////////////////////////
// 8SVX / 16SVX Samples

// IFF File Header
struct IFFHeader
{
	char     form[4];	// "FORM"
	uint32be size;
	char     magic[4];	// "8SVX" or "16SV"
};

MPT_BINARY_STRUCT(IFFHeader, 12)


// General IFF Chunk header
struct IFFChunk
{
	// 32-Bit chunk identifiers
	enum ChunkIdentifiers
	{
		idVHDR	= MAGIC4BE('V','H','D','R'),
		idBODY	= MAGIC4BE('B','O','D','Y'),
		idNAME	= MAGIC4BE('N','A','M','E'),
	};

	typedef ChunkIdentifiers id_type;

	uint32be id;		// See ChunkIdentifiers
	uint32be length;	// Chunk size without header

	size_t GetLength() const
	{
		if(length == 0)	// Broken files
			return std::numeric_limits<size_t>::max();
		return length;
	}

	id_type GetID() const
	{
		return static_cast<id_type>(id.get());
	}
};

MPT_BINARY_STRUCT(IFFChunk, 8)


struct IFFSampleHeader
{
	uint32be oneShotHiSamples;	// Samples in the high octave 1-shot part
	uint32be repeatHiSamples;	// Samples in the high octave repeat part
	uint32be samplesPerHiCycle;	// Samples/cycle in high octave, else 0
	uint16be samplesPerSec;		// Data sampling rate
	uint8be  octave;			// Octaves of waveforms
	uint8be  compression;		// Data compression technique used
	uint32be volume;
};

MPT_BINARY_STRUCT(IFFSampleHeader, 20)


bool CSoundFile::ReadIFFSample(SAMPLEINDEX nSample, FileReader &file)
//--------------------------------------------------------------------
{
	file.Rewind();

	IFFHeader fileHeader;
	if(!file.ReadStruct(fileHeader)
		|| memcmp(fileHeader.form, "FORM", 4 )
		|| (memcmp(fileHeader.magic, "8SVX", 4) && memcmp(fileHeader.magic, "16SV", 4)))
	{
		return false;
	}

	ChunkReader chunkFile(file);
	ChunkReader::ChunkList<IFFChunk> chunks = chunkFile.ReadChunks<IFFChunk>(2);

	FileReader vhdrChunk = chunks.GetChunk(IFFChunk::idVHDR);
	FileReader bodyChunk = chunks.GetChunk(IFFChunk::idBODY);
	IFFSampleHeader sampleHeader;
	if(!bodyChunk.IsValid()
		|| !vhdrChunk.IsValid()
		|| !vhdrChunk.ReadStruct(sampleHeader))
	{
		return false;
	}

	DestroySampleThreadsafe(nSample);
	// Default values
	ModSample &sample = Samples[nSample];
	sample.Initialize();
	sample.nLoopStart = sampleHeader.oneShotHiSamples;
	sample.nLoopEnd = sample.nLoopStart + sampleHeader.repeatHiSamples;
	sample.nC5Speed = sampleHeader.samplesPerSec;
	sample.nVolume = static_cast<uint16>(sampleHeader.volume >> 8);
	if(!sample.nVolume || sample.nVolume > 256) sample.nVolume = 256;
	if(!sample.nC5Speed) sample.nC5Speed = 22050;

	sample.Convert(MOD_TYPE_IT, GetType());

	FileReader nameChunk = chunks.GetChunk(IFFChunk::idNAME);
	if(nameChunk.IsValid())
	{
		nameChunk.ReadString<mpt::String::maybeNullTerminated>(m_szNames[nSample], nameChunk.GetLength());
	} else
	{
		strcpy(m_szNames[nSample], "");
	}

	sample.nLength = mpt::saturate_cast<SmpLength>(bodyChunk.GetLength());
	if((sample.nLoopStart + 4 < sample.nLoopEnd) && (sample.nLoopEnd <= sample.nLength)) sample.uFlags.set(CHN_LOOP);

	SampleIO(
		memcmp(fileHeader.magic, "8SVX", 4) ? SampleIO::_16bit : SampleIO::_8bit,
		SampleIO::mono,
		SampleIO::bigEndian,
		SampleIO::signedPCM)
		.ReadSample(sample, bodyChunk);
	sample.PrecomputeLoops(*this, false);

	return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// FLAC Samples

#ifdef MPT_WITH_FLAC

struct FLACDecoder
{
	FileReader &file;
	CSoundFile &sndFile;
	SAMPLEINDEX sample;
	bool ready;

	FLACDecoder(FileReader &f, CSoundFile &sf, SAMPLEINDEX smp) : file(f), sndFile(sf), sample(smp), ready(false) { }

	static FLAC__StreamDecoderReadStatus read_cb(const FLAC__StreamDecoder *, FLAC__byte buffer[], size_t *bytes, void *client_data)
	{
		FileReader &file = static_cast<FLACDecoder *>(client_data)->file;
		if(*bytes > 0)
		{
			FileReader::off_t readBytes = *bytes;
			LimitMax(readBytes, file.BytesLeft());
			file.ReadRaw(buffer, readBytes);
			*bytes = readBytes;
			if(*bytes == 0)
				return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
			else
				return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		} else
		{
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		}
	}

	static FLAC__StreamDecoderSeekStatus seek_cb(const FLAC__StreamDecoder *, FLAC__uint64 absolute_byte_offset, void *client_data)
	{
		FileReader &file = static_cast<FLACDecoder *>(client_data)->file;
		if(!file.Seek(static_cast<FileReader::off_t>(absolute_byte_offset)))
			return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
		else
			return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
	}

	static FLAC__StreamDecoderTellStatus tell_cb(const FLAC__StreamDecoder *, FLAC__uint64 *absolute_byte_offset, void *client_data)
	{
		FileReader &file = static_cast<FLACDecoder *>(client_data)->file;
		*absolute_byte_offset = file.GetPosition();
		return FLAC__STREAM_DECODER_TELL_STATUS_OK;
	}

	static FLAC__StreamDecoderLengthStatus length_cb(const FLAC__StreamDecoder *, FLAC__uint64 *stream_length, void *client_data)
	{
		FileReader &file = static_cast<FLACDecoder *>(client_data)->file;
		*stream_length = file.GetLength();
		return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
	}

	static FLAC__bool eof_cb(const FLAC__StreamDecoder *, void *client_data)
	{
		FileReader &file = static_cast<FLACDecoder *>(client_data)->file;
		return file.NoBytesLeft();
	}

	static FLAC__StreamDecoderWriteStatus write_cb(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
	{
		FLACDecoder &client = *static_cast<FLACDecoder *>(client_data);
		ModSample &sample = client.sndFile.GetSample(client.sample);

		if(frame->header.number.sample_number >= sample.nLength || !client.ready)
		{
			// We're reading beyond the sample size already, or we aren't even ready to decode yet!
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}

		// Number of samples to be copied in this call
		const SmpLength copySamples = std::min(static_cast<SmpLength>(frame->header.blocksize), static_cast<SmpLength>(sample.nLength - frame->header.number.sample_number));
		// Number of target channels
		const uint8 modChannels = sample.GetNumChannels();
		// Offset (in samples) into target data
		const size_t offset = static_cast<size_t>(frame->header.number.sample_number) * modChannels;
		// Source size in bytes
		const size_t srcSize = frame->header.blocksize * 4;
		// Source bit depth
		const unsigned int bps = frame->header.bits_per_sample;

		int8 *sampleData8 = sample.pSample8 + offset;
		int16 *sampleData16 = sample.pSample16 + offset;

		MPT_ASSERT((bps <= 8 && sample.GetElementarySampleSize() == 1) || (bps > 8 && sample.GetElementarySampleSize() == 2));
		MPT_ASSERT(modChannels <= FLAC__stream_decoder_get_channels(decoder));
		MPT_ASSERT(bps == FLAC__stream_decoder_get_bits_per_sample(decoder));
		MPT_UNREFERENCED_PARAMETER(decoder); // decoder is unused if ASSERTs are compiled out

		// Do the sample conversion
		for(uint8 chn = 0; chn < modChannels; chn++)
		{
			if(bps <= 8)
			{
				CopySample<SC::ConversionChain<SC::ConvertShift< int8, int32,  0>, SC::DecodeIdentity<int32> > >(sampleData8  + chn, copySamples, modChannels, buffer[chn], srcSize, 1);
			} else if(bps <= 16)
			{
				CopySample<SC::ConversionChain<SC::ConvertShift<int16, int32,  0>, SC::DecodeIdentity<int32> > >(sampleData16 + chn, copySamples, modChannels, buffer[chn], srcSize, 1);
			} else if(bps <= 24)
			{
				CopySample<SC::ConversionChain<SC::ConvertShift<int16, int32,  8>, SC::DecodeIdentity<int32> > >(sampleData16 + chn, copySamples, modChannels, buffer[chn], srcSize, 1);
			} else if(bps <= 32)
			{
				CopySample<SC::ConversionChain<SC::ConvertShift<int16, int32, 16>, SC::DecodeIdentity<int32> > >(sampleData16 + chn, copySamples, modChannels, buffer[chn], srcSize, 1);
			}
		}

		return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	}

	static void metadata_cb(const FLAC__StreamDecoder *, const FLAC__StreamMetadata *metadata, void *client_data)
	{
		FLACDecoder &client = *static_cast<FLACDecoder *>(client_data);
		if(client.sample > client.sndFile.GetNumSamples())
		{
			client.sndFile.m_nSamples = client.sample;
		}
		ModSample &sample = client.sndFile.GetSample(client.sample);

		if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO && metadata->data.stream_info.total_samples != 0)
		{
			// Init sample information
			client.sndFile.DestroySampleThreadsafe(client.sample);
			strcpy(client.sndFile.m_szNames[client.sample], "");
			sample.Initialize();
			sample.uFlags.set(CHN_16BIT, metadata->data.stream_info.bits_per_sample > 8);
			sample.uFlags.set(CHN_STEREO, metadata->data.stream_info.channels > 1);
			sample.nLength = static_cast<SmpLength>(metadata->data.stream_info.total_samples);
			sample.nC5Speed = metadata->data.stream_info.sample_rate;
			client.ready = (sample.AllocateSample() != 0);
		} else if(metadata->type == FLAC__METADATA_TYPE_APPLICATION && !memcmp(metadata->data.application.id, "riff", 4) && client.ready)
		{
			// Try reading RIFF loop points and other sample information
			ChunkReader data(mpt::as_span(metadata->data.application.data, metadata->length));
			ChunkReader::ChunkList<RIFFChunk> chunks = data.ReadChunks<RIFFChunk>(2);

			// We're not really going to read a WAV file here because there will be only one RIFF chunk per metadata event, but we can still re-use the code for parsing RIFF metadata...
			WAVReader riffReader(data);
			riffReader.FindMetadataChunks(chunks);
			riffReader.ApplySampleSettings(sample, client.sndFile.m_szNames[client.sample]);
		} else if(metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT && client.ready)
		{
			// Try reading Vorbis Comments for sample title, sample rate and loop points
			SmpLength loopStart = 0, loopLength = 0;
			for(FLAC__uint32 i = 0; i < metadata->data.vorbis_comment.num_comments; i++)
			{
				const char *tag = mpt::byte_cast<const char *>(metadata->data.vorbis_comment.comments[i].entry);
				const FLAC__uint32 length = metadata->data.vorbis_comment.comments[i].length;
				if(length > 6 && !mpt::CompareNoCaseAscii(tag, "TITLE=", 6))
				{
					mpt::String::Read<mpt::String::maybeNullTerminated>(client.sndFile.m_szNames[client.sample], tag + 6, length - 6);
				} else if(length > 11 && !mpt::CompareNoCaseAscii(tag, "SAMPLERATE=", 11))
				{
					uint32 sampleRate = ConvertStrTo<uint32>(tag + 11);
					if(sampleRate > 0) sample.nC5Speed = sampleRate;
				} else if(length > 10 && !mpt::CompareNoCaseAscii(tag, "LOOPSTART=", 10))
				{
					loopStart = ConvertStrTo<SmpLength>(tag + 10);
				} else if(length > 11 && !mpt::CompareNoCaseAscii(tag, "LOOPLENGTH=", 11))
				{
					loopLength = ConvertStrTo<SmpLength>(tag + 11);
				}
			}
			if(loopLength > 0)
			{
				sample.nLoopStart = loopStart;
				sample.nLoopEnd = loopStart + loopLength;
				sample.uFlags.set(CHN_LOOP);
				sample.SanitizeLoops();
			}
		}
	}

	static void error_cb(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus, void *)
	{
	}
};

#endif // MPT_WITH_FLAC


bool CSoundFile::ReadFLACSample(SAMPLEINDEX sample, FileReader &file)
//-------------------------------------------------------------------
{
#ifdef MPT_WITH_FLAC
	file.Rewind();
	bool isOgg = false;
#ifdef MPT_WITH_OGG
	uint32 oggFlacBitstreamSerial = 0;
#endif
	// Check whether we are dealing with native FLAC, OggFlac or no FLAC at all.
	if(file.ReadMagic("fLaC"))
	{ // ok
		isOgg = false;
#ifdef MPT_WITH_OGG
	} else if(file.ReadMagic("OggS"))
	{ // use libogg to find the first OggFlac stream header
		file.Rewind();
		bool oggOK = false;
		bool needMoreData = true;
		static const long bufsize = 65536;
		std::size_t readSize = 0;
		char *buf = nullptr;
		ogg_sync_state oy;
		MemsetZero(oy);
		ogg_page og;
		MemsetZero(og);
		std::map<uint32, ogg_stream_state*> oggStreams;
		ogg_packet op;
		MemsetZero(op);
		if(ogg_sync_init(&oy) != 0)
		{
			return false;
		}
		while(needMoreData)
		{
			if(file.NoBytesLeft())
			{ // stop at EOF
				oggOK = false;
				needMoreData = false;
				break;
			}
			buf = ogg_sync_buffer(&oy, bufsize);
			if(!buf)
			{
				oggOK = false;
				needMoreData = false;
				break;
			}
			readSize = file.ReadRaw(buf, bufsize);
			if(ogg_sync_wrote(&oy, static_cast<long>(readSize)) != 0)
			{
				oggOK = false;
				needMoreData = false;
				break;
			}
			while(ogg_sync_pageout(&oy, &og) == 1)
			{
				if(!ogg_page_bos(&og))
				{ // we stop scanning when seeing the first noo-begin-of-stream page
					oggOK = false;
					needMoreData = false;
					break;
				}
				uint32 serial = ogg_page_serialno(&og);
				if(!oggStreams[serial])
				{ // previously unseen stream serial
					oggStreams[serial] = new ogg_stream_state();
					MemsetZero(*(oggStreams[serial]));
					if(ogg_stream_init(oggStreams[serial], serial) != 0)
					{
						delete oggStreams[serial];
						oggStreams.erase(serial);
						oggOK = false;
						needMoreData = false;
						break;
					}
				}
				if(ogg_stream_pagein(oggStreams[serial], &og) != 0)
				{ // invalid page
					oggOK = false;
					needMoreData = false;
					break;
				}
				if(ogg_stream_packetout(oggStreams[serial], &op) != 1)
				{ // partial or broken packet, continue with more data
					continue;
				}
				if(op.packetno != 0)
				{ // non-begin-of-stream packet.
					// This should not appear on first page for any known ogg codec,
					// but deal gracefully with badly mused streams in that regard.
					continue;
				}
				FileReader packet(op.packet, op.bytes);
				if(packet.ReadIntLE<uint8>() == 0x7f && packet.ReadMagic("FLAC"))
				{ // looks like OggFlac
					oggOK = true;
					oggFlacBitstreamSerial = serial;
					needMoreData = false;
					break;
				}
			}
		}
		while(oggStreams.size() > 0)
		{
			uint32 serial = oggStreams.begin()->first;
			ogg_stream_clear(oggStreams[serial]);
			delete oggStreams[serial];
			oggStreams.erase(serial);
		}
		ogg_sync_clear(&oy);
		if(!oggOK)
		{
			return false;
		}
		isOgg = true;
#else // !MPT_WITH_OGG
	} else if(file.CanRead(78) && file.ReadMagic("OggS"))
	{ // first OggFlac page is exactly 78 bytes long
		// only support plain OggFlac here with the FLAC logical bitstream being the first one
		uint8 oggPageVersion = file.ReadIntLE<uint8>();
		uint8 oggPageHeaderType = file.ReadIntLE<uint8>();
		uint64 oggPageGranulePosition = file.ReadIntLE<uint64>();
		uint32 oggPageBitstreamSerialNumber = file.ReadIntLE<uint32>();
		uint32 oggPageSequenceNumber = file.ReadIntLE<uint32>();
		uint32 oggPageChecksum = file.ReadIntLE<uint32>();
		uint8 oggPageSegments = file.ReadIntLE<uint8>();
		uint8 oggPageSegmentLength = file.ReadIntLE<uint8>();
		if(oggPageVersion != 0)
		{ // unknown Ogg version
			return false;
		}
		if(!(oggPageHeaderType & 0x02) || (oggPageHeaderType& 0x01))
		{ // not BOS or continuation
			return false;
		}
		if(oggPageGranulePosition != 0)
		{ // not starting position
			return false;
		}
		if(oggPageSequenceNumber != 0)
		{ // not first page
			return false;
		}
		// skip CRC check for now
		if(oggPageSegments != 1)
		{ // first OggFlac page must contain exactly 1 segment
			return false;
		}
		if(oggPageSegmentLength != 51)
		{ // segment length must be 51 bytes in OggFlac mapping
			return false;
		}
		if(file.ReadIntLE<uint8>() != 0x7f)
		{ // OggFlac mapping demands 0x7f packet type
			return false;
		}
		if(!file.ReadMagic("FLAC"))
		{ // OggFlac magic
			return false;
		}
		if(file.ReadIntLE<uint8>() != 0x01)
		{ // OggFlac major version
			return false;
		}
		// by now, we are pretty confident that we are not parsing random junk
		isOgg = true;
#endif // MPT_WITH_OGG
	} else
	{
		return false;
	}
	file.Rewind();

	FLAC__StreamDecoder *decoder = FLAC__stream_decoder_new();
	if(decoder == nullptr)
	{
		return false;
	}

#ifdef MPT_WITH_OGG
	if(isOgg)
	{
		// force flac decoding of the logical bitstream that actually is OggFlac
		if(!FLAC__stream_decoder_set_ogg_serial_number(decoder, oggFlacBitstreamSerial))
		{
			FLAC__stream_decoder_delete(decoder);
			return false;
		}
	}
#endif

	// Give me all the metadata!
	FLAC__stream_decoder_set_metadata_respond_all(decoder);

	FLACDecoder client(file, *this, sample);

	// Init decoder
	FLAC__StreamDecoderInitStatus initStatus = isOgg ?
		FLAC__stream_decoder_init_ogg_stream(decoder, FLACDecoder::read_cb, FLACDecoder::seek_cb, FLACDecoder::tell_cb, FLACDecoder::length_cb, FLACDecoder::eof_cb, FLACDecoder::write_cb, FLACDecoder::metadata_cb, FLACDecoder::error_cb, &client)
		:
		FLAC__stream_decoder_init_stream(decoder, FLACDecoder::read_cb, FLACDecoder::seek_cb, FLACDecoder::tell_cb, FLACDecoder::length_cb, FLACDecoder::eof_cb, FLACDecoder::write_cb, FLACDecoder::metadata_cb, FLACDecoder::error_cb, &client)
		;
	if(initStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK)
	{
		FLAC__stream_decoder_delete(decoder);
		return false;
	}

	// Decode file
	FLAC__stream_decoder_process_until_end_of_stream(decoder);
	FLAC__stream_decoder_finish(decoder);
	FLAC__stream_decoder_delete(decoder);

	if(client.ready && Samples[sample].pSample != nullptr)
	{
		Samples[sample].Convert(MOD_TYPE_IT, GetType());
		Samples[sample].PrecomputeLoops(*this, false);
		return true;
	}
#else
	MPT_UNREFERENCED_PARAMETER(sample);
	MPT_UNREFERENCED_PARAMETER(file);
#endif // MPT_WITH_FLAC
	return false;
}


#ifdef MPT_WITH_FLAC
// Helper function for copying OpenMPT's sample data to FLAC's int32 buffer.
template<typename T>
inline static void SampleToFLAC32(FLAC__int32 *dst, const T *src, SmpLength numSamples)
{
	for(SmpLength i = 0; i < numSamples; i++)
	{
		dst[i] = src[i];
	}
};

// RAII-style helper struct for FLAC encoder
struct FLAC__StreamEncoder_RAII
{
	FLAC__StreamEncoder *encoder;
	FILE *f;

	operator FLAC__StreamEncoder *() { return encoder; }

	FLAC__StreamEncoder_RAII() : encoder(FLAC__stream_encoder_new()), f(nullptr) { }
	~FLAC__StreamEncoder_RAII()
	{
		FLAC__stream_encoder_delete(encoder);
		if(f != nullptr) fclose(f);
	}
};

#endif


#ifndef MODPLUG_NO_FILESAVE
bool CSoundFile::SaveFLACSample(SAMPLEINDEX nSample, const mpt::PathString &filename) const
//-----------------------------------------------------------------------------------------
{
#ifdef MPT_WITH_FLAC
	FLAC__StreamEncoder_RAII encoder;
	if(encoder == nullptr)
	{
		return false;
	}

	const ModSample &sample = Samples[nSample];
	uint32 sampleRate = sample.GetSampleRate(GetType());

	// First off, set up all the metadata...
	FLAC__StreamMetadata *metadata[] =
	{
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT),
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION),	// MPT sample information
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION),	// Loop points
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION),	// Cue points
	};

	unsigned numBlocks = 2;
	if(metadata[0])
	{
		// Store sample name
		FLAC__StreamMetadata_VorbisComment_Entry entry;
		FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "TITLE", m_szNames[nSample]);
		FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, false);
		FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ENCODER", MptVersion::GetOpenMPTVersionStr().c_str());
		FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, false);
		if(sampleRate > FLAC__MAX_SAMPLE_RATE)
		{
			// FLAC only supports a sample rate of up to 655350 Hz.
			// Store the real sample rate in a custom Vorbis comment.
			FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "SAMPLERATE", mpt::ToString(sampleRate).c_str());
			FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, false);
		}
	}
	if(metadata[1])
	{
		// Write MPT sample information
		memcpy(metadata[1]->data.application.id, "riff", 4);

		struct
		{
			RIFFChunk header;
			WAVExtraChunk mptInfo;
		} chunk;

		chunk.header.id = RIFFChunk::idxtra;
		chunk.header.length = sizeof(WAVExtraChunk);

		chunk.mptInfo.ConvertToWAV(sample, GetType());

		const uint32 length = sizeof(RIFFChunk) + sizeof(WAVExtraChunk);

		FLAC__metadata_object_application_set_data(metadata[1], reinterpret_cast<FLAC__byte *>(&chunk), length, true);
	}
	if(metadata[numBlocks] && sample.uFlags[CHN_LOOP | CHN_SUSTAINLOOP])
	{
		// Store loop points
		memcpy(metadata[numBlocks]->data.application.id, "riff", 4);

		struct
		{
			RIFFChunk header;
			WAVSampleInfoChunk info;
			WAVSampleLoop loops[2];
		} chunk;

		chunk.header.id = RIFFChunk::idsmpl;
		chunk.header.length = sizeof(WAVSampleInfoChunk);

		chunk.info.ConvertToWAV(sample.GetSampleRate(GetType()), sample.rootNote);

		if(sample.uFlags[CHN_SUSTAINLOOP])
		{
			chunk.loops[chunk.info.numLoops++].ConvertToWAV(sample.nSustainStart, sample.nSustainEnd, sample.uFlags[CHN_PINGPONGSUSTAIN]);
			chunk.header.length += sizeof(WAVSampleLoop);
		}
		if(sample.uFlags[CHN_LOOP])
		{
			chunk.loops[chunk.info.numLoops++].ConvertToWAV(sample.nLoopStart, sample.nLoopEnd, sample.uFlags[CHN_PINGPONGLOOP]);
			chunk.header.length += sizeof(WAVSampleLoop);
		}

		const uint32 length = sizeof(RIFFChunk) + chunk.header.length;

		FLAC__metadata_object_application_set_data(metadata[numBlocks], reinterpret_cast<FLAC__byte *>(&chunk), length, true);
		numBlocks++;
	}
	if(metadata[numBlocks] && sample.HasCustomCuePoints())
	{
		// Store cue points
		memcpy(metadata[numBlocks]->data.application.id, "riff", 4);

		struct
		{
			RIFFChunk header;
			uint32le numPoints;
			WAVCuePoint cues[CountOf(sample.cues)];
		} chunk;

		chunk.header.id = RIFFChunk::idcue_;
		chunk.header.length = 4 + sizeof(chunk.cues);
		chunk.numPoints = CountOf(sample.cues);

		for(uint32 i = 0; i < CountOf(sample.cues); i++)
		{
			chunk.cues[i].ConvertToWAV(i, sample.cues[i]);
		}

		const uint32 length = sizeof(RIFFChunk) + chunk.header.length;

		FLAC__metadata_object_application_set_data(metadata[numBlocks], reinterpret_cast<FLAC__byte *>(&chunk), length, true);
		numBlocks++;
	}

	// FLAC allows a maximum sample rate of 655350 Hz.
	// If the real rate is higher, we store it in a Vorbis comment above.
	LimitMax(sampleRate, FLAC__MAX_SAMPLE_RATE);
	if(!FLAC__format_sample_rate_is_subset(sampleRate))
	{
		// FLAC only supports 10 Hz granularity for frequencies above 65535 Hz if the streamable subset is chosen.
		FLAC__stream_encoder_set_streamable_subset(encoder, false);
	}
	FLAC__stream_encoder_set_channels(encoder, sample.GetNumChannels());
	FLAC__stream_encoder_set_bits_per_sample(encoder, sample.GetElementarySampleSize() * 8);
	FLAC__stream_encoder_set_sample_rate(encoder, sampleRate);
	FLAC__stream_encoder_set_total_samples_estimate(encoder, sample.nLength);
	FLAC__stream_encoder_set_metadata(encoder, metadata, numBlocks);
#ifdef MODPLUG_TRACKER
	FLAC__stream_encoder_set_compression_level(encoder, TrackerSettings::Instance().m_FLACCompressionLevel);
#endif // MODPLUG_TRACKER

	bool result = false;
	FLAC__int32 *sampleData = nullptr;

	encoder.f = mpt_fopen(filename, "wb");
	if(encoder.f == nullptr || FLAC__stream_encoder_init_FILE(encoder, encoder.f, nullptr, nullptr) != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
	{
		goto fail;
	}

	// Convert sample data to signed 32-Bit integer array.
	const SmpLength numSamples = sample.nLength * sample.GetNumChannels();
	sampleData = new (std::nothrow) FLAC__int32[numSamples];
	if(sampleData == nullptr)
	{
		goto fail;
	}

	if(sample.GetElementarySampleSize() == 1)
	{
		SampleToFLAC32(sampleData, sample.pSample8, numSamples);
	} else if(sample.GetElementarySampleSize() == 2)
	{
		SampleToFLAC32(sampleData, sample.pSample16, numSamples);
	} else
	{
		MPT_ASSERT_NOTREACHED();
	}

	// Do the actual conversion.
	FLAC__stream_encoder_process_interleaved(encoder, sampleData, sample.nLength);
	result = true;

fail:
	FLAC__stream_encoder_finish(encoder);

	delete[] sampleData;
	for(size_t i = 0; i < CountOf(metadata); i++)
	{
		FLAC__metadata_object_delete(metadata[i]);
	}

	return result;
#else
	MPT_UNREFERENCED_PARAMETER(nSample);
	MPT_UNREFERENCED_PARAMETER(filename);
	return false;
#endif // MPT_WITH_FLAC
}
#endif // MODPLUG_NO_FILESAVE


////////////////////////////////////////////////////////////////////////////////
// Opus

#if defined(MPT_WITH_OPUSFILE)

static mpt::ustring UStringFromOpus(const char *str)
//--------------------------------------------------
{
	return str ? mpt::ToUnicode(mpt::CharsetUTF8, str) : mpt::ustring();
}

static FileTags GetOpusFileTags(OggOpusFile *of)
//----------------------------------------------
{
	FileTags tags;
	const OpusTags *ot = op_tags(of, -1);
	if(!ot)
	{
		return tags;
	}
	tags.encoder = UStringFromOpus(opus_tags_query(ot, "ENCODER", 0));
	tags.title = UStringFromOpus(opus_tags_query(ot, "TITLE", 0));
	tags.comments = UStringFromOpus(opus_tags_query(ot, "DESCRIPTION", 0));
	tags.bpm = UStringFromOpus(opus_tags_query(ot, "BPM", 0)); // non-standard
	tags.artist = UStringFromOpus(opus_tags_query(ot, "ARTIST", 0));
	tags.album = UStringFromOpus(opus_tags_query(ot, "ALBUM", 0));
	tags.trackno = UStringFromOpus(opus_tags_query(ot, "TRACKNUMBER", 0));
	tags.year = UStringFromOpus(opus_tags_query(ot, "DATE", 0));
	tags.url = UStringFromOpus(opus_tags_query(ot, "CONTACT", 0));
	tags.genre = UStringFromOpus(opus_tags_query(ot, "GENRE", 0));
	return tags;
}

#endif // MPT_WITH_OPUSFILE

bool CSoundFile::ReadOpusSample(SAMPLEINDEX sample, FileReader &file)
{
	file.Rewind();

#if defined(MPT_WITH_OPUSFILE)

	int rate = 0;
	int channels = 0;
	std::vector<int16> raw_sample_data;

	FileReader initial = file.GetChunk(65536); // 512 is recommended by libopusfile
	if(op_test(NULL, initial.GetRawData<unsigned char>(), initial.GetLength()) != 0)
	{
		return false;
	}

	OggOpusFile *of = op_open_memory(file.GetRawData<unsigned char>(), file.GetLength(), NULL);
	if(!of)
	{
		return false;
	}

	rate = 48000;
	channels = op_channel_count(of, -1);
	if(rate <= 0 || channels <= 0)
	{
		op_free(of);
		of = NULL;
		return false;
	}
	if(channels > 2 || op_link_count(of) != 1)
	{
		// We downmix multichannel to stereo as recommended by Opus specification in
		// case we are not able to handle > 2 channels.
		// We also decode chained files as stereo even if they start with a mono
		// stream, which simplifies handling of link boundaries for us.
		channels = 2;
	}

	std::vector<int16> decodeBuf(120 * 48000 / 1000); // 120ms (max Opus packet), 48kHz
	bool eof = false;
	while(!eof)
	{
		int framesRead = 0;
		if(channels == 2)
		{
			framesRead = op_read_stereo(of, &(decodeBuf[0]), static_cast<int>(decodeBuf.size()));
		} else if(channels == 1)
		{
			framesRead = op_read(of, &(decodeBuf[0]), static_cast<int>(decodeBuf.size()), NULL);
		}
		if(framesRead > 0)
		{
			raw_sample_data.insert(raw_sample_data.end(), decodeBuf.begin(), decodeBuf.begin() + (framesRead * channels));
		} else if(framesRead == 0)
		{
			eof = true;
		} else if(framesRead == OP_HOLE)
		{
			// continue
		} else
		{
			// other errors are fatal, stop decoding
			eof = true;
		}
	}

	op_free(of);
	of = NULL;

	if(raw_sample_data.empty())
	{
		return false;
	}

	DestroySampleThreadsafe(sample);
	strcpy(m_szNames[sample], "");
	Samples[sample].Initialize();
	Samples[sample].nC5Speed = rate;
	Samples[sample].nLength = raw_sample_data.size() / channels;

	Samples[sample].uFlags.set(CHN_16BIT);
	Samples[sample].uFlags.set(CHN_STEREO, channels == 2);
	Samples[sample].AllocateSample();

	std::copy(raw_sample_data.begin(), raw_sample_data.end(), Samples[sample].pSample16);

	Samples[sample].Convert(MOD_TYPE_IT, GetType());
	Samples[sample].PrecomputeLoops(*this, false);

	return Samples[sample].pSample != nullptr;

#else // !MPT_WITH_OPUSFILE

	MPT_UNREFERENCED_PARAMETER(sample);
	MPT_UNREFERENCED_PARAMETER(file);

	return false;

#endif // MPT_WITH_OPUSFILE

}


////////////////////////////////////////////////////////////////////////////////
// Vorbis

#if defined(MPT_WITH_VORBISFILE)

static size_t VorbisfileFilereaderRead(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	FileReader &file = *reinterpret_cast<FileReader*>(datasource);
	return file.ReadRaw(mpt::void_cast<mpt::byte*>(ptr), size * nmemb) / size;
}

static int VorbisfileFilereaderSeek(void *datasource, ogg_int64_t offset, int whence)
{
	FileReader &file = *reinterpret_cast<FileReader*>(datasource);
	switch(whence)
	{
	case SEEK_SET:
		{
			if(!Util::TypeCanHoldValue<FileReader::off_t>(offset))
			{
				return -1;
			}
			return file.Seek(mpt::saturate_cast<FileReader::off_t>(offset)) ? 0 : -1;
		}
		break;
	case SEEK_CUR:
		{
			if(offset < 0)
			{
				if(offset == std::numeric_limits<ogg_int64_t>::min())
				{
					return -1;
				}
				if(!Util::TypeCanHoldValue<FileReader::off_t>(0-offset))
				{
					return -1;
				}
				return file.SkipBack(mpt::saturate_cast<FileReader::off_t>(0 - offset)) ? 0 : -1;
			} else
			{
				if(!Util::TypeCanHoldValue<FileReader::off_t>(offset))
				{
					return -1;
				}
				return file.Skip(mpt::saturate_cast<FileReader::off_t>(offset)) ? 0 : -1;
			}
		}
		break;
	case SEEK_END:
		{
			if(!Util::TypeCanHoldValue<FileReader::off_t>(offset))
			{
				return -1;
			}
			if(!Util::TypeCanHoldValue<FileReader::off_t>(file.GetLength() + offset))
			{
				return -1;
			}
			return file.Seek(mpt::saturate_cast<FileReader::off_t>(file.GetLength() + offset)) ? 0 : -1;
		}
		break;
	default:
		return -1;
	}
}

static long VorbisfileFilereaderTell(void *datasource)
{
	FileReader &file = *reinterpret_cast<FileReader*>(datasource);
	return file.GetPosition();
}

#if defined(MPT_WITH_VORBIS)
static mpt::ustring UStringFromVorbis(const char *str)
//----------------------------------------------------
{
	return str ? mpt::ToUnicode(mpt::CharsetUTF8, str) : mpt::ustring();
}
#endif // MPT_WITH_VORBIS

static FileTags GetVorbisFileTags(OggVorbis_File &vf)
//---------------------------------------------------
{
	FileTags tags;
	#if defined(MPT_WITH_VORBIS)
		vorbis_comment *vc = ov_comment(&vf, -1);
		if(!vc)
		{
			return tags;
		}
		tags.encoder = UStringFromVorbis(vorbis_comment_query(vc, "ENCODER", 0));
		tags.title = UStringFromVorbis(vorbis_comment_query(vc, "TITLE", 0));
		tags.comments = UStringFromVorbis(vorbis_comment_query(vc, "DESCRIPTION", 0));
		tags.bpm = UStringFromVorbis(vorbis_comment_query(vc, "BPM", 0)); // non-standard
		tags.artist = UStringFromVorbis(vorbis_comment_query(vc, "ARTIST", 0));
		tags.album = UStringFromVorbis(vorbis_comment_query(vc, "ALBUM", 0));
		tags.trackno = UStringFromVorbis(vorbis_comment_query(vc, "TRACKNUMBER", 0));
		tags.year = UStringFromVorbis(vorbis_comment_query(vc, "DATE", 0));
		tags.url = UStringFromVorbis(vorbis_comment_query(vc, "CONTACT", 0));
		tags.genre = UStringFromVorbis(vorbis_comment_query(vc, "GENRE", 0));
	#endif // MPT_WITH_VORBIS
	return tags;
}

#endif // MPT_WITH_VORBISFILE

bool CSoundFile::ReadVorbisSample(SAMPLEINDEX sample, FileReader &file)
//---------------------------------------------------------------------
{

#if defined(MPT_WITH_VORBISFILE) || defined(MPT_WITH_STBVORBIS)

	file.Rewind();

	int rate = 0;
	int channels = 0;
	std::vector<int16> raw_sample_data;

	std::string sampleName;

#endif // VORBIS

#if defined(MPT_WITH_VORBISFILE)

	bool unsupportedSample = false;

	ov_callbacks callbacks = {
		&VorbisfileFilereaderRead,
		&VorbisfileFilereaderSeek,
		NULL,
		&VorbisfileFilereaderTell
	};
	OggVorbis_File vf;
	MemsetZero(vf);
	if(ov_open_callbacks(&file, &vf, NULL, 0, callbacks) == 0)
	{
		if(ov_streams(&vf) == 1)
		{ // we do not support chained vorbis samples
			vorbis_info *vi = ov_info(&vf, -1);
			if(vi && vi->rate > 0 && vi->channels > 0)
			{
				sampleName = mpt::ToCharset(GetCharsetLocaleOrModule(), GetSampleNameFromTags(GetVorbisFileTags(vf)));
				rate = vi->rate;
				channels = vi->channels;
				std::size_t offset = 0;
				int current_section = 0;
				long decodedSamples = 0;
				bool eof = false;
				while(!eof)
				{
					float **output = nullptr;
					long ret = ov_read_float(&vf, &output, 1024, &current_section);
					if(ret == 0)
					{
						eof = true;
					} else if(ret < 0)
					{
						// stream error, just try to continue
					} else
					{
						decodedSamples = ret;
						if(decodedSamples > 0 && (channels == 1 || channels == 2))
						{
							raw_sample_data.resize(raw_sample_data.size() + (channels * decodedSamples));
							for(int chn = 0; chn < channels; chn++)
							{
								CopyChannelToInterleaved<SC::Convert<int16, float> >(&(raw_sample_data[0]) + offset * channels, output[chn], channels, decodedSamples, chn);
							}
							offset += decodedSamples;
						}
					}
				}
			} else
			{
				unsupportedSample = true;
			}
		} else
		{
			unsupportedSample = true;
		}
		ov_clear(&vf);
	} else
	{
		unsupportedSample = true;
	}

	if(unsupportedSample)
	{
		return false;
	}

#elif defined(MPT_WITH_STBVORBIS)

	// NOTE/TODO: stb_vorbis does not handle inferred negative PCM sample position
	// at stream start. (See
	// <https://www.xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-132000A.2>). This
	// means that, for remuxed and re-aligned/cutted (at stream start) Vorbis
	// files, stb_vorbis will include superfluous samples at the beginning.

	FileReader::PinnedRawDataView fileView = file.GetPinnedRawDataView();
	const mpt::byte* data = fileView.data();
	std::size_t dataLeft = fileView.size();

	std::size_t offset = 0;
	int consumed = 0;
	int error = 0;
	stb_vorbis *vorb = stb_vorbis_open_pushdata(data, mpt::saturate_cast<int>(dataLeft), &consumed, &error, nullptr);
	file.Skip(consumed);
	data += consumed;
	dataLeft -= consumed;
	if(!vorb)
	{
		return false;
	}
	rate = stb_vorbis_get_info(vorb).sample_rate;
	channels = stb_vorbis_get_info(vorb).channels;
	if(rate <= 0 || channels <= 0)
	{
		return false;
	}
	while((error == VORBIS__no_error || (error == VORBIS_need_more_data && dataLeft > 0)))
	{
		int frame_channels = 0;
		int decodedSamples = 0;
		float **output = nullptr;
		consumed = stb_vorbis_decode_frame_pushdata(vorb, data, mpt::saturate_cast<int>(dataLeft), &frame_channels, &output, &decodedSamples);
		file.Skip(consumed);
		data += consumed;
		dataLeft -= consumed;
		LimitMax(frame_channels, channels);
		if(decodedSamples > 0 && (frame_channels == 1 || frame_channels == 2))
		{
			raw_sample_data.resize(raw_sample_data.size() + (channels * decodedSamples));
			for(int chn = 0; chn < frame_channels; chn++)
			{
				CopyChannelToInterleaved<SC::Convert<int16, float> >(&(raw_sample_data[0]) + offset * channels, output[chn], channels, decodedSamples, chn);
			}
			offset += decodedSamples;
		}
		error = stb_vorbis_get_error(vorb);
	}
	stb_vorbis_close(vorb);

#endif // VORBIS

#if defined(MPT_WITH_VORBISFILE) || defined(MPT_WITH_STBVORBIS)

	if(rate <= 0 || channels <= 0 || raw_sample_data.empty())
	{
		return false;
	}

	DestroySampleThreadsafe(sample);
	mpt::String::Copy(m_szNames[sample], sampleName);
	Samples[sample].Initialize();
	Samples[sample].nC5Speed = rate;
	Samples[sample].nLength = raw_sample_data.size() / channels;

	Samples[sample].uFlags.set(CHN_16BIT);
	Samples[sample].uFlags.set(CHN_STEREO, channels == 2);
	Samples[sample].AllocateSample();

	std::copy(raw_sample_data.begin(), raw_sample_data.end(), Samples[sample].pSample16);

	Samples[sample].Convert(MOD_TYPE_IT, GetType());
	Samples[sample].PrecomputeLoops(*this, false);

	return Samples[sample].pSample != nullptr;

#else // !VORBIS

	MPT_UNREFERENCED_PARAMETER(sample);
	MPT_UNREFERENCED_PARAMETER(file);

	return false;

#endif // VORBIS

}


///////////////////////////////////////////////////////////////////////////////////////////////////
// MP3 Samples

#if defined(MPT_WITH_MPG123) || defined(MPT_ENABLE_MPG123_DYNBIND)

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

	enum mpg123_parms_flags
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
#if MPT_COMPILER_MSVCCLANGC2
	int (*mpg123_replace_reader_handle)(mpg123_handle*,
		size_t(*r_read)(void *, void *, size_t),
		_off_t(*r_lseek)(void *, _off_t, int),
		void(*cleanup)(void *));
#if !defined(MPT_WITH_MPG123) && defined(MPT_ENABLE_MPG123_DYNBIND)
	int (*mpg123_replace_reader_handle_64)(mpg123_handle*,
		size_t(*r_read)(void *, void *, size_t),
		_off_t(*r_lseek)(void *, _off_t, int),
		void(*cleanup)(void *));
#endif // !MPT_WITH_MPG123 && MPT_ENABLE_MPG123_DYNBIND
#elif MPT_COMPILER_MSVC
	int (*mpg123_replace_reader_handle)(mpg123_handle*,
		size_t(*r_read)(void *, void *, size_t),
		off_t(*r_lseek)(void *, off_t, int),
		void(*cleanup)(void *));
#if !defined(MPT_WITH_MPG123) && defined(MPT_ENABLE_MPG123_DYNBIND)
	int (*mpg123_replace_reader_handle_64)(mpg123_handle*,
		size_t(*r_read)(void *, void *, size_t),
		off_t(*r_lseek)(void *, off_t, int),
		void(*cleanup)(void *));
#endif // !MPT_WITH_MPG123 && MPT_ENABLE_MPG123_DYNBIND
#else // !MPT_COMPILER_MSVC
	int (*mpg123_replace_reader_handle)(mpg123_handle*,
		ssize_t(*r_read)(void *, void *, size_t),
		off_t(*r_lseek)(void *, off_t, int),
		void(*cleanup)(void *));
#if !defined(MPT_WITH_MPG123) && defined(MPT_ENABLE_MPG123_DYNBIND)
	int (*mpg123_replace_reader_handle_64)(mpg123_handle*,
		ssize_t(*r_read)(void *, void *, size_t),
		off_t(*r_lseek)(void *, off_t, int),
		void(*cleanup)(void *));
#endif // !MPT_WITH_MPG123 && MPT_ENABLE_MPG123_DYNBIND
#endif // MPT_COMPILER_MSVC
	int (*mpg123_read )(mpg123_handle*, unsigned char*, size_t, size_t*);
	int (*mpg123_getformat )(mpg123_handle*, long*, int*, int*);
	int (*mpg123_scan )(mpg123_handle*);
#if MPT_COMPILER_MSVCCLANGC2
	_off_t (*mpg123_length )(mpg123_handle*);
#if !defined(MPT_WITH_MPG123) && defined(MPT_ENABLE_MPG123_DYNBIND)
	_off_t (*mpg123_length_64 )(mpg123_handle*);
#endif // !MPT_WITH_MPG123 && MPT_ENABLE_MPG123_DYNBIND
#else
	off_t (*mpg123_length )(mpg123_handle*);
#if !defined(MPT_WITH_MPG123) && defined(MPT_ENABLE_MPG123_DYNBIND)
	off_t (*mpg123_length_64 )(mpg123_handle*);
#endif // !MPT_WITH_MPG123 && MPT_ENABLE_MPG123_DYNBIND
#endif

#if MPT_COMPILER_MSVCCLANGC2
	static size_t FileReaderRead(void *fp, void *buf, size_t count)
#elif MPT_COMPILER_MSVC
	static size_t FileReaderRead(void *fp, void *buf, size_t count)
#else // !MPT_COMPILER_MSVC
	static ssize_t FileReaderRead(void *fp, void *buf, size_t count)
#endif // MPT_COMPILER_MSVC
	{
		FileReader &file = *static_cast<FileReader *>(fp);
		size_t readBytes = std::min(count, static_cast<size_t>(file.BytesLeft()));
		file.ReadRaw(static_cast<char *>(buf), readBytes);
		return readBytes;
	}
#if MPT_COMPILER_MSVCCLANGC2
	static _off_t FileReaderLSeek(void *fp, _off_t offset, int whence)
#else
	static off_t FileReaderLSeek(void *fp, off_t offset, int whence)
#endif
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
		AddLibrary("mpg123", mpt::LibraryPath::AppFullName(MPT_PATHSTRING("libmpg123-0")));
		AddLibrary("mpg123", mpt::LibraryPath::AppFullName(MPT_PATHSTRING("libmpg123")));
		AddLibrary("mpg123", mpt::LibraryPath::AppFullName(MPT_PATHSTRING("mpg123-0")));
		AddLibrary("mpg123", mpt::LibraryPath::AppFullName(MPT_PATHSTRING("mpg123")));
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
		size_t ndecoded;
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


#if defined(MPT_WITH_MEDIAFOUNDATION)

template <typename T>
static void mptMFSafeRelease(T **ppT)
{
	if(*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

#define MPT_MF_CHECKED(x) MPT_DO { \
	HRESULT hr = (x); \
	if(!SUCCEEDED(hr)) \
	{ \
		goto fail; \
	} \
} MPT_WHILE_0

// Implementing IMFByteStream is apparently not enough to stream raw bytes
// data to MediaFoundation.
// Additionally, one has to also implement a custom IMFAsyncResult for the
// BeginRead/EndRead interface which allows transferring the number of read
// bytes around.
// To make things even worse, MediaFoundation fails to detect some AAC and MPEG
// files if a non-file-based or read-only stream is used for opening.
// The only sane option which remains if we do not have an on-disk filename
// available:
//  1 - write a temporary file
//  2 - close it
//  3 - open it using MediaFoundation.
// We use FILE_ATTRIBUTE_TEMPORARY which will try to keep the file data in
// memory just like regular allocated memory and reduce the overhead basically
// to memcpy.

static FileTags ReadMFMetadata(IMFMediaSource *mediaSource)
//---------------------------------------------------------
{

	FileTags tags;

	IMFPresentationDescriptor *presentationDescriptor = NULL;
	DWORD streams = 0;
	IMFMetadataProvider *metadataProvider = NULL;
	IMFMetadata *metadata = NULL;
	PROPVARIANT varPropNames;
	PropVariantInit(&varPropNames);

	MPT_MF_CHECKED(mediaSource->CreatePresentationDescriptor(&presentationDescriptor));
	MPT_MF_CHECKED(presentationDescriptor->GetStreamDescriptorCount(&streams));
	MPT_MF_CHECKED(MFGetService(mediaSource, MF_METADATA_PROVIDER_SERVICE, IID_IMFMetadataProvider, (void**)&metadataProvider));
	MPT_MF_CHECKED(metadataProvider->GetMFMetadata(presentationDescriptor, 0, 0, &metadata));

	MPT_MF_CHECKED(metadata->GetAllPropertyNames(&varPropNames));
	for(DWORD propIndex = 0; propIndex < varPropNames.calpwstr.cElems; ++propIndex)
	{

		PROPVARIANT propVal;
		PropVariantInit(&propVal);

		LPWSTR propName = varPropNames.calpwstr.pElems[propIndex];
		if(S_OK != metadata->GetProperty(propName, &propVal))
		{
			PropVariantClear(&propVal);
			break;
		}

		std::wstring stringVal;
		std::vector<WCHAR> wcharVal(256);
		for(;;)
		{
			HRESULT hrToString = PropVariantToString(propVal, wcharVal.data(), wcharVal.size());
			if(hrToString == S_OK)
			{
				stringVal = wcharVal.data();
				break;
			} else if(hrToString == ERROR_INSUFFICIENT_BUFFER)
			{
				wcharVal.resize(wcharVal.size() * 2);
			} else
			{
				break;
			}
		}

		PropVariantClear(&propVal);

		if(stringVal.length() > 0)
		{
			if(propName == std::wstring(L"Author")) tags.artist = mpt::ToUnicode(stringVal);
			if(propName == std::wstring(L"Title")) tags.title = mpt::ToUnicode(stringVal);
			if(propName == std::wstring(L"WM/AlbumTitle")) tags.album = mpt::ToUnicode(stringVal);
			if(propName == std::wstring(L"WM/Track")) tags.trackno = mpt::ToUnicode(stringVal);
			if(propName == std::wstring(L"WM/Year")) tags.year = mpt::ToUnicode(stringVal);
			if(propName == std::wstring(L"WM/Genre")) tags.genre = mpt::ToUnicode(stringVal);
		}
	}

fail:

	PropVariantClear(&varPropNames);
	mptMFSafeRelease(&metadata);
	mptMFSafeRelease(&metadataProvider);
	mptMFSafeRelease(&presentationDescriptor);

	return tags;

}


class ComponentMediaFoundation : public ComponentLibrary
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	ComponentMediaFoundation()
		: ComponentLibrary(ComponentTypeSystem)
	{
		return;
	}
	virtual bool DoInitialize()
	{
		if(!mpt::Windows::Version::Current().IsAtLeast(mpt::Windows::Version::Win7))
		{
			return false;
		}
		if(!(true
			&& AddLibrary("mf", mpt::LibraryPath::System(MPT_PATHSTRING("mf")))
			&& AddLibrary("mfplat", mpt::LibraryPath::System(MPT_PATHSTRING("mfplat")))
			&& AddLibrary("mfreadwrite", mpt::LibraryPath::System(MPT_PATHSTRING("mfreadwrite")))
			&& AddLibrary("propsys", mpt::LibraryPath::System(MPT_PATHSTRING("propsys")))
			))
		{
			return false;
		}
		if(!SUCCEEDED(MFStartup(MF_VERSION)))
		{
			return false;
		}
		return true;
	}
	virtual ~ComponentMediaFoundation()
	{
		if(IsAvailable())
		{
			MFShutdown();
		}
	}
};
MPT_REGISTERED_COMPONENT(ComponentMediaFoundation, "MediaFoundation")

#endif // MPT_WITH_MEDIAFOUNDATION


#ifdef MODPLUG_TRACKER
std::vector<FileType> CSoundFile::GetMediaFoundationFileTypes()
//-------------------------------------------------------------
{
	std::vector<FileType> result;

#if defined(MPT_WITH_MEDIAFOUNDATION)

	ComponentHandle<ComponentMediaFoundation> mf;
	if(!IsComponentAvailable(mf))
	{
		return result;
	}

	std::map<std::wstring, FileType> guidMap;

	HKEY hkHandlers = NULL;
	LSTATUS regResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows Media Foundation\\ByteStreamHandlers", 0, KEY_READ, &hkHandlers);
	if(regResult != ERROR_SUCCESS)
	{
		return result;
	}

	for(DWORD handlerIndex = 0; ; ++handlerIndex)
	{

		WCHAR handlerTypeBuf[256];
		MemsetZero(handlerTypeBuf);
		regResult = RegEnumKeyW(hkHandlers, handlerIndex, handlerTypeBuf, 256);
		if(regResult != ERROR_SUCCESS)
		{
			break;
		}

		std::wstring handlerType = handlerTypeBuf;

		if(handlerType.length() < 1)
		{
			continue;
		}

		HKEY hkHandler = NULL;
		regResult = RegOpenKeyExW(hkHandlers, handlerTypeBuf, 0, KEY_READ, &hkHandler);
		if(regResult != ERROR_SUCCESS)
		{
			continue;
		}

		for(DWORD valueIndex = 0; ; ++valueIndex)
		{
			WCHAR valueNameBuf[16384];
			MemsetZero(valueNameBuf);
			DWORD valueNameBufLen = 16384;
			DWORD valueType = 0;
			BYTE valueData[16384];
			MemsetZero(valueData);
			DWORD valueDataLen = 16384;
			regResult = RegEnumValueW(hkHandler, valueIndex, valueNameBuf, &valueNameBufLen, NULL, &valueType, valueData, &valueDataLen);
			if(regResult != ERROR_SUCCESS)
			{
				break;
			}
			if(valueNameBufLen <= 0 || valueType != REG_SZ || valueDataLen <= 0)
			{
				continue;
			}

			std::wstring guid = std::wstring(valueNameBuf);

			mpt::ustring description = mpt::ToUnicode(std::wstring(reinterpret_cast<WCHAR*>(valueData)));
			description = mpt::String::Replace(description, MPT_USTRING("Byte Stream Handler"), MPT_USTRING("Files"));
			description = mpt::String::Replace(description, MPT_USTRING("ByteStreamHandler"), MPT_USTRING("Files"));

			guidMap[guid]
				.ShortName(MPT_USTRING("mf"))
				.Description(description)
				;

			if(handlerType[0] == L'.')
			{
				guidMap[guid].AddExtension(mpt::PathString::FromWide(handlerType.substr(1)));
			} else
			{
				guidMap[guid].AddMimeType(mpt::ToCharset(mpt::CharsetASCII, handlerType));
			}

		}

		RegCloseKey(hkHandler);
		hkHandler = NULL;

	}

	RegCloseKey(hkHandlers);
	hkHandlers = NULL;

	for(auto it = guidMap.cbegin(); it != guidMap.cend(); ++it)
	{
		result.push_back(it->second);
	}

#endif // MPT_WITH_MEDIAFOUNDATION

	return result;
}
#endif // MODPLUG_TRACKER


bool CSoundFile::ReadMediaFoundationSample(SAMPLEINDEX sample, FileReader &file, bool mo3Decode)
//----------------------------------------------------------------------------------------------
{

#if !defined(MPT_WITH_MEDIAFOUNDATION)

	MPT_UNREFERENCED_PARAMETER(sample);
	MPT_UNREFERENCED_PARAMETER(file);
	MPT_UNREFERENCED_PARAMETER(mo3Decode);
	return false;

#else

	ComponentHandle<ComponentMediaFoundation> mf;
	if(!IsComponentAvailable(mf))
	{
		return false;
	}

	file.Rewind();
	// When using MF to decode MP3 samples in MO3 files, we need the mp3 file extension
	// for some of them or otherwise MF refuses to recognize them.
	mpt::PathString tmpfileExtension = (mo3Decode ? MPT_PATHSTRING("mp3") : MPT_PATHSTRING("tmp"));
	OnDiskFileWrapper diskfile(file, tmpfileExtension);
	if(!diskfile.IsValid())
	{
		return false;
	}

	bool result = false;

	std::vector<char> rawData;
	FileTags tags;
	std::string sampleName;

	IMFSourceResolver *sourceResolver = NULL;
	MF_OBJECT_TYPE objectType = MF_OBJECT_INVALID;
	IUnknown *unknownMediaSource = NULL;
	IMFMediaSource *mediaSource = NULL;
	IMFSourceReader *sourceReader = NULL;
	IMFMediaType *uncompressedAudioType = NULL;
	IMFMediaType *partialType = NULL;
	UINT32 numChannels = 0;
	UINT32 samplesPerSecond = 0;
	UINT32 bitsPerSample = 0;

	IMFSample *mfSample = NULL;
	DWORD mfSampleFlags = 0;
	IMFMediaBuffer *buffer = NULL;

	SmpLength length = 0;

	MPT_MF_CHECKED(MFCreateSourceResolver(&sourceResolver));
	MPT_MF_CHECKED(sourceResolver->CreateObjectFromURL(diskfile.GetFilename().AsNative().c_str(), MF_RESOLUTION_MEDIASOURCE | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE | MF_RESOLUTION_READ, NULL, &objectType, &unknownMediaSource));
	if(objectType != MF_OBJECT_MEDIASOURCE) goto fail;
	MPT_MF_CHECKED(unknownMediaSource->QueryInterface(&mediaSource));

	tags = ReadMFMetadata(mediaSource);

	MPT_MF_CHECKED(MFCreateSourceReaderFromMediaSource(mediaSource, NULL, &sourceReader));
	MPT_MF_CHECKED(MFCreateMediaType(&partialType));
	MPT_MF_CHECKED(partialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio));
	MPT_MF_CHECKED(partialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM));
	MPT_MF_CHECKED(sourceReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, partialType));
	MPT_MF_CHECKED(sourceReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &uncompressedAudioType));
	MPT_MF_CHECKED(sourceReader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE));
	MPT_MF_CHECKED(uncompressedAudioType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &numChannels));
	MPT_MF_CHECKED(uncompressedAudioType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &samplesPerSecond));
	MPT_MF_CHECKED(uncompressedAudioType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bitsPerSample));
	if(numChannels <= 0 || numChannels > 2) goto fail;
	if(samplesPerSecond <= 0) goto fail;
	if(bitsPerSample != 8 && bitsPerSample != 16 && bitsPerSample != 24 && bitsPerSample != 32) goto fail;

	for(;;)
	{
		mfSampleFlags = 0;
		MPT_MF_CHECKED(sourceReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, NULL, &mfSampleFlags, NULL, &mfSample));
		if(mfSampleFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
		{
			break;
		}
		if(mfSampleFlags & MF_SOURCE_READERF_ENDOFSTREAM)
		{
			break;
		}
		MPT_MF_CHECKED(mfSample->ConvertToContiguousBuffer(&buffer));
		{
			BYTE *data = NULL;
			DWORD dataSize = 0;
			MPT_MF_CHECKED(buffer->Lock(&data, NULL, &dataSize));
			rawData.insert(rawData.end(), mpt::byte_cast<char*>(data), mpt::byte_cast<char*>(data + dataSize));
			MPT_MF_CHECKED(buffer->Unlock());
		}
		mptMFSafeRelease(&buffer);
		mptMFSafeRelease(&mfSample);
	}

	mptMFSafeRelease(&uncompressedAudioType);
	mptMFSafeRelease(&partialType);
	mptMFSafeRelease(&sourceReader);

	sampleName = mpt::ToCharset(GetCharsetLocaleOrModule(), GetSampleNameFromTags(tags));

	length = rawData.size() / numChannels / (bitsPerSample/8);

	DestroySampleThreadsafe(sample);
	if(!mo3Decode)
	{
		mpt::String::Copy(m_szNames[sample], sampleName);
		Samples[sample].Initialize();
		Samples[sample].nC5Speed = samplesPerSecond;
	}
	Samples[sample].nLength = length;
	Samples[sample].uFlags.set(CHN_16BIT, bitsPerSample >= 16);
	Samples[sample].uFlags.set(CHN_STEREO, numChannels == 2);
	Samples[sample].AllocateSample();
	if(Samples[sample].pSample == nullptr)
	{
		result = false;
		goto fail;
	}

	if(bitsPerSample == 24)
	{
		if(numChannels == 2)
		{
			CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, littleEndian24> > >(Samples[sample], rawData.data(), rawData.size());
		} else
		{
			CopyMonoSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, littleEndian24> > >(Samples[sample], rawData.data(), rawData.size());
		}
	} else if(bitsPerSample == 32)
	{
		if(numChannels == 2)
		{
			CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, littleEndian32> > >(Samples[sample], rawData.data(), rawData.size());
		} else
		{
			CopyMonoSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, littleEndian32> > >(Samples[sample], rawData.data(), rawData.size());
		}
	} else
	{
		// just copy
		std::copy(rawData.data(), rawData.data() + rawData.size(), mpt::void_cast<char*>(Samples[sample].pSample));
	}

	result = true;

fail:

	mptMFSafeRelease(&buffer);
	mptMFSafeRelease(&mfSample);
	mptMFSafeRelease(&uncompressedAudioType);
	mptMFSafeRelease(&partialType);
	mptMFSafeRelease(&sourceReader);
	mptMFSafeRelease(&mediaSource);
	mptMFSafeRelease(&unknownMediaSource);
	mptMFSafeRelease(&sourceResolver);

	return result;

#endif

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
			ComponentHandle<ComponentMediaFoundation> mf;
			if(IsComponentAvailable(mf))
			{
				result = true;
			}
		}
	#endif
	return result;
}


bool CSoundFile::CanReadVorbis()
//------------------------------
{
	bool result = false;
	#if defined(MPT_WITH_OGG) && defined(MPT_WITH_VORBIS) && defined(MPT_WITH_VORBISFILE)
		if(!result)
		{
			result = true;
		}
	#endif
	#if defined(MPT_WITH_STBVORBIS)
		if(!result)
		{
			result = true;
		}
	#endif
	return result;
}


OPENMPT_NAMESPACE_END
