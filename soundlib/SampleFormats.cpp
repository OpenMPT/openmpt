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
#include "SampleFormatConverters.h"
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
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
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
	if(!ReadPATInstrument(nInstr, file)
		&& !ReadXIInstrument(nInstr, file)
		&& !ReadITIInstrument(nInstr, file)
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
	for(std::set<SAMPLEINDEX>::const_iterator sample = referencedSamples.begin(); sample != referencedSamples.end(); sample++)
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
			const std::vector<SAMPLEINDEX>::const_iterator entry = std::find(sourceSample.begin(), sourceSample.end(), sourceIndex);
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
		|| (wavFile.GetBitsPerSample() > 32 && wavFile.GetSampleFormat() != WAVFormatChunk::fmtFloat)
		|| (wavFile.GetBitsPerSample() > 64 && wavFile.GetSampleFormat() == WAVFormatChunk::fmtFloat)
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

		SampleIO sampleIO(
			bitDepth,
			(wavFile.GetNumChannels() > 1) ? SampleIO::stereoInterleaved : SampleIO::mono,
			SampleIO::littleEndian,
			(wavFile.GetBitsPerSample() > 8) ? SampleIO::signedPCM : SampleIO::unsignedPCM);

		if(wavFile.GetSampleFormat() == WAVFormatChunk::fmtFloat)
		{
			sampleIO |= SampleIO::floatPCM;
		}

		if(mayNormalize)
		{
			sampleIO.MayNormalize();
		}

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

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif


struct PACKED GF1PatchFileHeader
{
	char   magic[8];		// "GF1PATCH"
	char   version[4];		// "100", or "110"
	char   id[10];			// "ID#000002"
	char   copyright[60];	// Copyright
	uint8  numInstr;		// Number of instruments in patch
	uint8  voices;			// Number of voices, usually 14
	uint8  channels;		// Number of wav channels that can be played concurently to the patch
	uint16 numSamples;		// Total number of waveforms for all the .PAT
	uint16 volume;			// Master volume
	uint32 dataSize;
	char   reserved2[36];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(numSamples);
		SwapBytesLE(volume);
		SwapBytesLE(dataSize);
	}
};

STATIC_ASSERT(sizeof(GF1PatchFileHeader) == 129);


struct PACKED GF1Instrument
{
	uint16 id;				// Instrument id: 0-65535
	char   name[16];		// Name of instrument. Gravis doesn't seem to use it
	uint32 size;			// Number of bytes for the instrument with header. (To skip to next instrument)
	uint8  layers;			// Number of layers in instrument: 1-4
	char   reserved[40];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(id);
		SwapBytesLE(size);
	}
};

STATIC_ASSERT(sizeof(GF1Instrument) == 63);


struct PACKED GF1SampleHeader
{
	char name[7];			// null terminated string. name of the wave.
	uint8  fractions;		// Start loop point fraction in 4 bits + End loop point fraction in the 4 other bits.
	uint32 length;			// total size of wavesample. limited to 65535 now by the drivers, not the card.
	uint32 loopstart;		// start loop position in the wavesample
	uint32 loopend;			// end loop position in the wavesample
	uint16 freq;			// Rate at which the wavesample has been sampled
	uint32 low_freq, high_freq, root_freq;	// check note.h for the correspondance.
	int16  finetune;		// fine tune. -512 to +512, EXCLUDING 0 cause it is a multiplier. 512 is one octave off, and 1 is a neutral value
	uint8  balance;			// Balance: 0-15. 0=full left, 15 = full right
	uint8  env_rate[6];		// attack rates
	uint8  env_volume[6];	// attack volumes
	uint8  tremolo_sweep, tremolo_rate, tremolo_depth;
	uint8  vibrato_sweep, vibrato_rate, vibrato_depth;
	uint8  flags;
	int16  scale_frequency;
	uint16 scale_factor;
	char reserved[36];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(length);
		SwapBytesLE(loopstart);
		SwapBytesLE(loopend);
		SwapBytesLE(freq);
		SwapBytesLE(low_freq);
		SwapBytesLE(high_freq);
		SwapBytesLE(root_freq);
		SwapBytesLE(finetune);
		SwapBytesLE(scale_frequency);
		SwapBytesLE(scale_factor);
	}
};

STATIC_ASSERT(sizeof(GF1SampleHeader) == 96);

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


struct PACKED GF1Layer
{
	uint8  previous;		// If !=0 the wavesample to use is from the previous layer. The waveheader is still needed
	uint8  id;				// Layer id: 0-3
	uint32 size;			// data size in bytes in the layer, without the header. to skip to next layer for example:
	uint8  samples;			// number of wavesamples
	char   reserved[40];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(size);
	}
};

STATIC_ASSERT(sizeof(GF1Layer) == 47);


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif

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

	file.ReadConvertEndianness(sampleHeader);

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
	sample.FrequencyToTranspose();
	sample.RelativeTone += static_cast<int8>(84 - PatchFreqToNote(sampleHeader.root_freq));
	if(sampleHeader.scale_factor)
	{
		sample.RelativeTone = static_cast<uint8>(sample.RelativeTone - (sampleHeader.scale_frequency - 60));
	}
	sample.TransposeToFrequency();

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
	if(!file.ReadConvertEndianness(fileHeader)
		|| memcmp(fileHeader.magic, "GF1PATCH", 8)
		|| (memcmp(fileHeader.version, "110\0", 4) && memcmp(fileHeader.version, "100\0", 4))
		|| memcmp(fileHeader.id, "ID#000002\0", 10)
		|| !fileHeader.numInstr || !fileHeader.numSamples
		|| !file.ReadConvertEndianness(instrHeader)
		//|| !instrHeader.layers	// DOO.PAT has 0 layers
		|| !file.ReadConvertEndianness(layerHeader)
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
	if(!file.ReadConvertEndianness(fileHeader)
		|| memcmp(fileHeader.magic, "GF1PATCH", 8)
		|| (memcmp(fileHeader.version, "110\0", 4) && memcmp(fileHeader.version, "100\0", 4))
		|| memcmp(fileHeader.id, "ID#000002\0", 10)
		|| !fileHeader.numInstr || !fileHeader.numSamples
		|| !file.ReadConvertEndianness(instrHeader)
		|| !instrHeader.layers
		|| !file.ReadConvertEndianness(layerHeader)
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
	if(!file.ReadConvertEndianness(sampleHeader)
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
	if(!file.ReadConvertEndianness(fileHeader)
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
		if(!file.ReadConvertEndianness(sampleHeader)
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

	header.ConvertEndianness();
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

		xmSample.ConvertEndianness();
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

	int32 code = MAGIC4BE('M','P','T','X');
	fwrite(&code, 1, sizeof(int32), f);		// Write extension tag
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
	if(!file.ReadConvertEndianness(fileHeader)
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

	// Read first sample header
	XMSample sampleHeader;
	file.ReadConvertEndianness(sampleHeader);
	// Gotta skip 'em all!
	file.Skip(sizeof(XMSample) * (fileHeader.numSamples - 1));

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
// AIFF File I/O


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif


// AIFF header
struct PACKED AIFFHeader
{
	char   magic[4];	// FORM
	uint32 length;		// Size of the file, not including magic and length
	char   type[4];		// AIFF or AIFC

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		//SwapBytesBE(length);	// We ignore this field
	}
};

STATIC_ASSERT(sizeof(AIFFHeader) == 12);


// General IFF Chunk header
struct PACKED AIFFChunk
{
	// 32-Bit chunk identifiers
	enum ChunkIdentifiers
	{
		idCOMM	= 0x434F4D4D,
		idSSND	= 0x53534E44,
		idINST	= 0x494E5354,
		idMARK	= 0x4D41524B,
		idNAME	= 0x4E414D45,
	};

	typedef ChunkIdentifiers id_type;

	uint32 id;		// See ChunkIdentifiers
	uint32 length;	// Chunk size without header

	size_t GetLength() const
	{
		return SwapBytesReturnBE(length);
	}

	id_type GetID() const
	{
		return static_cast<id_type>(SwapBytesReturnBE(id));
	}
};

STATIC_ASSERT(sizeof(AIFFChunk) == 8);


// "Common" chunk (in AIFC, a compression ID and compression name follows this header, but apart from that it's identical)
struct PACKED AIFFCommonChunk
{
	uint16 numChannels;
	uint32 numSampleFrames;
	uint16 sampleSize;
	uint8  sampleRate[10];		// Sample rate in 80-Bit floating point

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(numChannels);
		SwapBytesBE(numSampleFrames);
		SwapBytesBE(sampleSize);
	}

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

STATIC_ASSERT(sizeof(AIFFCommonChunk) == 18);


// Sound chunk
struct PACKED AIFFSoundChunk
{
	uint32 offset;
	uint32 blockSize;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(offset);
		SwapBytesBE(blockSize);
	}
};

STATIC_ASSERT(sizeof(AIFFSoundChunk) == 8);


// Marker
struct PACKED AIFFMarker
{
	uint16 id;
	uint32 position;		// Position in sample
	uint8  nameLength;		// Not counting eventually existing padding byte in name string

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(id);
		SwapBytesBE(position);
	}};

STATIC_ASSERT(sizeof(AIFFMarker) == 7);


// Instrument loop
struct PACKED AIFFInstrumentLoop
{
	enum PlayModes
	{
		noLoop		= 0,
		loopNormal	= 1,
		loopBidi	= 2,
	};

	uint16 playMode;
	uint16 beginLoop;		// Marker index
	uint16 endLoop;			// Marker index

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(playMode);
		SwapBytesBE(beginLoop);
		SwapBytesBE(endLoop);
	}
};

STATIC_ASSERT(sizeof(AIFFInstrumentLoop) == 6);


struct PACKED AIFFInstrumentChunk
{
	uint8  baseNote;
	uint8  detune;
	uint8  lowNote;
	uint8  highNote;
	uint8  lowVelocity;
	uint8  highVelocity;
	uint16 gain;
	AIFFInstrumentLoop sustainLoop;
	AIFFInstrumentLoop releaseLoop;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		sustainLoop.ConvertEndianness();
		releaseLoop.ConvertEndianness();
	}
};

STATIC_ASSERT(sizeof(AIFFInstrumentChunk) == 20);


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


bool CSoundFile::ReadAIFFSample(SAMPLEINDEX nSample, FileReader &file, bool mayNormalize)
//---------------------------------------------------------------------------------------
{
	file.Rewind();
	ChunkReader chunkFile(file);

	// Verify header
	AIFFHeader fileHeader;
	if(!chunkFile.ReadConvertEndianness(fileHeader)
		|| memcmp(fileHeader.magic, "FORM", 4)
		|| (memcmp(fileHeader.type, "AIFF", 4) && memcmp(fileHeader.type, "AIFC", 4)))
	{
		return false;
	}

	ChunkReader::ChunkList<AIFFChunk> chunks = chunkFile.ReadChunks<AIFFChunk>(2);

	// Read COMM chunk
	FileReader commChunk(chunks.GetChunk(AIFFChunk::idCOMM));
	AIFFCommonChunk sampleInfo;
	if(!commChunk.ReadConvertEndianness(sampleInfo))
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
	if(!soundChunk.ReadConvertEndianness(sampleHeader)
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
	} else if(sampleInfo.sampleSize > 32)
	{
		// Double-precision floating point is the only 64-bit type supported at the moment.
		return false;
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
	if(markerChunk.IsValid() && chunks.GetChunk(AIFFChunk::idINST).ReadConvertEndianness(instrHeader))
	{
		uint16 numMarkers = markerChunk.ReadUint16BE();

		std::vector<AIFFMarker> markers;
		markers.reserve(numMarkers);
		for(size_t i = 0; i < numMarkers; i++)
		{
			AIFFMarker marker;
			if(!markerChunk.ReadConvertEndianness(marker))
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
		for(std::vector<AIFFMarker>::iterator iter = markers.begin(); iter != markers.end(); iter++)
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
	        sampleIO |= SampleIO::floatPCM; break;
	case 7: sampleIO |= SampleIO::_64bit;			// 64-bit IEEE floating point
	        sampleIO |= SampleIO::floatPCM; break;
	case 27: sampleIO |= SampleIO::_16bit;			// a-law
	        sampleIO |= SampleIO::aLaw; break;
	default: return false;
	}

	if(!file.Seek(dataOffset))
		return false;

	ModSample &mptSample = Samples[nSample];
	DestroySampleThreadsafe(nSample);
	mptSample.Initialize();
	mptSample.nLength = (std::min<FileReader::off_t>(file.BytesLeft(), dataSize) * 8u) / (sampleIO.GetEncodedBitsPerSample() * channels);
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
	if(!file.ReadConvertEndianness(sampleHeader)
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
	if(!file.ReadConvertEndianness(instrumentHeader)
		|| memcmp(instrumentHeader.id, "IMPI", 4)
		|| instrumentHeader.nos == 0)
	{
		return false;
	}
	file.Rewind();
	ModInstrument dummy;
	ITInstrToMPT(file, dummy, instrumentHeader.trkvers);
	return ReadITSSample(nSample, file, false);
}


bool CSoundFile::ReadITIInstrument(INSTRUMENTINDEX nInstr, FileReader &file)
//--------------------------------------------------------------------------
{
	ITInstrument instrumentHeader;
	SAMPLEINDEX smp = 0, nsamples;

	file.Rewind();
	if(!file.ReadConvertEndianness(instrumentHeader)
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
	nsamples = instrumentHeader.nos;

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
		ReadITSSample(smp, file, false);
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
	ITInstrumentEx iti;
	ModInstrument *pIns = Instruments[nInstr];
	FILE *f;

	if((!pIns) || filename.empty()) return false;
	if((f = mpt_fopen(filename, "wb")) == NULL) return false;

	size_t instSize = iti.ConvertToIT(*pIns, false, *this);

	// Create sample assignment table
	std::vector<SAMPLEINDEX> smptable;
	std::vector<SAMPLEINDEX> smpmap(GetNumSamples(), 0);
	for(size_t i = 0; i < NOTE_MAX; i++)
	{
		const SAMPLEINDEX smp = pIns->Keyboard[i];
		if(smp && smp <= GetNumSamples())
		{
			if(!smpmap[smp - 1])
			{
				// We haven't considered this sample yet.
				smptable.push_back(smp);
				smpmap[smp - 1] = static_cast<SAMPLEINDEX>(smptable.size());
			}
			iti.iti.keyboard[i * 2 + 1] = static_cast<uint8>(smpmap[smp - 1]);
		} else
		{
			iti.iti.keyboard[i * 2 + 1] = 0;
		}
	}
	smpmap.clear();

	uint32 filePos = instSize;
	iti.ConvertEndianness();
	fwrite(&iti, 1, instSize, f);

	filePos += mpt::saturate_cast<uint32>(smptable.size() * sizeof(ITSample));

	// Writing sample headers + data
	std::vector<SampleIO> sampleFlags;
	for(std::vector<SAMPLEINDEX>::iterator iter = smptable.begin(); iter != smptable.end(); iter++)
	{
		ITSample itss;
		itss.ConvertToIT(Samples[*iter], GetType(), compress, compress, allowExternal);
		const bool isExternal = itss.cvt == ITSample::cvtExternalSample;

		mpt::String::Write<mpt::String::nullTerminated>(itss.name, m_szNames[*iter]);

		itss.samplepointer = filePos;
		itss.ConvertEndianness();
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
				mpt::IO::WriteRaw(f, &filenameU8[0], strSize);
			}
#endif // MPT_EXTERNAL_SAMPLES
		}
		fseek(f, curPos, SEEK_SET);
	}

	fseek(f, 0, SEEK_END);
	int32 code = MAGIC4BE('M','P','T','X');
	SwapBytesLE(code);
	fwrite(&code, 1, sizeof(int32), f);		// Write extension tag
	WriteInstrumentHeaderStructOrField(pIns, f);	// Write full extended header.

	fclose(f);
	return true;
}

#endif // MODPLUG_NO_FILESAVE


///////////////////////////////////////////////////////////////////////////////////////////////////
// 8SVX / 16SVX Samples

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

// IFF File Header
struct PACKED IFFHeader
{
	char   form[4];		// "FORM"
	uint32 size;
	char   magic[4];	// "8SVX" or "16SV"

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(size);
	}
};

STATIC_ASSERT(sizeof(IFFHeader) == 12);


// General IFF Chunk header
struct PACKED IFFChunk
{
	// 32-Bit chunk identifiers
	enum ChunkIdentifiers
	{
		idVHDR	= 0x52444856,
		idBODY	= 0x59444F42,
		idNAME	= 0x454D414E,
	};

	typedef ChunkIdentifiers id_type;

	uint32 id;		// See ChunkIdentifiers
	uint32 length;	// Chunk size without header

	size_t GetLength() const
	{
		if(length == 0)	// Broken files
			return std::numeric_limits<size_t>::max();
		return SwapBytesReturnBE(length);
	}

	id_type GetID() const
	{
		return static_cast<id_type>(SwapBytesReturnLE(id));
	}
};

STATIC_ASSERT(sizeof(IFFChunk) == 8);


struct PACKED IFFSampleHeader
{
	uint32 oneShotHiSamples;	// Samples in the high octave 1-shot part
	uint32 repeatHiSamples;		// Samples in the high octave repeat part
	uint32 samplesPerHiCycle;	// Samples/cycle in high octave, else 0
	uint16 samplesPerSec;		// Data sampling rate
	uint8  octave;				// Octaves of waveforms
	uint8  compression;			// Data compression technique used
	uint32 volume;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(oneShotHiSamples);
		SwapBytesBE(repeatHiSamples);
		SwapBytesBE(samplesPerHiCycle);
		SwapBytesBE(samplesPerSec);
		SwapBytesBE(volume);
	}
};

STATIC_ASSERT(sizeof(IFFSampleHeader) == 20);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


bool CSoundFile::ReadIFFSample(SAMPLEINDEX nSample, FileReader &file)
//--------------------------------------------------------------------
{
	file.Rewind();

	IFFHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader)
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
		|| !vhdrChunk.ReadConvertEndianness(sampleHeader))
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
		chunk.header.ConvertEndianness();
		chunk.mptInfo.ConvertEndianness();

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

		chunk.info.ConvertToWAV(sample.GetSampleRate(GetType()));

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
		chunk.header.ConvertEndianness();
		chunk.info.ConvertEndianness();
		chunk.loops[0].ConvertEndianness();
		chunk.loops[1].ConvertEndianness();

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
			uint32 numPoints;
			WAVCuePoint cues[CountOf(sample.cues)];
		} chunk;

		chunk.header.id = RIFFChunk::idcue_;
		chunk.header.length = 4 + sizeof(chunk.cues);
		chunk.numPoints = SwapBytesReturnLE(CountOf(sample.cues));

		for(uint32 i = 0; i < CountOf(sample.cues); i++)
		{
			chunk.cues[i].ConvertToWAV(i, sample.cues[i]);
			chunk.cues[i].ConvertEndianness();
		}

		const uint32 length = sizeof(RIFFChunk) + chunk.header.length;
		chunk.header.ConvertEndianness();

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
#if MPT_COMPILER_MSVC
	int (*mpg123_replace_reader_handle)(mpg123_handle*,
		size_t(*)(void *, void *, size_t),
		off_t(*)(void *, off_t, int),
		void(*)(void *));
#if !defined(MPT_WITH_MPG123) && defined(MPT_ENABLE_MPG123_DYNBIND)
	int (*mpg123_replace_reader_handle_64)(mpg123_handle*,
		size_t(*)(void *, void *, size_t),
		off_t(*)(void *, off_t, int),
		void(*)(void *));
#endif // !MPT_WITH_MPG123 && MPT_ENABLE_MPG123_DYNBIND
#else // !MPT_COMPILER_MSVC
	int (*mpg123_replace_reader_handle)(mpg123_handle*,
		ssize_t(*)(void *, void *, size_t),
		off_t(*)(void *, off_t, int),
		void(*)(void *));
#if !defined(MPT_WITH_MPG123) && defined(MPT_ENABLE_MPG123_DYNBIND)
	int (*mpg123_replace_reader_handle_64)(mpg123_handle*,
		ssize_t(*)(void *, void *, size_t),
		off_t(*)(void *, off_t, int),
		void(*)(void *));
#endif // !MPT_WITH_MPG123 && MPT_ENABLE_MPG123_DYNBIND
#endif // MPT_COMPILER_MSVC
	int (*mpg123_read )(mpg123_handle*, unsigned char*, size_t, size_t*);
	int (*mpg123_getformat )(mpg123_handle*, long*, int*, int*);
	int (*mpg123_scan )(mpg123_handle*);
	off_t (*mpg123_length )(mpg123_handle*);
#if !defined(MPT_WITH_MPG123) && defined(MPT_ENABLE_MPG123_DYNBIND)
	off_t (*mpg123_length_64 )(mpg123_handle*);
#endif // !MPT_WITH_MPG123 && MPT_ENABLE_MPG123_DYNBIND

#if MPT_COMPILER_MSVC
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
	static off_t FileReaderLSeek(void *fp, off_t offset, int whence)
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
		uint8 header[3];
		file.ReadArray(header);

		if(!memcmp(header, "ID3", 3))
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
		} else if(!memcmp(header, "APE", 3) && file.ReadMagic("TAGEX"))
		{
			// Skip APE tags
			uint32 size = file.ReadUint32LE();
			file.Skip(16 + size);
		} else if(!memcmp(header, "\x00\x00\x00", 3) || !memcmp(header, "\xFF\x00\x00", 3))
		{
			// Some MP3 files are padded with zeroes...
		} else if(header[0] == 0)
		{
			// This might be some padding, followed by an MPEG header, so try again.
			file.SkipBack(2);
		} else if(MPEGFrame::IsMPEGHeader(header))
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
			} catch(MPTMemoryException)
			{
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
			HRESULT hrToString = PropVariantToString(propVal, &wcharVal[0], wcharVal.size());
			if(hrToString == S_OK)
			{
				stringVal = &wcharVal[0];
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

	for(std::map<std::wstring, FileType>::const_iterator it = guidMap.begin(); it != guidMap.end(); ++it)
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

	SmpLength length = rawData.size() / numChannels / (bitsPerSample/8);

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
			CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, littleEndian24> > >(Samples[sample], &rawData[0], rawData.size());
		} else
		{
			CopyMonoSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, littleEndian24> > >(Samples[sample], &rawData[0], rawData.size());
		}
	} else if(bitsPerSample == 32)
	{
		if(numChannels == 2)
		{
			CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, littleEndian32> > >(Samples[sample], &rawData[0], rawData.size());
		} else
		{
			CopyMonoSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, littleEndian32> > >(Samples[sample], &rawData[0], rawData.size());
		}
	} else
	{
		// just copy
		std::copy(&rawData[0], &rawData[0] + rawData.size(), mpt::void_cast<char*>(Samples[sample].pSample));
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
