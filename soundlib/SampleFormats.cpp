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
#ifdef MODPLUG_TRACKER
#include "../mptrack/Mptrack.h"
#include "../mptrack/Moddoc.h"
#include "../mptrack/TrackerSettings.h"
#endif //MODPLUG_TRACKER
#include "../common/AudioCriticalSection.h"
#include "Wav.h"
#include "Tagging.h"
#include "ITTools.h"
#include "XMTools.h"
#include "S3MTools.h"
#include "WAVTools.h"
#include "../common/version.h"
#include "ChunkReader.h"


#ifndef NO_MP3_SAMPLES
// Check for valid MPEG header
static bool IsMPEG(const uint8 (&header)[3])
//------------------------------------------
{
	return header[0] == 0xFF && (header[1] & 0xE0) == 0xE0 && (header[1] & 0x18) != 0x08 && (header[1] & 0x06) != 0x00 && (header[2] & 0xF0) != 0xF0;
}
static bool IsMPEG(FileReader file)
//---------------------------------
{
	uint8 header[3];
	if(!file.CanRead(3))
		return false;
	file.ReadArrayLE(header);
	file.SkipBack(3);
	return IsMPEG(header);
}

static bool IsID3(FileReader file)
//--------------------------------
{
	char header[3];
	if(!file.CanRead(3))
		return false;
	file.ReadArrayLE(header);
	file.SkipBack(3);
	return header[0] == 'I' && header[1] == 'D' && header[2] == '3';
}
#endif // NO_MP3_SAMPLES


bool CSoundFile::ReadSampleFromFile(SAMPLEINDEX nSample, FileReader &file, bool mayNormalize)
//-------------------------------------------------------------------------------------------
{
	file.Rewind();

	const BYTE * const lpMemFile = reinterpret_cast<const BYTE*>(file.GetRawData());
	const DWORD dwFileLength = file.GetLength();

	if(!nSample || nSample >= MAX_SAMPLES) return false;
	if(!ReadWAVSample(nSample, file, mayNormalize)
		&& !ReadXISample(nSample, file)
		&& !ReadITISample(nSample, file)
		&& !ReadAIFFSample(nSample, file, mayNormalize)
		&& !ReadITSSample(nSample, file)
		&& !ReadPATSample(nSample, const_cast<BYTE*>(lpMemFile), dwFileLength)
		&& !ReadIFFSample(nSample, file)
		&& !ReadS3ISample(nSample, file)
		&& !ReadFLACSample(nSample, file)
		&& !ReadMP3Sample(nSample, file))
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
	file.Rewind();
	if(!ReadPATInstrument(nInstr, (const LPBYTE)file.GetRawData(), file.GetLength())
		&& !ReadXIInstrument(nInstr, file)
		&& !ReadITIInstrument(nInstr, file)
		// Generic read
		&& !ReadSampleAsInstrument(nInstr, file, mayNormalize)) return false;

	if(nInstr > GetNumInstruments()) m_nInstruments = nInstr;
	return true;
}


bool CSoundFile::ReadSampleAsInstrument(INSTRUMENTINDEX nInstr, FileReader &file, bool mayNormalize)
//--------------------------------------------------------------------------------------------------
{
	file.Rewind();

	if(!file.CanRead(80))
		return false;
	uint32 psig[20];
	file.ReadArrayLE(psig);
	file.SkipBack(80);
	if(    (!memcmp(&psig[0], "RIFF", 4) && !memcmp(&psig[2], "WAVE", 4))	// RIFF....WAVE signature
		|| (!memcmp(&psig[0], "LIST", 4) && !memcmp(&psig[2], "wave", 4))	// LIST....wave
		||  !memcmp(&psig[76 / 4], "SCRS", 4)								// S3I signature
		|| (!memcmp(&psig[0], "FORM", 4) &&
			  (!memcmp(&psig[2], "AIFF", 4)									// AIFF signature
			|| !memcmp(&psig[2], "AIFC", 4)									// AIFF-C signature
			|| !memcmp(&psig[2], "8SVX", 4)))								// 8SVX signature
		|| psig[0] == LittleEndian(ITSample::magic)							// ITS signature
#ifndef NO_FLAC
		|| !memcmp(&psig[0], "fLaC", 4)										// FLAC signature
#endif // NO_FLAC
#ifndef NO_MP3_SAMPLES
		|| IsMPEG(file)														// MPEG signature
		|| IsID3(file)				// MP3 signature
#endif // NO_MP3_SAMPLES
		)
	{
		// Scanning free sample
		SAMPLEINDEX nSample = GetNextFreeSample(nInstr);
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
		 
		DestroyInstrument(nInstr, deleteAssociatedSamples);
		Instruments[nInstr] = pIns;

		ReadSampleFromFile(nSample, file, mayNormalize);
		return true;
	}
	return false;
}


bool CSoundFile::DestroyInstrument(INSTRUMENTINDEX nInstr, deleteInstrumentSamples removeSamples)
//-----------------------------------------------------------------------------------------------
{
	if(nInstr == 0 || nInstr >= MAX_INSTRUMENTS || !Instruments[nInstr]) return true;

	if(removeSamples == deleteAssociatedSamples)
	{
		RemoveInstrumentSamples(nInstr);
	}

#ifdef MODPLUG_TRACKER
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	m_szInstrumentPath[nInstr - 1] = mpt::PathString();
	if(GetpModDoc())
	{
		GetpModDoc()->m_bsInstrumentModified.reset(nInstr - 1);
	}
// -! NEW_FEATURE#0023
#endif // MODPLUG_TRACKER

	CriticalSection cs;

	ModInstrument *pIns = Instruments[nInstr];
	Instruments[nInstr] = nullptr;
	for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
	{
		if (Chn[i].pModInstrument == pIns)
		{
			Chn[i].pModInstrument = nullptr;
		}
	}
	delete pIns;
	return true;
}


// Removing all unused samples
bool CSoundFile::RemoveInstrumentSamples(INSTRUMENTINDEX nInstr)
//--------------------------------------------------------------
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
			keepSamples[*sample] = false;
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
		|| wavFile.GetBitsPerSample() > 32
		|| (wavFile.GetSampleFormat() != WAVFormatChunk::fmtPCM && wavFile.GetSampleFormat() != WAVFormatChunk::fmtFloat && wavFile.GetSampleFormat() != WAVFormatChunk::fmtIMA_ADPCM && wavFile.GetSampleFormat() != WAVFormatChunk::fmtMP3))
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

	if(wavFile.GetSampleFormat() == WAVFormatChunk::fmtIMA_ADPCM)
	{
		// IMA ADPCM 4:1
		LimitMax(sample.nLength, MAX_SAMPLE_LENGTH);
		sample.uFlags |= CHN_16BIT;
		if(!sample.AllocateSample())
		{
			return false;
		}
		IMAADPCMUnpack16((int16 *)sample.pSample, sample.nLength, FileReader(sampleChunk.GetRawData(), sampleChunk.BytesLeft()), wavFile.GetBlockAlign());
		sample.PrecomputeLoops(*this, false);
	} else if(wavFile.GetSampleFormat() == WAVFormatChunk::fmtMP3)
	{
		// MP3 in WAV
		return ReadMP3Sample(nSample, sampleChunk);
	} else if(!wavFile.IsExtensibleFormat() && wavFile.MayBeCoolEdit16_8() && wavFile.GetSampleFormat() == WAVFormatChunk::fmtPCM && wavFile.GetBitsPerSample() == 32 && wavFile.GetBlockAlign() == wavFile.GetNumChannels() * 4)
	{
		// Syntrillium Cool Edit hack to store IEEE 32bit floating point
		// Format is described as 32bit integer PCM contained in 32bit blocks and an WAVEFORMATEX extension size of 2 which contains a single 16 bit little endian value of 1.
		//  (This is parsed in WAVTools.cpp and returned via MayBeCoolEdit16_8()).
		// The data actually stored in this case is little endian 32bit floating point PCM with 2**15 full scale.
		// Cool Edit calls this format "16.8 float".
		SampleIO sampleIO(
			SampleIO::_32bit,
			(wavFile.GetNumChannels() > 1) ? SampleIO::stereoInterleaved : SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::floatPCM15);
		sampleIO.ReadSample(sample, sampleChunk);
	} else if(!wavFile.IsExtensibleFormat() && wavFile.GetSampleFormat() == WAVFormatChunk::fmtPCM && wavFile.GetBitsPerSample() == 24 && wavFile.GetBlockAlign() == wavFile.GetNumChannels() * 4)
	{
		// Syntrillium Cool Edit hack to store IEEE 32bit floating point
		// Format is described as 24bit integer PCM contained in 32bit blocks.
		// The data actually stored in this case is little endian 32bit floating point PCM with 2**23 full scale.
		// Cool Edit calls this format "24.0 float".
		SampleIO sampleIO(
			SampleIO::_32bit,
			(wavFile.GetNumChannels() > 1) ? SampleIO::stereoInterleaved : SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::floatPCM23);
		sampleIO.ReadSample(sample, sampleChunk);
	} else
	{
		// PCM / Float
		static const SampleIO::Bitdepth bitDepth[] = { SampleIO::_8bit, SampleIO::_16bit, SampleIO::_24bit, SampleIO::_32bit };

		SampleIO sampleIO(
			bitDepth[(wavFile.GetBitsPerSample() - 1) / 8],
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
	WAVWriter file(filename);

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
		.WriteSample(file.GetFile(), sample));

	file.WriteLoopInformation(sample);
	file.WriteExtraInformation(sample, GetType());
	
	FileTags tags;
	tags.title = mpt::ToWide(mpt::CharsetLocale, m_szNames[nSample]);
	tags.encoder = mpt::ToWide(mpt::CharsetLocale, MptVersion::GetOpenMPTVersionStr());
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
	FILE *f;
	if((f = mpt_fopen(filename, "wb")) == NULL) return false;

	const ModSample &sample = Samples[nSample];
	SampleIO(
		sample.uFlags[CHN_16BIT] ? SampleIO::_16bit : SampleIO::_8bit,
		sample.uFlags[CHN_STEREO] ? SampleIO::stereoInterleaved : SampleIO::mono,
		SampleIO::littleEndian,
		SampleIO::signedPCM)
		.WriteSample(f, sample);

	fclose(f);
	return true;
}

#endif // MODPLUG_NO_FILESAVE

/////////////////////////////////////////////////////////////
// GUS Patches

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif


typedef struct PACKED GF1PATCHFILEHEADER
{
	char  magic[8];			// "GF1PATCH"
	CHAR version[4];		// "100", or "110"
	CHAR id[10];			// "ID#000002"
	CHAR copyright[60];		// Copyright
	BYTE instrum;			// Number of instruments in patch
	BYTE voices;			// Number of voices, usually 14
	BYTE channels;			// Number of wav channels that can be played concurently to the patch
	WORD samples;			// Total number of waveforms for all the .PAT
	WORD volume;			// Master volume
	DWORD data_size;
	BYTE reserved2[36];
} GF1PATCHFILEHEADER;

STATIC_ASSERT(sizeof(GF1PATCHFILEHEADER) == 129);


typedef struct PACKED GF1INSTRUMENT
{
	WORD id;				// Instrument id: 0-65535
	char name[16];			// Name of instrument. Gravis doesn't seem to use it
	DWORD size;				// Number of bytes for the instrument with header. (To skip to next instrument)
	BYTE layers;			// Number of layers in instrument: 1-4
	BYTE reserved[40];
} GF1INSTRUMENT;

STATIC_ASSERT(sizeof(GF1INSTRUMENT) == 63);


typedef struct PACKED GF1SAMPLEHEADER
{
	char name[7];			// null terminated string. name of the wave.
	BYTE fractions;			// Start loop point fraction in 4 bits + End loop point fraction in the 4 other bits.
	DWORD length;			// total size of wavesample. limited to 65535 now by the drivers, not the card.
	DWORD loopstart;		// start loop position in the wavesample
	DWORD loopend;			// end loop position in the wavesample
	WORD freq;				// Rate at which the wavesample has been sampled
	DWORD low_freq, high_freq, root_freq;	// check note.h for the correspondance.
	SHORT finetune;			// fine tune. -512 to +512, EXCLUDING 0 cause it is a multiplier. 512 is one octave off, and 1 is a neutral value
	BYTE balance;			// Balance: 0-15. 0=full left, 15 = full right
	BYTE env_rate[6];		// attack rates
	BYTE env_volume[6];		// attack volumes
	BYTE tremolo_sweep, tremolo_rate, tremolo_depth;
	BYTE vibrato_sweep, vibrato_rate, vibrato_depth;
	BYTE flags;
	SHORT scale_frequency;
	WORD scale_factor;
	BYTE reserved[36];
} GF1SAMPLEHEADER;

STATIC_ASSERT(sizeof(GF1SAMPLEHEADER) == 96);

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


typedef struct PACKED GF1LAYER
{
	BYTE previous;			// If !=0 the wavesample to use is from the previous layer. The waveheader is still needed
	BYTE id;				// Layer id: 0-3
	DWORD size;				// data size in bytes in the layer, without the header. to skip to next layer for example:
	BYTE samples;			// number of wavesamples
	BYTE reserved[40];
} GF1LAYER;

STATIC_ASSERT(sizeof(GF1LAYER) == 47);


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


static void PatchToSample(CSoundFile *that, SAMPLEINDEX nSample, LPBYTE lpStream, DWORD dwMemLength)
//--------------------------------------------------------------------------------------------------
{
	ModSample &sample = that->GetSample(nSample);
	DWORD dwMemPos = sizeof(GF1SAMPLEHEADER);
	GF1SAMPLEHEADER *psh = (GF1SAMPLEHEADER *)(lpStream);

	if(dwMemLength < sizeof(GF1SAMPLEHEADER)) return;
	if(psh->name[0])
	{
		mpt::String::Read<mpt::String::maybeNullTerminated>(that->m_szNames[nSample], psh->name);
	}
	sample.Initialize();
	if(psh->flags & 4) sample.uFlags |= CHN_LOOP;
	if(psh->flags & 8) sample.uFlags |= CHN_PINGPONGLOOP;
	sample.nLength = psh->length;
	sample.nLoopStart = psh->loopstart;
	sample.nLoopEnd = psh->loopend;
	sample.nC5Speed = psh->freq;
	sample.nPan = (psh->balance << 4) + 8;
	if(sample.nPan > 256) sample.nPan = 128;
	sample.nVibType = VIB_SINE;
	sample.nVibSweep = psh->vibrato_sweep;
	sample.nVibDepth = psh->vibrato_depth;
	sample.nVibRate = psh->vibrato_rate/4;
	sample.FrequencyToTranspose();
	sample.RelativeTone += static_cast<int8>(84 - PatchFreqToNote(psh->root_freq));
	if(psh->scale_factor)
	{
		sample.RelativeTone = static_cast<uint8>(sample.RelativeTone - (psh->scale_frequency - 60));
	}
	sample.TransposeToFrequency();

	SampleIO sampleIO(
		SampleIO::_8bit,
		SampleIO::mono,
		SampleIO::littleEndian,
		(psh->flags & 2) ? SampleIO::unsignedPCM : SampleIO::signedPCM);

	if(psh->flags & 1)
	{
		sampleIO |= SampleIO::_16bit;
		sample.nLength /= 2;
		sample.nLoopStart /= 2;
		sample.nLoopEnd /= 2;
	}
	FileReader chunk(lpStream + dwMemPos, dwMemLength - dwMemPos);
	sampleIO.ReadSample(sample, chunk);
	sample.PrecomputeLoops(*that, false);
}


bool CSoundFile::ReadPATSample(SAMPLEINDEX nSample, LPBYTE lpStream, DWORD dwMemLength)
//-------------------------------------------------------------------------------------
{
	DWORD dwMemPos = sizeof(GF1PATCHFILEHEADER)+sizeof(GF1INSTRUMENT)+sizeof(GF1LAYER);
	GF1PATCHFILEHEADER *phdr = (GF1PATCHFILEHEADER *)lpStream;
	GF1INSTRUMENT *pinshdr = (GF1INSTRUMENT *)(lpStream+sizeof(GF1PATCHFILEHEADER));

	if ((!lpStream) || (dwMemLength < 512)
	 || memcmp(phdr->magic, "GF1PATCH", 8)
	 || (phdr->version[3] != 0) || (phdr->id[9] != 0) || (phdr->instrum < 1)
	 || (!phdr->samples) || (!pinshdr->layers)) return false;
	
	DestroySampleThreadsafe(nSample);
	PatchToSample(this, nSample, lpStream + dwMemPos, dwMemLength - dwMemPos);
	if (pinshdr->name[0] > ' ')
	{
		memcpy(m_szNames[nSample], pinshdr->name, 16);
		m_szNames[nSample][16] = 0;
	}
	return true;
}


// PAT Instrument
bool CSoundFile::ReadPATInstrument(INSTRUMENTINDEX nInstr, const LPBYTE lpStream, DWORD dwMemLength)
//--------------------------------------------------------------------------------------------------
{
	GF1PATCHFILEHEADER *phdr = (GF1PATCHFILEHEADER *)lpStream;
	GF1INSTRUMENT *pih = (GF1INSTRUMENT *)(lpStream+sizeof(GF1PATCHFILEHEADER));
	GF1LAYER *plh = (GF1LAYER *)(lpStream+sizeof(GF1PATCHFILEHEADER)+sizeof(GF1INSTRUMENT));
	DWORD dwMemPos = sizeof(GF1PATCHFILEHEADER)+sizeof(GF1INSTRUMENT)+sizeof(GF1LAYER);
	UINT nSamples;

	if ((!lpStream) || (dwMemLength < 512)
	 || memcmp(phdr->magic, "GF1PATCH", 8)
	 || (phdr->version[3] != 0) || (phdr->id[9] != 0)
	 || (phdr->instrum < 1) || (!phdr->samples)
	 || (!pih->layers) || (!plh->samples)) return false;
	if (nInstr > m_nInstruments) m_nInstruments = nInstr;

	ModInstrument *pIns = new (std::nothrow) ModInstrument();
	if(pIns == nullptr)
	{
		return false;
	}

	DestroyInstrument(nInstr, deleteAssociatedSamples);

	Instruments[nInstr] = pIns;
	nSamples = plh->samples;
	if (nSamples > 16) nSamples = 16;
	mpt::String::Copy(pIns->name, pih->name);
	pIns->nFadeOut = 2048;
	if (GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))
	{
		pIns->nNNA = NNA_NOTEOFF;
		pIns->nDNA = DNA_NOTEFADE;
	}
	SAMPLEINDEX nFreeSmp = 0;
	UINT nMinSmpNote = 0xff;
	SAMPLEINDEX nMinSmp = 0;
	for (UINT iSmp=0; iSmp<nSamples; iSmp++)
	{
		// Find a free sample
		nFreeSmp = GetNextFreeSample(nInstr, nFreeSmp + 1);
		if (nFreeSmp == SAMPLEINDEX_INVALID) break;
		if (m_nSamples < nFreeSmp) m_nSamples = nFreeSmp;
		if (!nMinSmp) nMinSmp = nFreeSmp;
		// Load it
		GF1SAMPLEHEADER *psh = (GF1SAMPLEHEADER *)(lpStream+dwMemPos);
		PatchToSample(this, nFreeSmp, lpStream+dwMemPos, dwMemLength-dwMemPos);
		LONG nMinNote = (psh->low_freq > 100) ? PatchFreqToNote(psh->low_freq) : 0;
		LONG nMaxNote = (psh->high_freq > 100) ? PatchFreqToNote(psh->high_freq) : NOTE_MAX;
		LONG nBaseNote = (psh->root_freq > 100) ? PatchFreqToNote(psh->root_freq) : -1;
		if ((!psh->scale_factor) && (nSamples == 1)) { nMinNote = 0; nMaxNote = NOTE_MAX; }
		// Fill Note Map
		for (UINT k=0; k<NOTE_MAX; k++)
		{
			if (((LONG)k == nBaseNote)
			 || ((!pIns->Keyboard[k])
			  && ((LONG)k >= nMinNote)
			  && ((LONG)k <= nMaxNote)))
			{
				if(!psh->scale_factor)
					pIns->NoteMap[k] = NOTE_MIDDLEC;

				pIns->Keyboard[k] = nFreeSmp;
				if (k < nMinSmpNote)
				{
					nMinSmpNote = k;
					nMinSmp = nFreeSmp;
				}
			}
		}
	/*
		// Create dummy envelope
		if (!iSmp)
		{
			pIns->dwFlags |= ENV_VOLUME;
			if (psh->flags & 32) pIns->dwFlags |= ENV_VOLSUSTAIN;
			pIns->VolEnv.Values[0] = 64;
			pIns->VolEnv.Ticks[0] = 0;
			pIns->VolEnv.Values[1] = 64;
			pIns->VolEnv.Ticks[1] = 1;
			pIns->VolEnv.Values[2] = 32;
			pIns->VolEnv.Ticks[2] = 20;
			pIns->VolEnv.Values[3] = 0;
			pIns->VolEnv.Ticks[3] = 100;
			pIns->VolEnv.nNodes = 4;
		}
	*/
		// Skip to next sample
		dwMemPos += sizeof(GF1SAMPLEHEADER)+psh->length;
		if (dwMemPos + sizeof(GF1SAMPLEHEADER) >= dwMemLength) break;
	}
	if (nMinSmp)
	{
		// Fill note map and missing samples
		for (UINT k=0; k<NOTE_MAX; k++)
		{
			if (!pIns->NoteMap[k]) pIns->NoteMap[k] = (BYTE)(k+1);
			if (!pIns->Keyboard[k])
			{
				pIns->Keyboard[k] = nMinSmp;
			} else
			{
				nMinSmp = pIns->Keyboard[k];
			}
		}
	}
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

	int32 code = MULTICHAR4_LE_MSVC('M','P','T','X');
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
	// 32-Bit chunk identifiers
	enum AIFFMagic
	{
		idFORM	= 0x464F524D,
		idAIFF	= 0x41494646,
		idAIFC	= 0x41494643,
	};

	uint32 magic;	// FORM
	uint32 length;	// Size of the file, not including magic and length
	uint32 type;	// AIFF or AIFC

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(magic);
		SwapBytesBE(type);
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
		|| fileHeader.magic != AIFFHeader::idFORM
		|| (fileHeader.type != AIFFHeader::idAIFF && fileHeader.type != AIFFHeader::idAIFC))
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
		|| sampleInfo.numChannels == 0 || sampleInfo.numChannels > 2
		|| sampleInfo.sampleSize == 0 || sampleInfo.sampleSize > 32)
	{
		return false;
	}

	// Read compression type in AIFF-C files.
	uint8 compression[4] = { 'N', 'O', 'N', 'E' };
	SampleIO::Endianness endian = SampleIO::bigEndian;
	if(fileHeader.type == AIFFHeader::idAIFC)
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

	static const SampleIO::Bitdepth bitDepth[] = { SampleIO::_8bit, SampleIO::_16bit, SampleIO::_24bit, SampleIO::_32bit };

	SampleIO sampleIO(bitDepth[(sampleInfo.sampleSize - 1) / 8],
		(sampleInfo.numChannels == 2) ? SampleIO::stereoInterleaved : SampleIO::mono,
		endian,
		SampleIO::signedPCM);

	if(!memcmp(compression, "fl32", 4) || !memcmp(compression, "FL32", 4))
	{
		sampleIO |= SampleIO::floatPCM;
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
		size_t numMarkers = markerChunk.ReadUint16BE();

		std::vector<AIFFMarker> markers;
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

	file.Seek(sampleHeader.ConvertToMPT(Samples[nSample]));
	mpt::String::Read<mpt::String::spacePaddedNull>(m_szNames[nSample], sampleHeader.name);
	Samples[nSample].Convert(MOD_TYPE_IT, GetType());

	sampleHeader.GetSampleFormat().ReadSample(Samples[nSample], file);
	Samples[nSample].PrecomputeLoops(*this, false);
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
	ReadITSSample(nSample, file, false);

	return true;
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

	return true;
}


#ifndef MODPLUG_NO_FILESAVE

bool CSoundFile::SaveITIInstrument(INSTRUMENTINDEX nInstr, const mpt::PathString &filename, bool compress) const
//--------------------------------------------------------------------------------------------------------------
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
		itss.ConvertToIT(Samples[*iter], GetType(), compress, compress);

		mpt::String::Write<mpt::String::nullTerminated>(itss.name, m_szNames[*iter]);

		itss.samplepointer = filePos;
		itss.ConvertEndianness();
		fwrite(&itss, 1, sizeof(itss), f);

		// Write sample
		off_t curPos = ftell(f);
		fseek(f, filePos, SEEK_SET);
		filePos += mpt::saturate_cast<uint32>(itss.GetSampleFormat(0x0214).WriteSample(f, Samples[*iter]));
		fseek(f, curPos, SEEK_SET);
	}

	fseek(f, 0, SEEK_END);
	int32 code = MULTICHAR4_LE_MSVC('M','P','T','X');
	SwapBytesLE(code);
	fwrite(&code, 1, sizeof(int32), f);		// Write extension tag
	WriteInstrumentHeaderStructOrField(pIns, f);	// Write full extended header.

	fclose(f);
	return true;
}

#endif // MODPLUG_NO_FILESAVE


void ReadInstrumentExtensionField(ModInstrument* pIns, const uint32 code, const uint16 size, FileReader &file)
//------------------------------------------------------------------------------------------------------------
{
	if(code == MULTICHAR4_LE_MSVC('K','[','.','.'))
	{
		// skip keyboard mapping
		file.Skip(size);
		return;
	}

	bool success = ReadInstrumentHeaderField(pIns, code, size, file);

	if(!success)
	{
		file.Skip(size);
		return;
	}

	if(code == MULTICHAR4_LE_MSVC('n','[','.','.'))
		mpt::String::SetNullTerminator(pIns->name);
	if(code == MULTICHAR4_LE_MSVC('f','n','[','.'))
		mpt::String::SetNullTerminator(pIns->filename);

	if(code == MULTICHAR4_LE_MSVC('d','F','.','.')) // 'dF..' field requires additional processing.
		ConvertReadExtendedFlags(pIns);
}


void ReadExtendedInstrumentProperty(ModInstrument* pIns, const uint32 code, FileReader &file)
//-------------------------------------------------------------------------------------------
{
	uint16 size = file.ReadUint16LE();
	if(!file.CanRead(size))
	{
		return;
	}
	ReadInstrumentExtensionField(pIns, code, size, file);
}


void ReadExtendedInstrumentProperties(ModInstrument* pIns, FileReader &file)
//--------------------------------------------------------------------------
{
	if(!file.ReadMagic("XTPM"))	// 'MPTX'
	{
		return;
	}

	while(file.CanRead(7))
	{
		ReadExtendedInstrumentProperty(pIns, file.ReadUint32LE(), file);
	}
}


void ConvertReadExtendedFlags(ModInstrument *pIns)
//------------------------------------------------
{
	const uint32 dwOldFlags = pIns->dwFlags;

	pIns->VolEnv.dwFlags.set(ENV_ENABLED, (dwOldFlags & dFdd_VOLUME) != 0);
	pIns->VolEnv.dwFlags.set(ENV_SUSTAIN, (dwOldFlags & dFdd_VOLSUSTAIN) != 0);
	pIns->VolEnv.dwFlags.set(ENV_LOOP, (dwOldFlags & dFdd_VOLLOOP) != 0);
	pIns->VolEnv.dwFlags.set(ENV_CARRY, (dwOldFlags & dFdd_VOLCARRY) != 0);

	pIns->PanEnv.dwFlags.set(ENV_ENABLED, (dwOldFlags & dFdd_PANNING) != 0);
	pIns->PanEnv.dwFlags.set(ENV_SUSTAIN, (dwOldFlags & dFdd_PANSUSTAIN) != 0);
	pIns->PanEnv.dwFlags.set(ENV_LOOP, (dwOldFlags & dFdd_PANLOOP) != 0);
	pIns->PanEnv.dwFlags.set(ENV_CARRY, (dwOldFlags & dFdd_PANCARRY) != 0);

	pIns->PitchEnv.dwFlags.set(ENV_ENABLED, (dwOldFlags & dFdd_PITCH) != 0);
	pIns->PitchEnv.dwFlags.set(ENV_SUSTAIN, (dwOldFlags & dFdd_PITCHSUSTAIN) != 0);
	pIns->PitchEnv.dwFlags.set(ENV_LOOP, (dwOldFlags & dFdd_PITCHLOOP) != 0);
	pIns->PitchEnv.dwFlags.set(ENV_CARRY, (dwOldFlags & dFdd_PITCHCARRY) != 0);
	pIns->PitchEnv.dwFlags.set(ENV_FILTER, (dwOldFlags & dFdd_FILTER) != 0);

	pIns->dwFlags.set(INS_SETPANNING, (dwOldFlags & dFdd_SETPANNING) != 0);
	pIns->dwFlags.set(INS_MUTE, (dwOldFlags & dFdd_MUTE) != 0);
}


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
			return SIZE_MAX;
		return SwapBytesReturnBE(length);
	}

	id_type GetID() const
	{
		return static_cast<id_type>(SwapBytesReturnLE(id));
	}
};

STATIC_ASSERT(sizeof(AIFFChunk) == 8);


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

#ifndef NO_FLAC
#define FLAC__NO_DLL
#include <flac/include/FLAC/stream_decoder.h>
#include <flac/include/FLAC/stream_encoder.h>
#include <flac/include/FLAC/metadata.h>
#include "SampleFormatConverters.h"

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
			file.ReadRaw(reinterpret_cast<char *>(buffer), readBytes);
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

		int8 *sampleData8 = static_cast<int8 *>(sample.pSample) + offset;
		int16 *sampleData16 = static_cast<int16 *>(sample.pSample) + offset;

		ASSERT((bps <= 8 && sample.GetElementarySampleSize() == 1) || (bps > 8 && sample.GetElementarySampleSize() == 2));
		ASSERT(modChannels <= FLAC__stream_decoder_get_channels(decoder));
		ASSERT(bps == FLAC__stream_decoder_get_bits_per_sample(decoder));
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
			ChunkReader data(reinterpret_cast<const char*>(metadata->data.application.data), metadata->length);
			ChunkReader::ChunkList<RIFFChunk> chunks = data.ReadChunks<RIFFChunk>(2);

			// We're not really going to read a WAV file here because there will be only one RIFF chunk per metadata event, but we can still re-use the code for parsing RIFF metadata...
			WAVReader riffReader(data);
			riffReader.FindMetadataChunks(chunks);
			riffReader.ApplySampleSettings(sample, client.sndFile.m_szNames[client.sample]);
		} else if(metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT && client.ready)
		{
			// Try reading Vorbis Comments for sample title
			for(FLAC__uint32 i = 0; i < metadata->data.vorbis_comment.num_comments; i++)
			{
				const char *tag = reinterpret_cast<const char *>(metadata->data.vorbis_comment.comments[i].entry);
				const FLAC__uint32 length = metadata->data.vorbis_comment.comments[i].length;
				if(length > 6 && !mpt::strnicmp(tag, "TITLE=", 6))
				{
					mpt::String::Read<mpt::String::maybeNullTerminated>(client.sndFile.m_szNames[client.sample], tag + 6, length - 6);
					break;
				}
			}
		}
	}

	static void error_cb(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus, void *)
	{
	}
};

#endif // NO_FLAC


bool CSoundFile::ReadFLACSample(SAMPLEINDEX sample, FileReader &file)
//-------------------------------------------------------------------
{
#ifndef NO_FLAC
	// Check if we're dealing with FLAC in an OGG container.
	// <del>We won't check for the "fLaC" signature but let libFLAC decide whether a file is valid or not, as some FLAC files might have e.g. leading ID3v2 data.</del> Apparently we do that now.
	file.Rewind();
	if(!file.ReadMagic("fLaC"))
	{
		return false;
	}
	file.Rewind();

	FLAC__StreamDecoder *decoder = FLAC__stream_decoder_new();
	if(decoder == nullptr)
	{
		return false;
	}

	// Give me all the metadata!
	FLAC__stream_decoder_set_metadata_respond_all(decoder);

	FLACDecoder client(file, *this, sample);

	// Init decoder
	FLAC__StreamDecoderInitStatus initStatus = FLAC__stream_decoder_init_stream(decoder, FLACDecoder::read_cb, FLACDecoder::seek_cb, FLACDecoder::tell_cb, FLACDecoder::length_cb, FLACDecoder::eof_cb, FLACDecoder::write_cb, FLACDecoder::metadata_cb, FLACDecoder::error_cb, &client);
	if(initStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK)
	{
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
#endif // NO_FLAC
	return false;
}


#ifndef NO_FLAC
// Helper function for copying OpenMPT's sample data to FLAC's int32 buffer.
template<typename T>
inline static void SampleToFLAC32(FLAC__int32 *dst, const void *src, SmpLength numSamples)
{
	const T *in = reinterpret_cast<const T *>(src);
	for(SmpLength i = 0; i < numSamples; i++)
	{
		dst[i] = in[i];
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
#ifndef NO_FLAC
	FLAC__StreamEncoder_RAII encoder;
	if(encoder == nullptr)
	{
		return false;
	}

	const ModSample &sample = Samples[nSample];

	// First off, set up all the metadata...
	FLAC__StreamMetadata *metadata[] =
	{
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT),
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION),
		FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION),
	};

	const bool writeLoopData = sample.uFlags[CHN_LOOP | CHN_SUSTAINLOOP];
	if(metadata[0])
	{
		// Store sample name
		FLAC__StreamMetadata_VorbisComment_Entry entry;
		FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "TITLE", m_szNames[nSample]);
		FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, false);
		FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ENCODER", MptVersion::GetOpenMPTVersionStr().c_str());
		FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, false);
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
	if(metadata[2] && writeLoopData)
	{
		// Store loop points
		memcpy(metadata[2]->data.application.id, "riff", 4);

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
		
		FLAC__metadata_object_application_set_data(metadata[2], reinterpret_cast<FLAC__byte *>(&chunk), length, true);
	}

	FLAC__stream_encoder_set_channels(encoder, sample.GetNumChannels());
	FLAC__stream_encoder_set_bits_per_sample(encoder, sample.GetElementarySampleSize() * 8);
	FLAC__stream_encoder_set_sample_rate(encoder, sample.GetSampleRate(GetType()));
	FLAC__stream_encoder_set_total_samples_estimate(encoder, sample.nLength);
	FLAC__stream_encoder_set_metadata(encoder, metadata, writeLoopData ? 3 : 2);
#ifdef MODPLUG_TRACKER
	FLAC__stream_encoder_set_compression_level(encoder, TrackerSettings::Instance().m_FLACCompressionLevel);
#endif // MODPLUG_TRACKER

	encoder.f = mpt_fopen(filename, "wb");
	if(encoder.f == nullptr || FLAC__stream_encoder_init_FILE(encoder, encoder.f, nullptr, nullptr) != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
	{
		return false;
	}

	// Convert sample data to signed 32-Bit integer array.
	const SmpLength numSamples = sample.nLength * sample.GetNumChannels();
	FLAC__int32 *sampleData = new (std::nothrow) FLAC__int32[numSamples];
	if(sampleData == nullptr)
	{
		return false;
	}

	if(sample.GetElementarySampleSize() == 1)
	{
		SampleToFLAC32<int8>(sampleData, sample.pSample, numSamples);
	} else if(sample.GetElementarySampleSize() == 2)
	{
		SampleToFLAC32<int16>(sampleData, sample.pSample, numSamples);
	} else
	{
		ASSERT(false);
	}

	// Do the actual conversion.
	FLAC__stream_encoder_process_interleaved(encoder, sampleData, sample.nLength);
	FLAC__stream_encoder_finish(encoder);

	delete[] sampleData;
	for(size_t i = 0; i < CountOf(metadata); i++)
	{
		FLAC__metadata_object_delete(metadata[i]);
	}

	return true;
#else
	MPT_UNREFERENCED_PARAMETER(nSample);
	MPT_UNREFERENCED_PARAMETER(filename);
	return false;
#endif // NO_FLAC
}
#endif // MODPLUG_NO_FILESAVE


///////////////////////////////////////////////////////////////////////////////////////////////////
// MP3 Samples

#ifndef NO_MP3_SAMPLES

namespace mpg123
{
	typedef struct {int foo;} mpg123_handle;
	typedef int (*pfn_init )();
	typedef mpg123_handle* (*pfn_new )(char*,int*);
	typedef void (*pfn_delete )(mpg123_handle*);
	typedef int (*pfn_open_handle )(mpg123_handle*, void*);
	typedef int (*pfn_replace_reader_handle)(mpg123_handle*, 
		size_t(*)(void *, void *, size_t),
		off_t(*)(void *, off_t, int),
		void(*)(void *));
	typedef int (*pfn_read )(mpg123_handle*, unsigned char*, size_t, size_t*);
	typedef int (*pfn_getformat )(mpg123_handle*, long*, int*, int*);
	typedef int (*pfn_scan )(mpg123_handle*);
	typedef off_t (*pfn_length )(mpg123_handle*);

	size_t FileReaderRead(void *fp, void *buf, size_t count)
	{
		FileReader &file = *static_cast<FileReader *>(fp);
		size_t readBytes = std::min(count, static_cast<size_t>(file.BytesLeft()));
		file.ReadRaw(static_cast<char *>(buf), readBytes);
		return readBytes;
	}

	off_t FileReaderLSeek(void *fp, off_t offset, int whence)
	{
		FileReader &file = *static_cast<FileReader *>(fp);
		if(whence == SEEK_CUR) file.Seek(file.GetPosition() + offset);
		else if(whence == SEEK_END) file.Seek(file.GetLength() + offset);
		else file.Seek(offset);
		return file.GetPosition();
	}

	enum mpg123_enc_enum
	{
		MPG123_ENC_16 = 0x040, MPG123_ENC_SIGNED = 0x080,
	};
};

#endif // NO_MP3_SAMPLES


bool CSoundFile::ReadMP3Sample(SAMPLEINDEX sample, FileReader &file)
//------------------------------------------------------------------
{
#ifndef NO_MP3_SAMPLES
	static HMODULE mp3lib = nullptr;

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
		} else if(IsMPEG(header))
		{
			// This is what we want!
			break;
		} else
		{
			// This, on the other hand, isn't.
			return false;
		}
	}

#ifdef MODPLUG_TRACKER
	if(!mp3lib) mp3lib = LoadLibraryW((theApp.GetAppDirPath() + MPT_PATHSTRING("libmpg123-0.dll")).AsNative().c_str());
	if(!mp3lib) mp3lib = LoadLibraryW((theApp.GetAppDirPath() + MPT_PATHSTRING("libmpg123.dll")).AsNative().c_str());
#else
	if(!mp3lib) mp3lib = LoadLibraryW(MPT_PATHSTRING("libmpg123-0.dll").AsNative().c_str());
	if(!mp3lib) mp3lib = LoadLibraryW(MPT_PATHSTRING("libmpg123.dll").AsNative().c_str());
#endif // MODPLUG_TRACKER
	if(!mp3lib) return false;

	#define MP3_DYNAMICBIND(f) mpg123::pfn_ ## f mpg123_ ## f = (mpg123::pfn_ ## f)GetProcAddress(mp3lib, "mpg123_" #f); if(!mpg123_ ## f) return false

	MP3_DYNAMICBIND(init);
	MP3_DYNAMICBIND(new);
	MP3_DYNAMICBIND(delete);
	MP3_DYNAMICBIND(open_handle);
	MP3_DYNAMICBIND(replace_reader_handle);
	MP3_DYNAMICBIND(read);
	MP3_DYNAMICBIND(getformat);
	MP3_DYNAMICBIND(scan);
	MP3_DYNAMICBIND(length);

	#undef MP3_DYNAMICBIND

	if(mpg123_init()) return false;

	mpg123::mpg123_handle *mh;
	int err;
	if((mh = mpg123_new(0, &err)) == nullptr) return false;
	file.Rewind();

	long rate; int nchannels, encoding;
	SmpLength length;

	// Set up decoder...
	if(mpg123_replace_reader_handle(mh, mpg123::FileReaderRead, mpg123::FileReaderLSeek, 0)
		|| mpg123_open_handle(mh, &file)
		|| mpg123_scan(mh)
		|| mpg123_getformat(mh, &rate, &nchannels, &encoding)
		|| !nchannels || nchannels > 2
		|| (encoding & mpg123::MPG123_ENC_16 | mpg123::MPG123_ENC_SIGNED) == 0
		|| (length = mpg123_length(mh)) == 0)
	{
		mpg123_delete(mh);
		return false;
	}

	DestroySampleThreadsafe(sample);
	strcpy(m_szNames[sample], "");
	Samples[sample].Initialize();
	Samples[sample].nLength = length;

	Samples[sample].nC5Speed = rate;
	Samples[sample].uFlags.set(CHN_16BIT);
	Samples[sample].uFlags.set(CHN_STEREO, nchannels == 2);
	Samples[sample].AllocateSample();

	size_t ndecoded;
	mpg123_read(mh, static_cast<unsigned char *>(Samples[sample].pSample), Samples[sample].GetSampleSizeInBytes(), &ndecoded);
	mpg123_delete(mh);

	if(Samples[sample].pSample != nullptr)
	{
		Samples[sample].Convert(MOD_TYPE_IT, GetType());
		Samples[sample].PrecomputeLoops(*this, false);
		return true;
	}
#else
	MPT_UNREFERENCED_PARAMETER(sample);
	MPT_UNREFERENCED_PARAMETER(file);
#endif // NO_MP3_SAMPLES
	return false;
}
